// Copyright 2015 Open Source Robotics Foundation, Inc.
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

#ifndef RMW_OPENDDS_SHARED_CPP__OPENDDSNODE_HPP_
#define RMW_OPENDDS_SHARED_CPP__OPENDDSNODE_HPP_

#include "rmw_opendds_shared_cpp/RmwAllocateFree.hpp"
#include "rmw_opendds_shared_cpp/types.hpp"

#include <dds/DCPS/DomainParticipantImpl.h>

class OpenDDSNode
{
public:
  typedef RmwAllocateFree<OpenDDSNode> Raf;
  static OpenDDSNode * from(const rmw_node_t * node);
  const rmw_guard_condition_t * get_guard_condition() const { return gc_; }
  bool assert_liveliness() const { return dp_->assert_liveliness() == DDS::RETCODE_OK; }
  DDS::DomainParticipant_var dp() { return dp_; }
  void add_pub(const DDS::InstanceHandle_t & pub, const std::string & topic_name, const std::string & type_name);
  void add_sub(const DDS::InstanceHandle_t & sub, const std::string & topic_name, const std::string & type_name);
  bool remove_pub(const DDS::InstanceHandle_t & pub);
  bool remove_sub(const DDS::InstanceHandle_t & sub);
  rmw_ret_t count_publishers(const char * topic_name, size_t * count);
  rmw_ret_t count_subscribers(const char * topic_name, size_t * count);

private:
  friend Raf;
  OpenDDSNode(rmw_context_t & context);
  ~OpenDDSNode() { cleanup(); }
  void cleanup();
  bool configureTransport();

  rmw_context_t & context_;
  rmw_guard_condition_t * gc_;
  CustomPublisherListener * pub_listener_;
  CustomSubscriberListener * sub_listener_;
  DDS::DomainParticipant_var dp_;
  OpenDDS::DCPS::DomainParticipantImpl * dpi_;

  typedef std::mutex Lock;
  typedef std::lock_guard<Lock> Guard;
  mutable Lock lock_;
};

#endif  // RMW_OPENDDS_SHARED_CPP__OPENDDSNODE_HPP_
