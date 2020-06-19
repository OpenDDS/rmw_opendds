// Copyright 2014-2017 Open Source Robotics Foundation, Inc.
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

#ifndef RMW_OPENDDS_CPP__OPENDDS_STATIC_SUBSCRIBER_INFO_HPP_
#define RMW_OPENDDS_CPP__OPENDDS_STATIC_SUBSCRIBER_INFO_HPP_

#include <atomic>

#include "rmw_opendds_shared_cpp/opendds_include.hpp"
#include "rmw_opendds_shared_cpp/opendds_static_event_info.hpp"

#include "rosidl_typesupport_opendds_cpp/message_type_support.h"

class OpenDDSSubscriberListener;

extern "C"
{
struct OpenDDSStaticSubscriberInfo : OpenDDSCustomEventInfo
{
  DDS::Subscriber_var dds_subscriber_;
  OpenDDSSubscriberListener * listener_;
  DDS::DataReader_var topic_reader_;
  DDS::ReadCondition_var read_condition_;
  bool ignore_local_publications;
  const message_type_support_callbacks_t * callbacks_;
  OpenDDSStaticSubscriberInfo() :
    dds_subscriber_(),
    listener_(nullptr),
    topic_reader_(),
    read_condition_(),
    ignore_local_publications(false),
    callbacks_(nullptr) {}

  /// Remap the specific OpenDDS DataReader Status to a generic RMW status type.
  /**
   * \param mask input status mask
   * \param event
   */
  rmw_ret_t get_status(DDS::StatusMask mask, void* event) override;
  /// Return the topic reader entity for this subscriber.
  /**
   * \return the topic reader associated with this subscriber
   */
  DDS::Entity* get_entity() override;

};
}  // extern "C"

class OpenDDSSubscriberListener : public DDS::SubscriberListener
{
public:
  virtual void on_subscription_matched(DDS::DataReader *, const DDS::SubscriptionMatchedStatus & status)
  {
    current_count_ = status.current_count;
  }

  std::size_t current_count() const
  {
    return current_count_;
  }

  void on_data_available(DDS::DataReader_ptr)
  {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_data_available()\n")));
  }

  void on_requested_deadline_missed(DDS::DataReader_ptr, const DDS::RequestedDeadlineMissedStatus&)
  {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_requested_deadline_missed()\n")));
  }

  void on_requested_incompatible_qos(DDS::DataReader_ptr, const DDS::RequestedIncompatibleQosStatus&)
  {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_requested_incompatible_qos()\n")));
  }

  void on_liveliness_changed(DDS::DataReader_ptr, const DDS::LivelinessChangedStatus&)
  {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_liveliness_changed()\n")));
  }

  void on_sample_rejected(DDS::DataReader_ptr, const DDS::SampleRejectedStatus&)
  {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_sample_rejected()\n")));
  }

  void on_sample_lost(DDS::DataReader_ptr, const DDS::SampleLostStatus&)
  {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_sample_lost()\n")));
  }

  void on_data_on_readers(DDS::Subscriber_ptr)
  {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_data_on_readers()\n")));
  }

private:
  std::atomic<std::size_t> current_count_;
};

#endif  // RMW_OPENDDS_CPP__OPENDDS_STATIC_SUBSCRIBER_INFO_HPP_
