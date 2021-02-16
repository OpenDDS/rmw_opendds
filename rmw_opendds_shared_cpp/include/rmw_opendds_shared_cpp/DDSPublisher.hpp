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

#ifndef RMW_OPENDDS_SHARED_CPP__DDSPUBLISHER_HPP_
#define RMW_OPENDDS_SHARED_CPP__DDSPUBLISHER_HPP_

#include <rmw_opendds_shared_cpp/DDSEntity.hpp>
#include <rmw_opendds_shared_cpp/DDSTopic.hpp>
#include <rmw_opendds_shared_cpp/RmwAllocateFree.hpp>

#include <atomic>

class OpenDDSPublisherListener : public DDS::PublisherListener
{
public:
  typedef RmwAllocateFree<OpenDDSPublisherListener> Raf;

  virtual void on_publication_matched(DDS::DataWriter *, const DDS::PublicationMatchedStatus & status) {
    current_count_ = status.current_count;
  }
  std::size_t current_count() const { return current_count_; }

  void on_offered_deadline_missed(DDS::DataWriter_ptr, const DDS::OfferedDeadlineMissedStatus&) {}
  void on_offered_incompatible_qos(DDS::DataWriter_ptr, const DDS::OfferedIncompatibleQosStatus&) {}
  void on_liveliness_lost(DDS::DataWriter_ptr, const DDS::LivelinessLostStatus&) {}

private:
  friend Raf;
  OpenDDSPublisherListener() { current_count_ = 0; }
  ~OpenDDSPublisherListener() {}
  std::atomic<std::size_t> current_count_;
};

class DDSPublisher : public DDSEntity
{
public:
  typedef RmwAllocateFree<DDSPublisher> Raf;
  static DDSPublisher * from(const rmw_publisher_t * pub);
  const std::string& topic_name() const { return topic_.name(); }
  const std::string& topic_type() const { return topic_.type(); }
  rmw_ret_t get_rmw_qos(rmw_qos_profile_t & qos) const;
  rmw_ret_t to_cdr_stream(const void * ros_message, rcutils_uint8_array_t & cdr_stream);
  DDS::DataWriter_var writer() const { return writer_; }

  std::size_t matched_subscribers() const { return listener_->current_count(); }
  DDS::InstanceHandle_t instance_handle() const { return publisher_->get_instance_handle(); }
  rmw_gid_t gid() const { return publisher_gid_; }

  // Remap the OpenDDS DataWriter status to a generic RMW status
  rmw_ret_t get_status(const DDS::StatusMask mask, void * rmw_status) override;
  // Return the writer associated with this publisher
  DDS::Entity * get_entity() override { return writer_; }
private:
  friend Raf;
  DDSPublisher(DDS::DomainParticipant_var dp, const rosidl_message_type_support_t * ros_ts,
               const char * topic_name, const rmw_qos_profile_t * rmw_qos);
  ~DDSPublisher() { cleanup(); }
  void cleanup();

  DDSTopic topic_;
  OpenDDSPublisherListener * listener_;
  DDS::Publisher_var publisher_;
  DDS::DataWriter_var writer_;
  rmw_gid_t publisher_gid_;
};

#endif  // RMW_OPENDDS_SHARED_CPP__DDSPUBLISHER_HPP_
