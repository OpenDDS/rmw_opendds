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

#include "rmw_opendds_cpp/DDSPublisher.hpp"
#include "process_topic_and_service_names.hpp"
#include "type_support_common.hpp"
#include "opendds_static_serialized_dataTypeSupportImpl.h"

#include "rmw_opendds_shared_cpp/identifier.hpp"
#include "rmw_opendds_shared_cpp/OpenDDSNode.hpp"
#include "rmw_opendds_shared_cpp/qos.hpp"
#include "rmw_opendds_shared_cpp/types.hpp"

// Uncomment this to get extra console output about discovery.
// This affects code in this file, but there is a similar variable in:
//   rmw_opendds_shared_cpp/shared_functions.cpp
// #define DISCOVERY_DEBUG_LOGGING 1

#include <dds/DCPS/DataWriterImpl_T.h>
#include <dds/DCPS/DomainParticipantImpl.h>
#include <dds/DCPS/Marked_Default_Qos.h>

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/rmw.h"
#include "rmw/types.h"

#include <string>

extern "C"
{
rmw_ret_t
rmw_init_publisher_allocation(
  const rosidl_message_type_support_t * type_support,
  const rosidl_runtime_c__Sequence__bound * message_bounds,
  rmw_publisher_allocation_t * allocation)
{
  // Unused in current implementation.
  (void) type_support;
  (void) message_bounds;
  (void) allocation;
  RMW_SET_ERROR_MSG("unimplemented");
  return RMW_RET_ERROR;
}

rmw_ret_t
rmw_fini_publisher_allocation(rmw_publisher_allocation_t * allocation)
{
  // Unused in current implementation.
  (void) allocation;
  RMW_SET_ERROR_MSG("unimplemented");
  return RMW_RET_ERROR;
}

void clean_publisher(rmw_publisher_t * publisher)
{
  if (publisher) {
    auto dds_pub = static_cast<DDSPublisher*>(publisher->data);
    if (dds_pub) {
      DDSPublisher::Raf::destroy(dds_pub);
      publisher->data = nullptr;
    }
    rmw_publisher_free(publisher);
  }
}

rmw_publisher_t * create_initial_publisher(const rmw_publisher_options_t * publisher_options)
{
  rmw_publisher_t * publisher = rmw_publisher_allocate();
  if (!publisher) {
    throw std::runtime_error("failed to allocate publisher");
  }
  publisher->implementation_identifier = opendds_identifier;
  publisher->data = nullptr;
  publisher->topic_name = nullptr;
  if (publisher_options) {
    publisher->options = *publisher_options;
  } else {
    publisher->options.rmw_specific_publisher_payload = nullptr;
  }
  publisher->can_loan_messages = false;
  return publisher;
}

rmw_publisher_t *
rmw_create_publisher(
  const rmw_node_t * node,
  const rosidl_message_type_support_t * type_supports,
  const char * topic_name,
  const rmw_qos_profile_t * rmw_qos,
  const rmw_publisher_options_t * publisher_options)
{
  auto dds_node = OpenDDSNode::from(node);
  if (!dds_node) {
    return nullptr;
  }
  const rosidl_message_type_support_t * ts = rmw_get_message_type_support(type_supports);
  if (!ts) {
    return nullptr; // error set in rmw_get_message_type_support
  }
  if (!rmw_qos) {
    RMW_SET_ERROR_MSG("rmw_qos is null");
    return nullptr;
  }

  rmw_publisher_t * publisher = nullptr;
  try {
    publisher = create_initial_publisher(publisher_options);
    auto pub_i = DDSPublisher::Raf::create(dds_node->dp(), *ts, topic_name, *rmw_qos);
    if (!pub_i) {
      throw std::runtime_error("DDSPublisher failed");
    }
    publisher->data = pub_i;
    publisher->topic_name = pub_i->topic_name_.c_str();

    dds_node->add_pub(pub_i->instance_handle(), pub_i->topic_name_, pub_i->type_name_);
    return publisher;
  } catch (const std::exception& e) {
    RMW_SET_ERROR_MSG(e.what());
  } catch (...) {
    RMW_SET_ERROR_MSG("rmw_create_publisher failed");
  }
  clean_publisher(publisher);
  return nullptr;
}

rmw_ret_t
rmw_publisher_count_matched_subscriptions(
  const rmw_publisher_t * publisher,
  size_t * subscription_count)
{
  if (!subscription_count) {
    RMW_SET_ERROR_MSG("subscription_count is null");
    return RMW_RET_INVALID_ARGUMENT;
  }
  auto dds_pub = DDSPublisher::from(publisher);
  if (dds_pub) {
    *subscription_count = dds_pub->matched_subscribers();
    return RMW_RET_OK;
  }
  return RMW_RET_ERROR;
}

rmw_ret_t
rmw_publisher_get_actual_qos(
  const rmw_publisher_t * publisher,
  rmw_qos_profile_t * qos)
{
  if (!qos) {
    RMW_SET_ERROR_MSG("qos is null");
    return RMW_RET_INVALID_ARGUMENT;
  }
  auto dds_pub = DDSPublisher::from(publisher);
  return dds_pub ? dds_pub->get_rmw_qos(*qos) : RMW_RET_ERROR;
}

rmw_ret_t
rmw_publisher_assert_liveliness(const rmw_publisher_t * publisher)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(publisher, RMW_RET_INVALID_ARGUMENT);

  //auto info = static_cast<ConnextStaticPublisherInfo *>(publisher->data);
  //if (nullptr == info) {
  //  RMW_SET_ERROR_MSG("publisher internal data is invalid");
  //  return RMW_RET_ERROR;
  //}
  //if (nullptr == info->topic_writer_) {
  //  RMW_SET_ERROR_MSG("publisher internal datawriter is invalid");
  //  return RMW_RET_ERROR;
  //}

  //if (info->topic_writer_->assert_liveliness() != DDS::RETCODE_OK) {
  //  RMW_SET_ERROR_MSG("failed to assert liveliness of datawriter");
  //  return RMW_RET_ERROR;
  //}

  return RMW_RET_OK;
}

