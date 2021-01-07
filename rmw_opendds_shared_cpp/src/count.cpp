// Copyright 2015-2017 Open Source Robotics Foundation, Inc.
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

#include <map>

#include "rmw_opendds_shared_cpp/count.hpp"
#include "rmw_opendds_shared_cpp/demangle.hpp"
#include "rmw_opendds_shared_cpp/types.hpp"

#include "rmw/error_handling.h"

rmw_ret_t
count_publishers(
  const char * implementation_identifier,
  const rmw_node_t * node,
  const char * topic_name,
  size_t * count)
{
  OpenDDSNode* dds_node = OpenDDSNode::get_from(node, implementation_identifier);
  if (!dds_node) {
    return RMW_RET_ERROR;
  }
  if (!dds_node->pub_listener_) {
    RMW_SET_ERROR_MSG("publisher listener is null");
    return RMW_RET_ERROR;
  }
  if (!topic_name) {
    RMW_SET_ERROR_MSG("topic name is null");
    return RMW_RET_ERROR;
  }
  if (!count) {
    RMW_SET_ERROR_MSG("count handle is null");
    return RMW_RET_ERROR;
  }
  *count = dds_node->pub_listener_->count_topic(topic_name);
  return RMW_RET_OK;
}

rmw_ret_t
count_subscribers(
  const char * implementation_identifier,
  const rmw_node_t * node,
  const char * topic_name,
  size_t * count)
{
  OpenDDSNode* dds_node = OpenDDSNode::get_from(node, implementation_identifier);
  if (!dds_node) {
    return RMW_RET_ERROR;
  }
  if (!dds_node->sub_listener_) {
    RMW_SET_ERROR_MSG("subscriber listener is null");
    return RMW_RET_ERROR;
  }
  if (!topic_name) {
    RMW_SET_ERROR_MSG("topic name is null");
    return RMW_RET_ERROR;
  }
  if (!count) {
    RMW_SET_ERROR_MSG("count handle is null");
    return RMW_RET_ERROR;
  }
  *count = dds_node->sub_listener_->count_topic(topic_name);
  return RMW_RET_OK;
}
