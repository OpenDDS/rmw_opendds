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

#include "rmw_opendds_shared_cpp/OpenDDSNode.hpp"
#include "rmw_opendds_shared_cpp/node.hpp"
#include "rmw_opendds_shared_cpp/identifier.hpp"

#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/rmw.h"

extern "C"
{

rmw_node_t *
rmw_create_node(
  rmw_context_t * context,
  const char * name,
  const char * namespace_,
  size_t domain_id,
  bool localhost_only)
{
  ACE_UNUSED_ARG(domain_id);
  ACE_UNUSED_ARG(localhost_only);
  RMW_CHECK_FOR_NULL_WITH_MSG(context, "context is null", NULL);
  if (!check_impl_id(context->implementation_identifier)) {
    return NULL;
  }
  RMW_CHECK_FOR_NULL_WITH_MSG(context->impl, "context->impl is null", NULL);
  RMW_CHECK_FOR_NULL_WITH_MSG(name, "node name is null", NULL);
  RMW_CHECK_FOR_NULL_WITH_MSG(namespace_, "node namespace_ is null", NULL);
  return create_node(*context, name, namespace_);
}

rmw_ret_t
rmw_destroy_node(rmw_node_t * node)
{
  RMW_CHECK_FOR_NULL_WITH_MSG(node, "node is null", RMW_RET_INVALID_ARGUMENT);
  if (!check_impl_id(node->implementation_identifier)) {
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION;
  }
  return destroy_node(node);
}

rmw_ret_t
rmw_node_assert_liveliness(const rmw_node_t * node)
{
  auto dds_node = OpenDDSNode::from(node);
  return dds_node && dds_node->assert_liveliness() ? RMW_RET_OK : RMW_RET_ERROR;
}

const rmw_guard_condition_t *
rmw_node_get_graph_guard_condition(const rmw_node_t * node)
{
  auto dds_node = OpenDDSNode::from(node);
  return dds_node ? dds_node->get_guard_condition() : nullptr;
}

rmw_ret_t
rmw_count_publishers(
  const rmw_node_t * node,
  const char * topic_name,
  size_t * count)
{
  auto dds_node = OpenDDSNode::from(node);
  return dds_node ? dds_node->count_publishers(topic_name, count) : RMW_RET_ERROR;
}

rmw_ret_t
rmw_count_subscribers(
  const rmw_node_t * node,
  const char * topic_name,
  size_t * count)
{
  auto dds_node = OpenDDSNode::from(node);
  return dds_node ? dds_node->count_subscribers(topic_name, count) : RMW_RET_ERROR;
}

rmw_ret_t
rmw_get_topic_names_and_types(
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  bool no_demangle,
  rmw_names_and_types_t * topic_names_and_types)
{
  auto dds_node = OpenDDSNode::from(node);
  return dds_node ? dds_node->get_topic_names_types(topic_names_and_types, no_demangle, allocator) : RMW_RET_ERROR;
}

rmw_ret_t
rmw_get_service_names_and_types(
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  rmw_names_and_types_t * service_names_and_types)
{
  auto dds_node = OpenDDSNode::from(node);
  return dds_node ? dds_node->get_service_names_types(service_names_and_types, allocator) : RMW_RET_ERROR;
}

}  // extern "C"
