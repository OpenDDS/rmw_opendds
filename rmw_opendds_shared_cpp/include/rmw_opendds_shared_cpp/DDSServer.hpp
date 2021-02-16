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

#ifndef RMW_OPENDDS_SHARED_CPP__DDSSERVER_HPP_
#define RMW_OPENDDS_SHARED_CPP__DDSSERVER_HPP_

#include <rmw_opendds_shared_cpp/Service.hpp>
#include <rmw_opendds_shared_cpp/OpenDDSNode.hpp>
#include <rmw_opendds_shared_cpp/RmwAllocateFree.hpp>

#include <dds/DCPS/GuardCondition.h>

class DDSServer
{
public:
  typedef RmwAllocateFree<DDSServer> Raf;
  static DDSServer * from(const rmw_service_t * service);
  bool take(rmw_service_info_t * request_header, void * ros_request) const;
  bool send(const rmw_request_id_t * request_header, const void * ros_reply) const;
  void add_to(OpenDDSNode * dds_node);
  bool remove_from(OpenDDSNode * dds_node);
  const std::string& service_name() const { return service_.name(); }
  void * replier() const { return replier_; }
  DDS::ReadCondition_var read_condition() const { return read_condition_; }

private:
  friend Raf;
  DDSServer(const rosidl_service_type_support_t * ts, const char * service_name,
            const rmw_qos_profile_t * rmw_qos, DDS::DomainParticipant_var dp);
  ~DDSServer() { cleanup(); }
  void cleanup();

  Service service_;
  void * replier_;
  DDS::DataReader_var reader_;
  DDS::ReadCondition_var read_condition_;
  DDS::DataWriter_var writer_;
};

#endif  // RMW_OPENDDS_SHARED_CPP__DDSSERVER_HPP_
