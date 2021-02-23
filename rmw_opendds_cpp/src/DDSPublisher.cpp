// Copyright 2014-2019 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <rmw_opendds_cpp/DDSPublisher.hpp>
#include <rmw_opendds_cpp/event_converter.hpp>
#include <rmw_opendds_cpp/identifier.hpp>
#include <rmw_opendds_cpp/qos.hpp>
#include <rmw_opendds_cpp/types.hpp>

#include <dds/DCPS/DataWriterImpl_T.h>
#include <dds/DCPS/Marked_Default_Qos.h>

#include <rmw/visibility_control.h>
#include <rmw/incompatible_qos_events_statuses.h>

DDSPublisher * DDSPublisher::from(const rmw_publisher_t * pub)
{
  if (!pub) {
    RMW_SET_ERROR_MSG("rmw_publisher is null");
    return nullptr;
  }
  if (!check_impl_id(pub->implementation_identifier)) {
    return nullptr; // error set
  }
  auto dds_pub = static_cast<DDSPublisher *>(pub->data);
  if (!dds_pub) {
    RMW_SET_ERROR_MSG("dds_pub is null");
    return nullptr;
  }
  return dds_pub;
}

rmw_ret_t DDSPublisher::get_rmw_qos(rmw_qos_profile_t & qos) const
{
  DDS::DataWriterQos dds_qos;
  if (writer_->get_qos(dds_qos) == DDS::RETCODE_OK) {
    dds_qos_to_rmw_qos(dds_qos, qos);
    return RMW_RET_OK;
  } else {
    RMW_SET_ERROR_MSG("get_rmw_qos failed");
  }
  return RMW_RET_ERROR;
}

rmw_ret_t DDSPublisher::to_cdr_stream(const void * ros_message, rcutils_uint8_array_t & cdr_stream)
{
  if (!ros_message) {
    RMW_SET_ERROR_MSG("ros_message is null");
    return RMW_RET_ERROR;
  }
  if (topic_.callbacks()->to_cdr_stream(ros_message, &cdr_stream)) {
    return RMW_RET_OK;
  }
  RMW_SET_ERROR_MSG("to_cdr_stream failed");
  return RMW_RET_ERROR;
}

rmw_ret_t DDSPublisher::get_status(const DDS::StatusMask mask, void * rmw_status)
{
  switch (mask) {
    case DDS::LIVELINESS_LOST_STATUS:
    {
      DDS::LivelinessLostStatus ll;
      rmw_ret_t ret = check_dds_ret_code(writer_->get_liveliness_lost_status(ll));
      if (ret != RMW_RET_OK) {
        return ret;
      }
      auto rmw_ll = static_cast<rmw_liveliness_lost_status_t *>(rmw_status);
      rmw_ll->total_count = ll.total_count;
      rmw_ll->total_count_change = ll.total_count_change;
      break;
    }
    case DDS::OFFERED_DEADLINE_MISSED_STATUS:
    {
      DDS::OfferedDeadlineMissedStatus odm;
      rmw_ret_t ret = check_dds_ret_code(writer_->get_offered_deadline_missed_status(odm));
      if (ret != RMW_RET_OK) {
        return ret;
      }
      auto rmw_odm = static_cast<rmw_offered_deadline_missed_status_t *>(rmw_status);
      rmw_odm->total_count = odm.total_count;
      rmw_odm->total_count_change = odm.total_count_change;
      break;
    }
    case DDS::OFFERED_INCOMPATIBLE_QOS_STATUS:
    {
      DDS::OfferedIncompatibleQosStatus oiq;
      rmw_ret_t ret = check_dds_ret_code(writer_->get_offered_incompatible_qos_status(oiq));
      if (RMW_RET_OK != ret) {
        return ret;
      }
      auto rmw_oiq = static_cast<rmw_offered_qos_incompatible_event_status_t *>(rmw_status);
      rmw_oiq->total_count = oiq.total_count;
      rmw_oiq->total_count_change = oiq.total_count_change;
      rmw_oiq->last_policy_kind = dds_qos_policy_to_rmw_qos_policy(oiq.last_policy_id);
      break;
    }
    default:
      return RMW_RET_UNSUPPORTED;
  }
  return RMW_RET_OK;
}

void DDSPublisher::cleanup()
{
  if (writer_) {
    writer_ = nullptr;
  }
  if (publisher_) {
    publisher_ = nullptr;
  }
  OpenDDSPublisherListener::Raf::destroy(listener_);
}

DDSPublisher::DDSPublisher(DDS::DomainParticipant_var dp
  , const rosidl_message_type_support_t * ros_ts
  , const char * topic_name
  , const rmw_qos_profile_t * rmw_qos
) : topic_(ros_ts, topic_name, rmw_qos, dp)
  , listener_(OpenDDSPublisherListener::Raf::create())
  , publisher_()
  , writer_()
  , publisher_gid_{opendds_identifier, {0}}
{
  try {
    if (!listener_) {
      throw std::runtime_error("OpenDDSPublisherListener failed to contstruct");
    }
    publisher_ = dp->create_publisher(PUBLISHER_QOS_DEFAULT, listener_, DDS::PUBLICATION_MATCHED_STATUS);
    if (!publisher_) {
      throw std::runtime_error("create_publisher failed");
    }

    DDS::DataWriterQos dw_qos;
    if (!get_datawriter_qos(publisher_.in(), *rmw_qos, dw_qos)) {
      throw std::runtime_error("get_datawriter_qos failed");
    }
    writer_ = publisher_->create_datawriter(topic_.get(), dw_qos, NULL, OpenDDS::DCPS::NO_STATUS_MASK);
    if (!writer_) {
      throw std::runtime_error("create_datawriter failed");
    }
    auto wri = dynamic_cast<OpenDDS::DCPS::DataWriterImpl_T<OpenDDSStaticSerializedData>*>(writer_.in());
    wri->set_marshal_skip_serialize(true);

    static_assert(sizeof(OpenDDSPublisherGID) <= RMW_GID_STORAGE_SIZE, "insufficient RMW_GID_STORAGE_SIZE");
    auto gid = reinterpret_cast<OpenDDSPublisherGID*>(publisher_gid_.data);
    gid->publication_handle = writer_->get_instance_handle();
  } catch (const std::exception& e) {
    RMW_SET_ERROR_MSG(e.what());
    cleanup();
    throw;
  } catch (...) {
    RMW_SET_ERROR_MSG("DDSPublisher constructor failed");
    cleanup();
    throw;
  }
}
