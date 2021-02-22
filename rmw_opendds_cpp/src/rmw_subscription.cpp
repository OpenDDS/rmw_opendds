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

#include <rmw_opendds_shared_cpp/DDSSubscriber.hpp>
#include <rmw_opendds_shared_cpp/OpenDDSNode.hpp>
#include <rmw_opendds_shared_cpp/identifier.hpp>
#include <rmw_opendds_shared_cpp/qos.hpp>
#include <rmw_opendds_shared_cpp/types.hpp>

// Uncomment this to get extra console output about discovery.
// This affects code in this file, but there is a similar variable in:
//   rmw_opendds_cpp/shared_functions.cpp
// #define DISCOVERY_DEBUG_LOGGING 1

#include <dds/DCPS/DataReaderImpl_T.h>
#include <dds/DCPS/DomainParticipantImpl.h>
#include <dds/DCPS/Marked_Default_Qos.h>

#include <rmw/allocators.h>
#include <rmw/error_handling.h>
#include <rmw/impl/cpp/macros.hpp>
#include <rmw/rmw.h>

#include <string>

extern "C"
{
rmw_ret_t
rmw_init_subscription_allocation(
  const rosidl_message_type_support_t * type_support,
  const rosidl_runtime_c__Sequence__bound * message_bounds,
  rmw_subscription_allocation_t * allocation)
{
  // Unused in current implementation.
  (void) type_support;
  (void) message_bounds;
  (void) allocation;
  RMW_SET_ERROR_MSG("unimplemented");
  return RMW_RET_ERROR;
}

rmw_ret_t
rmw_fini_subscription_allocation(rmw_subscription_allocation_t * allocation)
{
  // Unused in current implementation.
  (void) allocation;
  RMW_SET_ERROR_MSG("unimplemented");
  return RMW_RET_ERROR;
}

void clean_subscription(rmw_subscription_t * subscription)
{
  if (subscription) {
    auto dds_subscriber = static_cast<DDSSubscriber*>(subscription->data);
    if (dds_subscriber) {
      DDSSubscriber::Raf::destroy(dds_subscriber);
      subscription->data = nullptr;
    }
    rmw_subscription_free(subscription);
  }
}

rmw_subscription_t * create_initial_subscription(const rmw_subscription_options_t * subscription_options)
{
  rmw_subscription_t * subscription = rmw_subscription_allocate();
  if (!subscription) {
    throw std::runtime_error("failed to allocate subscription");
  }
  subscription->implementation_identifier = opendds_identifier;
  subscription->data = nullptr;
  subscription->topic_name = nullptr;
  if (subscription_options) {
    subscription->options = *subscription_options;
  } else {
    subscription->options.rmw_specific_subscription_payload = nullptr;
    subscription->options.ignore_local_publications = false;
  }
  subscription->can_loan_messages = false;
  return subscription;
}

rmw_subscription_t *
rmw_create_subscription(
  const rmw_node_t * node,
  const rosidl_message_type_support_t * type_supports,
  const char * topic_name,
  const rmw_qos_profile_t * rmw_qos,
  const rmw_subscription_options_t * subscription_options)
{
  auto dds_node = OpenDDSNode::from(node);
  if (!dds_node) {
    return nullptr; // error set
  }
  rmw_subscription_t * subscription = nullptr;
  try {
    subscription = create_initial_subscription(subscription_options);
    auto dds_sub = DDSSubscriber::Raf::create(dds_node->dp(), type_supports, topic_name, rmw_qos);
    if (!dds_sub) {
      throw std::runtime_error("DDSSubscriber failed");
    }
    subscription->data = dds_sub;
    subscription->topic_name = dds_sub->topic_name().c_str();
    dds_node->add_sub(dds_sub->instance_handle(), dds_sub->topic_name(), dds_sub->topic_type());
    return subscription;
  } catch (const std::exception& e) {
    RMW_SET_ERROR_MSG(e.what());
  } catch (...) {
    RMW_SET_ERROR_MSG("rmw_create_subscription failed");
  }
  clean_subscription(subscription);
  return nullptr;
}

rmw_ret_t
rmw_subscription_count_matched_publishers(
  const rmw_subscription_t * subscription,
  size_t * publisher_count)
{
  if (!publisher_count) {
    RMW_SET_ERROR_MSG("publisher_count is null");
    return RMW_RET_INVALID_ARGUMENT;
  }
  auto dds_sub = DDSSubscriber::from(subscription);
  if (dds_sub) {
    *publisher_count = dds_sub->matched_publishers();
    return RMW_RET_OK;
  }
  return RMW_RET_ERROR;
}

rmw_ret_t
rmw_subscription_get_actual_qos(
  const rmw_subscription_t * subscription,
  rmw_qos_profile_t * qos)
{
  if (!qos) {
    RMW_SET_ERROR_MSG("qos is null");
    return RMW_RET_INVALID_ARGUMENT;
  }
  auto dds_sub = DDSSubscriber::from(subscription);
  return dds_sub ? dds_sub->get_rmw_qos(*qos) : RMW_RET_ERROR;
}

rmw_ret_t
rmw_destroy_subscription(rmw_node_t * node, rmw_subscription_t * subscription)
{
  auto dds_node = OpenDDSNode::from(node);
  if (!dds_node) {
    return RMW_RET_ERROR; // error set
  }
  if (!dds_node->dp()) {
    RMW_SET_ERROR_MSG("DomainParticipant is null");
    return RMW_RET_ERROR;
  }

  auto dds_sub = DDSSubscriber::from(subscription);
  if (!dds_sub) {
    return RMW_RET_ERROR; // error set
  }
  bool ret = dds_node->remove_sub(dds_sub->instance_handle());
  if (ret) {
    clean_subscription(subscription);
  }
  return ret ? RMW_RET_OK : RMW_RET_ERROR;
}
}  // extern "C"
