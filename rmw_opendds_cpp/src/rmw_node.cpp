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

#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/rmw.h"

#include "rmw_opendds_shared_cpp/node.hpp"

#include "rmw_opendds_cpp/identifier.hpp"

OpenDDSNode* get_dds_node(const rmw_node_t * node)
{
  RMW_CHECK_FOR_NULL_WITH_MSG(node, "node is null", return nullptr);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(node, node->implementation_identifier, opendds_identifier, return nullptr)
  auto dds_node = static_cast<OpenDDSNode *>(node->data);
  RMW_CHECK_FOR_NULL_WITH_MSG(dds_node, "dds_node is null", return nullptr);
  return dds_node;
}

extern "C"
{
rmw_node_t *
rmw_create_node(
  rmw_context_t * context,
  const char * name,
  const char * namespace_,
  size_t domain_id,    //?? removed
  bool localhost_only) //?? removed
{
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(context, NULL);
/*
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    init context,
    context->implementation_identifier,
    opendds_identifier,
    // TODO(wjwwood): replace this with RMW_RET_INCORRECT_RMW_IMPLEMENTATION when refactored
    return NULL);
  return create_node(opendds_identifier, name, namespace_, domain_id);
*/
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(name, NULL);
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(namespace_, NULL);
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(context->impl, NULL);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(context, context->implementation_identifier, opendds_identifier, return NULL);
  std::cout << "rmw_create_node context:" << context << '\n'; //??
  return create_node(*context, name, namespace_);
}

rmw_ret_t
rmw_destroy_node(rmw_node_t * node)
{
//return destroy_node(opendds_identifier, node);
  RMW_CHECK_ARGUMENT_FOR_NULL(node, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(node->implementation_identifier, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(node, node->implementation_identifier,
    opendds_identifier, return RMW_RET_INCORRECT_RMW_IMPLEMENTATION)
  return destroy_node(node);
}

rmw_ret_t
rmw_node_assert_liveliness(const rmw_node_t * node)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(node, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(node->implementation_identifier, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node handle,
    node->implementation_identifier,
    opendds_identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION)
  return node_assert_liveliness(node);
}

const rmw_guard_condition_t *
rmw_node_get_graph_guard_condition(const rmw_node_t * node)
{
  if (!node) {
    RMW_SET_ERROR_MSG("node handle is null");
    return nullptr;
  }
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node handle,
    node->implementation_identifier, opendds_identifier,
    return nullptr)

  return node_get_graph_guard_condition(node);
}
}  // extern "C"
