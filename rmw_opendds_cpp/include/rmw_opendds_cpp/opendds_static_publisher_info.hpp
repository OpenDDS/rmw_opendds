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

#ifndef RMW_OPENDDS_CPP__OPENDDS_STATIC_PUBLISHER_INFO_HPP_
#define RMW_OPENDDS_CPP__OPENDDS_STATIC_PUBLISHER_INFO_HPP_

#include <atomic>

#include "rmw_opendds_shared_cpp/OpenDDSNode.hpp"
#include "rmw_opendds_shared_cpp/types.hpp"
#include "rmw_opendds_shared_cpp/opendds_static_event_info.hpp"
#include "rmw_opendds_shared_cpp/RmwAllocateFree.hpp"

#include "rosidl_typesupport_opendds_cpp/message_type_support.h"

class OpenDDSPublisherListener;

extern "C"
{
struct OpenDDSStaticPublisherInfo : OpenDDSCustomEventInfo
{
  const message_type_support_callbacks_t * callbacks_;
  const std::string topic_name_;
  const std::string type_name_;
  OpenDDSPublisherListener * listener_;
  DDS::Publisher_var publisher_;
  DDS::DataWriter_var writer_;
  rmw_gid_t publisher_gid_;

  typedef RmwAllocateFree<OpenDDSStaticPublisherInfo> Raf;
  static OpenDDSStaticPublisherInfo* get_from(const rmw_publisher_t * publisher);

  // Remap the OpenDDS DataWriter Status to a generic RMW status type
  rmw_ret_t get_status(DDS::StatusMask mask, void* event) override;

  // return the topic writer associated with this publisher
  DDS::Entity* get_entity() override;
private:
  friend Raf;
  OpenDDSStaticPublisherInfo(DDS::DomainParticipant_var dp, const rosidl_message_type_support_t& ros_ts,
                             const char * topic_name, const rmw_qos_profile_t& rmw_qos);
  ~OpenDDSStaticPublisherInfo(){ cleanup(); }
  void cleanup();
};
}  // extern "C"

class OpenDDSPublisherListener : public DDS::PublisherListener
{
public:
  typedef RmwAllocateFree<OpenDDSPublisherListener> Raf;

  virtual void on_publication_matched(DDS::DataWriter *, const DDS::PublicationMatchedStatus & status)
  {
    current_count_ = status.current_count;
  }

  std::size_t current_count() const
  {
    return current_count_;
  }

  void on_offered_deadline_missed(DDS::DataWriter_ptr, const DDS::OfferedDeadlineMissedStatus&) {}
  void on_offered_incompatible_qos(DDS::DataWriter_ptr, const DDS::OfferedIncompatibleQosStatus&) {}
  void on_liveliness_lost(DDS::DataWriter_ptr, const DDS::LivelinessLostStatus&) {}

private:
  friend Raf;
  OpenDDSPublisherListener(){ current_count_ = 0; }
  ~OpenDDSPublisherListener(){}
  std::atomic<std::size_t> current_count_;
};

#endif  // RMW_OPENDDS_CPP__OPENDDS_STATIC_PUBLISHER_INFO_HPP_
