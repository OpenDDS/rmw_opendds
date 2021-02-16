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
#include <set>
#include <string>

#include "rcutils/logging_macros.h"
#include "rcutils/strdup.h"

#include "rmw/convert_rcutils_ret_to_rmw_ret.h"
#include "rmw/error_handling.h"

#include "rmw_opendds_shared_cpp/demangle.hpp"
#include "rmw_opendds_shared_cpp/namespace_prefix.hpp"
#include "rmw_opendds_shared_cpp/names_and_types_helpers.hpp"
#include "rmw_opendds_shared_cpp/opendds_include.hpp"
#include "rmw_opendds_shared_cpp/topic_names_and_types.hpp"
#include <rmw_opendds_shared_cpp/OpenDDSNode.hpp>

rmw_ret_t
get_topic_names_and_types(
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  bool no_demangle,
  rmw_names_and_types_t * topic_names_and_types)
{
  auto dds_node = OpenDDSNode::from(node);
  if (!dds_node) {
    return RMW_RET_ERROR;
  }
  rmw_ret_t rmw_ret = rmw_names_and_types_check_zero(topic_names_and_types);
  if (rmw_ret != RMW_RET_OK) {
    return rmw_ret;
  }

  // combine publisher and subscriber information
  std::map<std::string, std::set<std::string>> topics;
  dds_node->pub_listener()->fill_topic_names_and_types(no_demangle, topics);
  dds_node->sub_listener()->fill_topic_names_and_types(no_demangle, topics);

  // Copy data to results handle
  if (!topics.empty()) {
    rmw_ret_t rmw_ret = copy_topics_names_and_types(topics, allocator, no_demangle, topic_names_and_types);
    if (rmw_ret != RMW_RET_OK) {
      return rmw_ret;
    }
  }

  return RMW_RET_OK;
}
