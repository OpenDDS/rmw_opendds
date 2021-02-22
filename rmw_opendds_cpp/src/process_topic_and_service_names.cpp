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

#include "process_topic_and_service_names.hpp"
#include "rcutils/format_string.h"
#include "rcutils/types.h"
#include "rcutils/split.h"

#include "rmw/rmw.h"
#include "rmw/allocators.h"
#include "rmw/error_handling.h"

#include "rmw_opendds_cpp/namespace_prefix.hpp"
#include "rmw_opendds_cpp/opendds_include.hpp"

std::string
get_topic_str(
  const char * topic_name,
  bool avoid_ros_namespace_conventions)
{
  if (!topic_name || strlen(topic_name) == 0) {
    return "";
  }
  if (avoid_ros_namespace_conventions) {
    return topic_name;
  }
  return std::string(ros_topic_prefix) + topic_name;
}

void
get_service_topic_names(
  const std::string& service_name,
  bool avoid_ros_namespace_conventions,
  std::string& request_topic,
  std::string& response_topic)
{
  request_topic = (avoid_ros_namespace_conventions ? "" : ros_service_requester_prefix) + service_name + "Request";
  response_topic = (avoid_ros_namespace_conventions ? "" : ros_service_response_prefix) + service_name + "Reply";
}
