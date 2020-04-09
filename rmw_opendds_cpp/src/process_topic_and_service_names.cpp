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

#include "rcutils/format_string.h"
#include "rcutils/types.h"
#include "rcutils/split.h"

#include "rmw/rmw.h"
#include "rmw/allocators.h"
#include "rmw/error_handling.h"

#include "rmw_opendds_shared_cpp/namespace_prefix.hpp"
#include "rmw_opendds_shared_cpp/opendds_include.hpp"

bool
_process_topic_name(
  const char * topic_name,
  bool avoid_ros_namespace_conventions,
  char ** topic_str)
{
  bool success = true;
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  const char * topic_prefix = avoid_ros_namespace_conventions ? "" : ros_topic_prefix;
  char * concat_str = rcutils_format_string(allocator, "%s%s", topic_prefix, topic_name);
  if (concat_str) {
    *topic_str = CORBA::string_dup(concat_str);
    allocator.deallocate(concat_str, allocator.state);
  } else {
    RMW_SET_ERROR_MSG("could not allocate memory for topic string");
    success = false;
  }
  return success;
}

bool
_process_service_name(
  const char * service_name,
  bool avoid_ros_namespace_conventions,
  char ** request_topic_str,
  char ** response_topic_str)
{
  bool success = true;
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  const char * rq_pfx = avoid_ros_namespace_conventions ? "" : ros_service_requester_prefix;
  const char * rp_pfx = avoid_ros_namespace_conventions ? "" : ros_service_response_prefix;

  // concat the ros_service_*_prefix and Request/Reply suffixes with the service_name
  char * rq_str = rcutils_format_string(allocator, "%s%s%s", rq_pfx, service_name, "Request");
  if (rq_str) {
    *request_topic_str = CORBA::string_dup(rq_str);
    allocator.deallocate(rq_str, allocator.state);
  } else {
    RMW_SET_ERROR_MSG("could not allocate memory for request topic string");
    success = false;
  }
  char * rp_str = rcutils_format_string(allocator, "%s%s%s", rp_pfx, service_name, "Reply");
  if (rp_str) {
    *response_topic_str = CORBA::string_dup(rp_str);
    allocator.deallocate(rp_str, allocator.state);
  } else {
    RMW_SET_ERROR_MSG("could not allocate memory for response topic string");
    success = false;
  }
  return success;
}
