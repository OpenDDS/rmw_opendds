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

#ifndef RMW_OPENDDS_SHARED_CPP__SERVICE_HPP_
#define RMW_OPENDDS_SHARED_CPP__SERVICE_HPP_

#include <rosidl_typesupport_opendds_cpp/service_type_support.h>

#include <dds/DdsDcpsDomainC.h>

class Service
{
public:
  Service(const rosidl_service_type_support_t * ts, const char * service_name,
          const rmw_qos_profile_t * rmw_qos, DDS::DomainParticipant_var dp);
  ~Service() { cleanup(); }

  const std::string& name() const { return name_; }

  void * create_requester();
  void * create_replier();
  const char * destroy_requester(void * requester) const;
  const char * destroy_replier(void * replier) const;
  DDS::DataWriter * get_request_writer(void * requester) const;
  DDS::DataReader * get_request_reader(void * replier) const;
  DDS::DataWriter * get_reply_writer(void * replier) const;
  DDS::DataReader * get_reply_reader(void * requester) const;

  int64_t send_request(void * requester, const void * ros_request) const;
  bool take_request(void * replier, rmw_service_info_t * request_header, void * ros_request) const;
  bool send_reply(void * replier, const rmw_request_id_t * request_header, const void * ros_reply) const;
  bool take_reply(void * requester, rmw_service_info_t * request_header, void * ros_reply) const;

private:
  void cleanup();
  const rosidl_service_type_support_t & get_type_support(const rosidl_service_type_support_t * sts) const;
  const service_type_support_callbacks_t * get_callbacks(const rosidl_service_type_support_t * sts) const;
  const std::string create_request_name(const rmw_qos_profile_t * rmw_qos) const;
  const std::string create_reply_name(const rmw_qos_profile_t * rmw_qos) const;

  const service_type_support_callbacks_t * cb_;
  const std::string name_;
  const std::string request_;
  const std::string reply_;
  DDS::DomainParticipant_var dp_;
  DDS::Publisher_var pub_;
  DDS::Subscriber_var sub_;
};

#endif  // RMW_OPENDDS_SHARED_CPP__SERVICE_HPP_
