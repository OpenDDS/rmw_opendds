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

#include "rmw_opendds_cpp/DDSSubscriber.hpp"
#include "opendds_static_serialized_dataTypeSupportImpl.h"

#include "rmw_opendds_cpp/event_converter.hpp"
#include "rmw_opendds_cpp/identifier.hpp"
#include "rmw_opendds_cpp/qos.hpp"

#include <dds/DCPS/DataReaderImpl_T.h>

#include "rmw/visibility_control.h"
#include "rmw/incompatible_qos_events_statuses.h"

DDSSubscriber * DDSSubscriber::from(const rmw_subscription_t * sub)
{
  if (!sub) {
    RMW_SET_ERROR_MSG("rmw_subscription is null");
    return nullptr;
  }
  if (!check_impl_id(sub->implementation_identifier)) {
    return nullptr;
  }
  auto dds_sub = static_cast<DDSSubscriber *>(sub->data);
  if (!dds_sub) {
    RMW_SET_ERROR_MSG("dds_sub is null");
    return nullptr;
  }
  return dds_sub;
}

rmw_ret_t DDSSubscriber::get_rmw_qos(rmw_qos_profile_t & qos) const
{
  DDS::DataReaderQos dds_qos;
  if (reader_->get_qos(dds_qos) == DDS::RETCODE_OK) {
    dds_qos_to_rmw_qos(dds_qos, qos);
    return RMW_RET_OK;
  } else {
    RMW_SET_ERROR_MSG("get_rmw_qos failed");
  }
  return RMW_RET_ERROR;
}

rmw_ret_t DDSSubscriber::to_ros_message(const rcutils_uint8_array_t & cdr_stream, void * ros_message)
{
  if (callbacks_->to_message(&cdr_stream, ros_message)) {
    return RMW_RET_OK;
  }
  RMW_SET_ERROR_MSG("to_ros_message failed");
  return RMW_RET_ERROR;
}

rmw_ret_t DDSSubscriber::get_status(const DDS::StatusMask mask, void * rmw_status)
{
  switch (mask) {
    case DDS::LIVELINESS_CHANGED_STATUS:
    {
      DDS::LivelinessChangedStatus llc;
      rmw_ret_t ret = check_dds_ret_code(reader_->get_liveliness_changed_status(llc));
      if (ret != RMW_RET_OK) {
        return ret;
      }
      auto rmw_llc = static_cast<rmw_liveliness_changed_status_t *>(rmw_status);
      rmw_llc->alive_count = llc.alive_count;
      rmw_llc->not_alive_count = llc.not_alive_count;
      rmw_llc->alive_count_change = llc.alive_count_change;
      rmw_llc->not_alive_count_change = llc.not_alive_count_change;
      break;
    }
    case DDS::REQUESTED_DEADLINE_MISSED_STATUS:
    {
      DDS::RequestedDeadlineMissedStatus rdm;
      rmw_ret_t ret = check_dds_ret_code(reader_->get_requested_deadline_missed_status(rdm));
      if (ret != RMW_RET_OK) {
        return ret;
      }
      auto rmw_rdm = static_cast<rmw_requested_deadline_missed_status_t *>(rmw_status);
      rmw_rdm->total_count = rdm.total_count;
      rmw_rdm->total_count_change = rdm.total_count_change;
      break;
    }
    case DDS::REQUESTED_INCOMPATIBLE_QOS_STATUS:
    {
      DDS::RequestedIncompatibleQosStatus ric;
      rmw_ret_t ret = check_dds_ret_code(reader_->get_requested_incompatible_qos_status(ric));
      if (ret != RMW_RET_OK) {
        return ret;
      }
      auto rmw_ric = static_cast<rmw_requested_qos_incompatible_event_status_t *>(rmw_status);
      rmw_ric->total_count = ric.total_count;
      rmw_ric->total_count_change = ric.total_count_change;
      rmw_ric->last_policy_kind = dds_qos_policy_to_rmw_qos_policy(ric.last_policy_id);
      break;
    }
    default:
      return RMW_RET_UNSUPPORTED;
  }
  return RMW_RET_OK;
}

void DDSSubscriber::cleanup()
{
  if (reader_) {
    reader_ = nullptr;
  }
  if (subscriber_) {
    subscriber_ = nullptr;
  }
  OpenDDSSubscriberListener::Raf::destroy(listener_);
}

DDSSubscriber::DDSSubscriber(DDS::DomainParticipant_var dp
  , const rosidl_message_type_support_t & ros_ts
  , const char * topic_name
  , const rmw_qos_profile_t & rmw_qos
) : DDSEntity(ros_ts, topic_name, rmw_qos)
  , listener_(OpenDDSSubscriberListener::Raf::create())
  , subscriber_()
  , reader_()
  , read_condition_()
  , ignore_local_publications(false)
{
  try {
    if (!listener_) {
      throw std::runtime_error("OpenDDSSubscriberListener failed to contstruct");
    }
    if (!dp) {
      throw std::runtime_error("DomainParticipant is null");
    }
    subscriber_ = dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT, listener_, DDS::SUBSCRIPTION_MATCHED_STATUS);
    if (!subscriber_) {
      throw std::runtime_error("create_subscriber failed");
    }
    if (!register_type(dp)) {
      throw std::runtime_error("failed to register OpenDDS type");
    }

    DDS::Topic_var topic = find_or_create_topic(dp);
    if (!topic) {
      throw std::runtime_error("find_or_create_topic failed");
    }
    DDS::DataReaderQos dr_qos;
    if (!get_datareader_qos(subscriber_.in(), rmw_qos, dr_qos)) {
      throw std::runtime_error("get_datareader_qos failed");
    }
    reader_ = subscriber_->create_datareader(topic, dr_qos, NULL, OpenDDS::DCPS::NO_STATUS_MASK);
    if (!reader_) {
      throw std::runtime_error("create_datawriter failed");
    }

    auto rdi = dynamic_cast<OpenDDS::DCPS::DataReaderImpl_T<OpenDDSStaticSerializedData>*>(reader_.in());
    rdi->set_marshal_skip_serialize(true);

    read_condition_ = reader_->create_readcondition(DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
    if (!read_condition_) {
      throw std::runtime_error("create_readcondition failed");
    }
  } catch (const std::exception& e) {
    RMW_SET_ERROR_MSG(e.what());
    cleanup();
    throw;
  } catch (...) {
    RMW_SET_ERROR_MSG("DDSSubscriber constructor failed");
    cleanup();
    throw;
  }
}
