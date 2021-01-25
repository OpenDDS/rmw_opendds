// Copyright 2015-2019 Open Source Robotics Foundation, Inc.
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

#include "rmw_opendds_cpp/opendds_static_publisher_info.hpp"

#include "rmw_opendds_shared_cpp/identifier.hpp"
#include "rmw_opendds_shared_cpp/OpenDDSNode.hpp"
#include "process_topic_and_service_names.hpp"
#include "type_support_common.hpp"
#include "rmw_opendds_shared_cpp/event_converter.hpp"
#include "rmw_opendds_shared_cpp/qos.hpp"
#include "opendds_static_serialized_dataTypeSupportImpl.h"

#include <dds/DCPS/DataWriterImpl_T.h>
#include <dds/DCPS/Marked_Default_Qos.h>

#include "rmw/visibility_control.h"
#include "rmw/incompatible_qos_events_statuses.h"

OpenDDSStaticPublisherInfo * OpenDDSStaticPublisherInfo::get_from(const rmw_publisher_t * publisher)
{
  if (!publisher) {
    RMW_SET_ERROR_MSG("publisher is null");
    return nullptr;
  }
  if (!check_impl_id(publisher->implementation_identifier)) {
    return nullptr;
  }
  auto publisher_info = static_cast<OpenDDSStaticPublisherInfo *>(publisher->data);
  if (!publisher_info) {
    RMW_SET_ERROR_MSG("publisher_info is null");
    return nullptr;
  }
  return publisher_info;
}

rmw_ret_t OpenDDSStaticPublisherInfo::get_status(
  DDS::StatusMask mask,
  void * event)
{
  switch (mask) {
    case DDS::LIVELINESS_LOST_STATUS:
      {
        DDS::LivelinessLostStatus liveliness_lost;
        DDS::ReturnCode_t dds_return_code = writer_->get_liveliness_lost_status(liveliness_lost);
        rmw_ret_t from_dds = check_dds_ret_code(dds_return_code);
        if (from_dds != RMW_RET_OK) {
          return from_dds;
        }

        rmw_liveliness_lost_status_t * rmw_liveliness_lost =
          static_cast<rmw_liveliness_lost_status_t *>(event);
        rmw_liveliness_lost->total_count = liveliness_lost.total_count;
        rmw_liveliness_lost->total_count_change = liveliness_lost.total_count_change;

        break;
      }
    case DDS::OFFERED_DEADLINE_MISSED_STATUS:
      {
        DDS::OfferedDeadlineMissedStatus offered_deadline_missed;
        DDS::ReturnCode_t dds_return_code = writer_->get_offered_deadline_missed_status(offered_deadline_missed);
        rmw_ret_t from_dds = check_dds_ret_code(dds_return_code);
        if (from_dds != RMW_RET_OK) {
          return from_dds;
        }

        rmw_offered_deadline_missed_status_t * rmw_offered_deadline_missed =
          static_cast<rmw_offered_deadline_missed_status_t *>(event);
        rmw_offered_deadline_missed->total_count = offered_deadline_missed.total_count;
        rmw_offered_deadline_missed->total_count_change =
          offered_deadline_missed.total_count_change;

        break;
      }
    case DDS::OFFERED_INCOMPATIBLE_QOS_STATUS:
      {
        DDS::OfferedIncompatibleQosStatus offered_incompatible_qos;
        DDS::ReturnCode_t dds_return_code = writer_->get_offered_incompatible_qos_status(offered_incompatible_qos);
        rmw_ret_t from_dds = check_dds_ret_code(dds_return_code);
        if (RMW_RET_OK != from_dds) {
          return from_dds;
        }

        rmw_offered_qos_incompatible_event_status_t * rmw_offered_qos_incompatible =
          static_cast<rmw_offered_qos_incompatible_event_status_t *>(event);
        rmw_offered_qos_incompatible->total_count = offered_incompatible_qos.total_count;
        rmw_offered_qos_incompatible->total_count_change =
          offered_incompatible_qos.total_count_change;
        rmw_offered_qos_incompatible->last_policy_kind = dds_qos_policy_to_rmw_qos_policy(
          offered_incompatible_qos.last_policy_id);

        break;
      }
    default:
      return RMW_RET_UNSUPPORTED;
  }
  return RMW_RET_OK;
}

DDS::Entity * OpenDDSStaticPublisherInfo::get_entity()
{
  return writer_;
}

void OpenDDSStaticPublisherInfo::cleanup()
{
  if (writer_) {
    writer_ = nullptr;
  }
  if (publisher_) {
    publisher_ = nullptr;
  }
  OpenDDSPublisherListener::Raf::destroy(listener_);
  callbacks_ = nullptr;
}

OpenDDSStaticPublisherInfo::OpenDDSStaticPublisherInfo(DDS::DomainParticipant_var dp
  , const rosidl_message_type_support_t & ros_ts
  , const char * topic_name
  , const rmw_qos_profile_t & rmw_qos
)
  : callbacks_(static_cast<const message_type_support_callbacks_t*>(ros_ts.data))
  , topic_name_(get_topic_str(topic_name, rmw_qos.avoid_ros_namespace_conventions))
  , type_name_(callbacks_ ? _create_type_name(callbacks_) : "")
  , listener_(OpenDDSPublisherListener::Raf::create())
  , publisher_()
  , writer_()
  , publisher_gid_{opendds_identifier, {0}}
{
  try {
    if (!callbacks_) {
      throw std::runtime_error("callbacks_ is null");
    }
    if (topic_name_.empty()) {
      throw std::runtime_error("topic_name_ is empty");
    }
    if (type_name_.empty()) {
      throw std::runtime_error("type_name_ is empty");
    }
    if (!listener_) {
      throw std::runtime_error("OpenDDSPublisherListener failed to contstruct");
    }
    if (!dp) {
      throw std::runtime_error("DomainParticipant is null");
    }
    publisher_ = dp->create_publisher(PUBLISHER_QOS_DEFAULT, listener_, DDS::PUBLICATION_MATCHED_STATUS);
    if (!publisher_) {
      throw std::runtime_error("create_publisher failed");
    }
    OpenDDSStaticSerializedDataTypeSupport_var ts = new OpenDDSStaticSerializedDataTypeSupportImpl();
    if (ts->register_type(dp, type_name_.c_str()) != DDS::RETCODE_OK) {
      throw std::runtime_error("failed to register OpenDDS type");
    }

    // find or create DDS::Topic
    DDS::TopicDescription_var td = dp->lookup_topicdescription(topic_name_.c_str());
    DDS::Topic_var topic = td ? dp->find_topic(topic_name_.c_str(), DDS::Duration_t{0, 0}) :
      dp->create_topic(topic_name_.c_str(), type_name_.c_str(), TOPIC_QOS_DEFAULT, NULL, OpenDDS::DCPS::NO_STATUS_MASK);
    if (!topic) {
      throw std::runtime_error(std::string("failed to ") + (td ? "find" : "create") + " topic");
    }

    DDS::DataWriterQos dw_qos;
    if (!get_datawriter_qos(publisher_.in(), rmw_qos, dw_qos)) {
      throw std::runtime_error("get_datawriter_qos failed");
    }
    writer_ = publisher_->create_datawriter(topic, dw_qos, NULL, OpenDDS::DCPS::NO_STATUS_MASK);
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
    RMW_SET_ERROR_MSG("OpenDDSStaticPublisherInfo construtor failed");
    cleanup();
    throw;
  }
}
