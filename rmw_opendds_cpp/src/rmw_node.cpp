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

#include <rmw_opendds_cpp/OpenDDSNode.hpp>
#include <rmw_opendds_cpp/init.hpp>
#include <rmw_opendds_cpp/identifier.hpp>

#include <rmw/error_handling.h>
#include <rmw/get_node_info_and_types.h>
#include <rmw/get_service_names_and_types.h>
#include <rmw/get_topic_names_and_types.h>
#include <rmw/names_and_types.h>
#include <rmw/sanity_checks.h>
#include <rmw/impl/cpp/macros.hpp>
#include <rmw/rmw.h>

extern "C"
{

void clean_node(rmw_node_t * node)
{
  if (node) {
    node->namespace_ = nullptr;
    node->name = nullptr;
    auto dds_node = static_cast<OpenDDSNode*>(node->data);
    if (dds_node) {
      OpenDDSNode::Raf::destroy(dds_node);
      node->data = nullptr;
    }
    node->implementation_identifier = nullptr;
    node->context = nullptr;
    rmw_node_free(node);
  }
}

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
  RMW_CHECK_FOR_NULL_WITH_MSG(context, "context is null", nullptr);
  if (!check_impl_id(context->implementation_identifier)) {
    return nullptr;
  }
  RMW_CHECK_FOR_NULL_WITH_MSG(context->impl, "context->impl is null", nullptr);
  if (!OpenDDSNode::validate_name_namespace(name, namespace_)) {
    return nullptr;
  }

  rmw_node_t * node = nullptr;
  try {
    node = rmw_node_allocate();
    if (!node) {
      throw std::runtime_error("rmw_node_allocate failed");
    }
    node->context = context;
    node->implementation_identifier = opendds_identifier;
    node->data = nullptr;
    auto dds_node = OpenDDSNode::Raf::create(*context, name, namespace_);
    if (!dds_node) {
      throw std::runtime_error("OpenDDSNode failed");
    }
    node->data = dds_node;
    node->name = dds_node->name().c_str();
    node->namespace_ = dds_node->name_space().c_str();
    return node;
  } catch (const std::exception& e) {
    RMW_SET_ERROR_MSG(e.what());
  } catch (...) {
    RMW_SET_ERROR_MSG("create_node unkown exception");
  }
  clean_node(node);
  return nullptr;
}

rmw_ret_t
rmw_destroy_node(rmw_node_t * node)
{
  RMW_CHECK_FOR_NULL_WITH_MSG(node, "node is null", RMW_RET_INVALID_ARGUMENT);
  if (!check_impl_id(node->implementation_identifier)) {
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION;
  }
  if (node && node->context && node->context->impl) {
    clean_node(node);
    return RMW_RET_OK;
  }
  return RMW_RET_ERROR;
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
  return dds_node ? dds_node->count_publishers(topic_name, count) : RMW_RET_INVALID_ARGUMENT;
}

rmw_ret_t
rmw_count_subscribers(
  const rmw_node_t * node,
  const char * topic_name,
  size_t * count)
{
  auto dds_node = OpenDDSNode::from(node);
  return dds_node ? dds_node->count_subscribers(topic_name, count) : RMW_RET_INVALID_ARGUMENT;
}

rmw_ret_t
rmw_get_topic_names_and_types(
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  bool no_demangle,
  rmw_names_and_types_t * topic_names_and_types)
{
  auto dds_node = OpenDDSNode::from(node);
  return dds_node ? dds_node->get_topic_names_types(topic_names_and_types, no_demangle, allocator) : RMW_RET_INVALID_ARGUMENT;
}

rmw_ret_t
rmw_get_service_names_and_types(
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  rmw_names_and_types_t * service_names_and_types)
{
  auto dds_node = OpenDDSNode::from(node);
  return dds_node ? dds_node->get_service_names_types(service_names_and_types, allocator) : RMW_RET_INVALID_ARGUMENT;
}

rmw_ret_t
rmw_get_node_names(
  const rmw_node_t * node,
  rcutils_string_array_t * node_names,
  rcutils_string_array_t * node_namespaces)
{
  auto dds_node = OpenDDSNode::from(node);
  return dds_node ? dds_node->get_names(node_names, node_namespaces, nullptr) : RMW_RET_INVALID_ARGUMENT;
}

rmw_ret_t
rmw_get_node_names_with_enclaves(
  const rmw_node_t* node,
  rcutils_string_array_t* node_names,
  rcutils_string_array_t* node_namespaces,
  rcutils_string_array_t* enclaves)
{
  if (rmw_check_zero_rmw_string_array(enclaves) != RMW_RET_OK) {
    return RMW_RET_INVALID_ARGUMENT;
  }
  auto dds_node = OpenDDSNode::from(node);
  return dds_node ? dds_node->get_names(node_names, node_namespaces, enclaves) : RMW_RET_INVALID_ARGUMENT;
}

rmw_ret_t
rmw_get_publisher_names_and_types_by_node(
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * node_name,
  const char * node_namespace,
  bool no_demangle,
  rmw_names_and_types_t * topic_names_and_types)
{
  auto dds_node = OpenDDSNode::from(node);
  return dds_node ? dds_node->get_pub_names_types(topic_names_and_types, node_name, node_namespace, no_demangle, allocator) : RMW_RET_INVALID_ARGUMENT;
}

rmw_ret_t
rmw_get_subscriber_names_and_types_by_node(
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * node_name,
  const char * node_namespace,
  bool no_demangle,
  rmw_names_and_types_t * topic_names_and_types)
{
  auto dds_node = OpenDDSNode::from(node);
  return dds_node ? dds_node->get_sub_names_types(topic_names_and_types, node_name, node_namespace, no_demangle, allocator) : RMW_RET_INVALID_ARGUMENT;
}

rmw_ret_t
rmw_get_service_names_and_types_by_node(
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * node_name,
  const char * node_namespace,
  rmw_names_and_types_t * service_names_and_types)
{
  auto dds_node = OpenDDSNode::from(node);
  return dds_node ? dds_node->get_service_names_types(service_names_and_types, node_name, node_namespace, "Request", allocator) : RMW_RET_INVALID_ARGUMENT;
}

rmw_ret_t
rmw_get_client_names_and_types_by_node(
  const rmw_node_t * node,
  rcutils_allocator_t * allocator,
  const char * node_name,
  const char * node_namespace,
  rmw_names_and_types_t * service_names_and_types)
{
  auto dds_node = OpenDDSNode::from(node);
  return dds_node ? dds_node->get_service_names_types(service_names_and_types, node_name, node_namespace, "Reply", allocator) : RMW_RET_INVALID_ARGUMENT;
}

}  // extern "C"
