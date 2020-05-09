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

#ifndef PROCESS_TOPIC_AND_SERVICE_NAMES_HPP_
#define PROCESS_TOPIC_AND_SERVICE_NAMES_HPP_

void
get_topic_name(
  const std::string topic_name,
  bool avoid_ros_namespace_conventions,
  std::string& topic_str);

void
get_service_topic_names(
  const std::string service_name,
  bool avoid_ros_namespace_conventions,
  std::string& request_topic,
  std::string& response_topic);

#endif  // PROCESS_TOPIC_AND_SERVICE_NAMES_HPP_
