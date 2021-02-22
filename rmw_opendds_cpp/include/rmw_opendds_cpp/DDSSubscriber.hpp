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

#ifndef RMW_OPENDDS_CPP__DDSSUBSCRIBER_HPP_
#define RMW_OPENDDS_CPP__DDSSUBSCRIBER_HPP_

#include "rmw_opendds_cpp/DDSEntity.hpp"
#include "rmw_opendds_cpp/RmwAllocateFree.hpp"

#include <atomic>

class OpenDDSSubscriberListener : public DDS::SubscriberListener
{
public:
  typedef RmwAllocateFree<OpenDDSSubscriberListener> Raf;

  virtual void on_subscription_matched(DDS::DataReader *, const DDS::SubscriptionMatchedStatus & status) {
    current_count_ = status.current_count;
  }
  std::size_t current_count() const {
    return current_count_;
  }
  void on_data_available(DDS::DataReader_ptr) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_data_available()\n")));
  }
  void on_requested_deadline_missed(DDS::DataReader_ptr, const DDS::RequestedDeadlineMissedStatus&) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_requested_deadline_missed()\n")));
  }
  void on_requested_incompatible_qos(DDS::DataReader_ptr, const DDS::RequestedIncompatibleQosStatus&) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_requested_incompatible_qos()\n")));
  }
  void on_liveliness_changed(DDS::DataReader_ptr, const DDS::LivelinessChangedStatus&) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_liveliness_changed()\n")));
  }
  void on_sample_rejected(DDS::DataReader_ptr, const DDS::SampleRejectedStatus&) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_sample_rejected()\n")));
  }
  void on_sample_lost(DDS::DataReader_ptr, const DDS::SampleLostStatus&) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_sample_lost()\n")));
  }
  void on_data_on_readers(DDS::Subscriber_ptr) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_data_on_readers()\n")));
  }
private:
  friend Raf;
  OpenDDSSubscriberListener() { current_count_ = 0; }
  ~OpenDDSSubscriberListener() {}
  std::atomic<std::size_t> current_count_;
};

class DDSSubscriber : public DDSEntity
{
public:
  typedef RmwAllocateFree<DDSSubscriber> Raf;
  static DDSSubscriber * from(const rmw_subscription_t * sub);
  rmw_ret_t get_rmw_qos(rmw_qos_profile_t & qos) const;
  rmw_ret_t to_ros_message(const rcutils_uint8_array_t & cdr_stream, void * ros_message);
  DDS::ReadCondition_var read_condition() const { return read_condition_; }
  std::size_t matched_publishers() const { return listener_->current_count(); }
  DDS::InstanceHandle_t instance_handle() const { return subscriber_->get_instance_handle(); }

  // Remap the specific OpenDDS DataReader status to a generic RMW status
  rmw_ret_t get_status(const DDS::StatusMask mask, void * rmw_status) override;
  // Return the reader associated with this subscriber
  DDS::Entity * get() override { return reader_; }
private:
  friend Raf;
  DDSSubscriber(DDS::DomainParticipant_var dp, const rosidl_message_type_support_t & ros_ts,
                const char * topic_name, const rmw_qos_profile_t & rmw_qos);
  ~DDSSubscriber() { cleanup(); }
  void cleanup();

  OpenDDSSubscriberListener * listener_;
  DDS::Subscriber_var subscriber_;
  DDS::DataReader_var reader_;
  DDS::ReadCondition_var read_condition_;
  bool ignore_local_publications;
};

#endif  // RMW_OPENDDS_CPP__DDSSUBSCRIBER_HPP_