rmw_ret_t
rmw_borrow_loaned_message(
  const rmw_publisher_t * publisher,
  const rosidl_message_type_support_t * type_support,
  void ** ros_message)
{
  (void) publisher;
  (void) type_support;
  (void) ros_message;

  RMW_SET_ERROR_MSG("rmw_borrow_loaned_message not implemented for rmw_opendds_cpp");
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t
rmw_return_loaned_message_from_publisher(
  const rmw_publisher_t * publisher,
  void * loaned_message)
{
  (void) publisher;
  (void) loaned_message;

  RMW_SET_ERROR_MSG(
    "rmw_return_loaned_message_from_publisher not implemented for rmw_opendds_cpp");
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t
rmw_destroy_publisher(rmw_node_t * node, rmw_publisher_t * publisher)
{
  auto dds_node = OpenDDSNode::from(node);
  if (!dds_node) {
    return RMW_RET_ERROR; // error set
  }
  auto dds_pub = DDSPublisher::from(publisher);
  if (!dds_pub) {
    return RMW_RET_ERROR; // error set
  }
  bool ret = dds_node->remove_pub(dds_pub->instance_handle());
  if (ret) {
    clean_publisher(publisher);
  }
  return ret ? RMW_RET_OK : RMW_RET_ERROR;
}

rmw_ret_t
rmw_get_gid_for_publisher(const rmw_publisher_t * publisher, rmw_gid_t * gid)
{
  auto dds_pub = DDSPublisher::from(publisher);
  if (!dds_pub) {
    return RMW_RET_ERROR; // error set
  }
  if (!gid) {
    RMW_SET_ERROR_MSG("gid is null");
    return RMW_RET_ERROR;
  }
  *gid = dds_pub->gid();
  return RMW_RET_OK;
}
}  // extern "C"
