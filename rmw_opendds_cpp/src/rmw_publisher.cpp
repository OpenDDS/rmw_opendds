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

#include <string>

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/rmw.h"
#include "rmw/types.h"

#include "rmw_opendds_shared_cpp/qos.hpp"
#include "rmw_opendds_shared_cpp/types.hpp"

#include "rmw_opendds_cpp/identifier.hpp"

#include "process_topic_and_service_names.hpp"
#include "type_support_common.hpp"
#include "rmw_opendds_cpp/opendds_static_publisher_info.hpp"

// include patched generated code from the build folder
//#include "opendds_static_serialized_dataTypeSupportC.h"
#include "opendds_static_serialized_dataTypeSupportImpl.h"

// Uncomment this to get extra console output about discovery.
// This affects code in this file, but there is a similar variable in:
//   rmw_opendds_shared_cpp/shared_functions.cpp
// #define DISCOVERY_DEBUG_LOGGING 1

extern "C"
{
rmw_ret_t
rmw_init_publisher_allocation(
  const rosidl_message_type_support_t * type_support,
  const rosidl_message_bounds_t * message_bounds,
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

rmw_publisher_t *
rmw_create_publisher(
  const rmw_node_t * node,
  const rosidl_message_type_support_t * type_supports,
  const char * topic_name,
  const rmw_qos_profile_t * qos_policies,
  const rmw_publisher_options_t * publisher_options)
{
  if (!node) {
    RMW_SET_ERROR_MSG("node handle is null");
    return NULL;
  }
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node handle,
    node->implementation_identifier, opendds_identifier,
    return NULL)

  RMW_OPENDDS_EXTRACT_MESSAGE_TYPESUPPORT(type_supports, type_support, NULL)

  if (!topic_name || strlen(topic_name) == 0) {
    RMW_SET_ERROR_MSG("publisher topic is null or empty string");
    return NULL;
  }

  if (!qos_policies) {
    RMW_SET_ERROR_MSG("qos_policies is null");
    return NULL;
  }

  auto node_info = static_cast<OpenDDSNodeInfo *>(node->data);
  if (!node_info) {
    RMW_SET_ERROR_MSG("node info handle is null");
    return NULL;
  }
  auto participant = static_cast<DDS::DomainParticipant *>(node_info->participant);
  if (!participant) {
    RMW_SET_ERROR_MSG("participant handle is null");
    return NULL;
  }

  const message_type_support_callbacks_t * callbacks =
    static_cast<const message_type_support_callbacks_t *>(type_support->data);
  if (!callbacks) {
    RMW_SET_ERROR_MSG("callbacks handle is null");
    return NULL;
  }
  std::string type_name = _create_type_name(callbacks, "msg");
  // Past this point, a failure results in unrolling code in the goto fail block.
  DDS::DataWriterQos datawriter_qos;
  DDS::PublisherQos publisher_qos;
  DDS::ReturnCode_t status;
  DDS::Publisher_var dds_publisher;
  DDS::DataWriter_var topic_writer;
  DDS::Topic_var topic;
  DDS::TopicDescription_var topic_description;
  void * info_buf = nullptr;
  void * listener_buf = nullptr;
  OpenDDSPublisherListener * publisher_listener = nullptr;
  OpenDDSStaticPublisherInfo * publisher_info = nullptr;
  rmw_publisher_t * publisher = nullptr;
  std::string mangled_name = "";
  OpenDDSStaticSerializedDataTypeSupport_var ts = new OpenDDSStaticSerializedDataTypeSupportImpl();

  char * topic_str = nullptr;

  // Begin initializing elements
  publisher = rmw_publisher_allocate();
  if (!publisher) {
    RMW_SET_ERROR_MSG("failed to allocate publisher");
    goto fail;
  }

  publisher->can_loan_messages = false;

  // Register TypeSupport here ????????????????????????
  if (ts->register_type(participant, "") != DDS::RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to register OpenDDS type");
    goto fail;
  }

  status = participant->get_default_publisher_qos(publisher_qos);
  if (status != DDS::RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to get default publisher qos");
    goto fail;
  }

  // allocating memory for topic_str
  if (!_process_topic_name(
      topic_name,
      qos_policies->avoid_ros_namespace_conventions,
      &topic_str))
  {
    goto fail;
  }

  // Allocate memory for the PublisherListener object.
  listener_buf = rmw_allocate(sizeof(OpenDDSPublisherListener));
  if (!listener_buf) {
    RMW_SET_ERROR_MSG("failed to allocate memory for publisher listener");
    goto fail;
  }
  // Use a placement new to construct the PublisherListener in the preallocated buffer.
  RMW_TRY_PLACEMENT_NEW(publisher_listener, listener_buf, goto fail, OpenDDSPublisherListener, )
  listener_buf = nullptr;  // Only free the buffer pointer.

  dds_publisher = participant->create_publisher(
    publisher_qos, publisher_listener, DDS::PUBLICATION_MATCHED_STATUS);
  if (!dds_publisher) {
    RMW_SET_ERROR_MSG("failed to create publisher");
    goto fail;
  }

  topic_description = participant->lookup_topicdescription(topic_str);
  if (!topic_description) {
    DDS::TopicQos default_topic_qos;
    status = participant->get_default_topic_qos(default_topic_qos);
    if (status != DDS::RETCODE_OK) {
      RMW_SET_ERROR_MSG("failed to get default topic qos");
      goto fail;
    }

    topic = participant->create_topic(
      topic_str, type_name.c_str(),
      default_topic_qos, NULL, OpenDDS::DCPS::NO_STATUS_MASK);
    if (!topic) {
      RMW_SET_ERROR_MSG("failed to create topic");
      goto fail;
    }
  } else {
    DDS::Duration_t timeout = { 0, 0 };
    topic = participant->find_topic(topic_str, timeout);
    if (!topic) {
      RMW_SET_ERROR_MSG("failed to find topic");
      goto fail;
    }
  }
  CORBA::string_free(topic_str);
  topic_str = nullptr;

  if (!get_datawriter_qos(participant, *qos_policies, datawriter_qos)) {
    // error string was set within the function
    goto fail;
  }

  topic_writer = dds_publisher->create_datawriter(
    topic, datawriter_qos, NULL, OpenDDS::DCPS::NO_STATUS_MASK);
  if (!topic_writer) {
    RMW_SET_ERROR_MSG("failed to create datawriter");
    goto fail;
  }

  // Allocate memory for the OpenDDSStaticPublisherInfo object.
  info_buf = rmw_allocate(sizeof(OpenDDSStaticPublisherInfo));
  if (!info_buf) {
    RMW_SET_ERROR_MSG("failed to allocate memory for publisher info");
    goto fail;
  }
  // Use a placement new to construct the OpenDDSStaticPublisherInfo in the preallocated buffer.
  RMW_TRY_PLACEMENT_NEW(publisher_info, info_buf, goto fail, OpenDDSStaticPublisherInfo, )
  info_buf = nullptr;  // Only free the publisher_info pointer; don't need the buf pointer anymore.
  publisher_info->dds_publisher_ = dds_publisher;
  publisher_info->topic_writer_ = topic_writer;
  publisher_info->callbacks_ = callbacks;
  publisher_info->publisher_gid.implementation_identifier = opendds_identifier;
  publisher_info->listener_ = publisher_listener;
  publisher_listener = nullptr;
  static_assert(
    sizeof(OpenDDSPublisherGID) <= RMW_GID_STORAGE_SIZE,
    "RMW_GID_STORAGE_SIZE insufficient to store the rmw_opendds_cpp GID implemenation."
  );
  // Zero the data memory.
  memset(publisher_info->publisher_gid.data, 0, RMW_GID_STORAGE_SIZE);
  {
    auto publisher_gid =
      reinterpret_cast<OpenDDSPublisherGID *>(publisher_info->publisher_gid.data);
    publisher_gid->publication_handle = topic_writer->get_instance_handle();
  }
  publisher_info->publisher_gid.implementation_identifier = opendds_identifier;

  publisher->implementation_identifier = opendds_identifier;
  publisher->data = publisher_info;
  publisher->topic_name = reinterpret_cast<const char *>(rmw_allocate(strlen(topic_name) + 1));
  if (!publisher->topic_name) {
    RMW_SET_ERROR_MSG("failed to allocate memory for node name");
    goto fail;
  }
  memcpy(const_cast<char *>(publisher->topic_name), topic_name, strlen(topic_name) + 1);

  if (!qos_policies->avoid_ros_namespace_conventions) {
    mangled_name =
      topic_writer->get_topic()->get_name();
  } else {
    mangled_name = topic_name;
  }
  node_info->publisher_listener->add_information(
    node_info->participant->get_instance_handle(),
    dds_publisher->get_instance_handle(),
    mangled_name,
    type_name,
    EntityType::Publisher);
  node_info->publisher_listener->trigger_graph_guard_condition();

  return publisher;
fail:
  if (topic_str) {
    CORBA::string_free(topic_str);
    topic_str = nullptr;
  }
  if (publisher) {
    rmw_publisher_free(publisher);
  }
  if (publisher_listener) {
    RMW_TRY_DESTRUCTOR_FROM_WITHIN_FAILURE(
      publisher_listener->~OpenDDSPublisherListener(), OpenDDSPublisherListener)
    rmw_free(publisher_listener);
  }
  if (publisher_info) {
    if (publisher_info->listener_) {
      RMW_TRY_DESTRUCTOR_FROM_WITHIN_FAILURE(
        publisher_info->listener_->~OpenDDSPublisherListener(), OpenDDSPublisherListener)
      rmw_free(publisher_info->listener_);
    }
    RMW_TRY_DESTRUCTOR_FROM_WITHIN_FAILURE(
      publisher_info->~OpenDDSStaticPublisherInfo(), OpenDDSStaticPublisherInfo)
    rmw_free(publisher_info);
  }
  if (info_buf) {
    rmw_free(info_buf);
  }
  if (listener_buf) {
    rmw_free(listener_buf);
  }

  return NULL;
}

rmw_ret_t
rmw_publisher_count_matched_subscriptions(
  const rmw_publisher_t * publisher,
  size_t * subscription_count)
{
  if (!publisher) {
    RMW_SET_ERROR_MSG("publisher handle is null");
    return RMW_RET_INVALID_ARGUMENT;
  }

  if (!subscription_count) {
    RMW_SET_ERROR_MSG("subscription_count is null");
    return RMW_RET_INVALID_ARGUMENT;
  }

  auto info = static_cast<OpenDDSStaticPublisherInfo *>(publisher->data);
  if (!info) {
    RMW_SET_ERROR_MSG("publisher internal data is invalid");
    return RMW_RET_ERROR;
  }
  if (!info->listener_) {
    RMW_SET_ERROR_MSG("publisher internal listener is invalid");
    return RMW_RET_ERROR;
  }

  *subscription_count = info->listener_->current_count();

  return RMW_RET_OK;
}

rmw_ret_t
rmw_publisher_get_actual_qos(
  const rmw_publisher_t * publisher,
  rmw_qos_profile_t * qos)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(publisher, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(qos, RMW_RET_INVALID_ARGUMENT);

  auto info = static_cast<OpenDDSStaticPublisherInfo*>(publisher->data);
  if (!info) {
    RMW_SET_ERROR_MSG("publisher internal data is invalid");
    return RMW_RET_ERROR;
  }
  DDS::DataWriter_var data_writer = info->topic_writer_;
  if (!data_writer) {
    RMW_SET_ERROR_MSG("publisher internal data writer is invalid");
    return RMW_RET_ERROR;
  }
  DDS::DataWriterQos dds_qos;
  DDS::ReturnCode_t status = data_writer->get_qos(dds_qos);
  if (DDS::RETCODE_OK != status) {
    RMW_SET_ERROR_MSG("publisher can't get data writer qos policies");
    return RMW_RET_ERROR;
  }

  dds_qos_to_rmw_qos(dds_qos, qos);

  return RMW_RET_OK;
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

  RMW_SET_ERROR_MSG("rmw_borrow_loaned_message not implemented for rmw_connext_cpp");
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
    "rmw_return_loaned_message_from_publisher not implemented for rmw_connext_cpp");
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t
rmw_destroy_publisher(rmw_node_t * node, rmw_publisher_t * publisher)
{
  if (!node) {
    RMW_SET_ERROR_MSG("node handle is null");
    return RMW_RET_ERROR;
  }
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node handle,
    node->implementation_identifier, opendds_identifier,
    return RMW_RET_ERROR)

  if (!publisher) {
    RMW_SET_ERROR_MSG("publisher handle is null");
    return RMW_RET_ERROR;
  }
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    publisher handle,
    publisher->implementation_identifier, opendds_identifier,
    return RMW_RET_ERROR)

  auto node_info = static_cast<OpenDDSNodeInfo *>(node->data);
  if (!node_info) {
    RMW_SET_ERROR_MSG("node info handle is null");
    return RMW_RET_ERROR;
  }
  auto participant = static_cast<DDS::DomainParticipant *>(node_info->participant);
  if (!participant) {
    RMW_SET_ERROR_MSG("participant handle is null");
    return RMW_RET_ERROR;
  }
  // TODO(wjwwood): need to figure out when to unregister types with the participant.
  OpenDDSStaticPublisherInfo * publisher_info =
    static_cast<OpenDDSStaticPublisherInfo *>(publisher->data);
  if (publisher_info) {
    node_info->publisher_listener->remove_information(
      publisher_info->dds_publisher_->get_instance_handle(), EntityType::Publisher);
    node_info->publisher_listener->trigger_graph_guard_condition();
    //DDS::Publisher * dds_publisher = publisher_info->dds_publisher_;

    //if (dds_publisher) {
    //  if (publisher_info->topic_writer_) {
    //    if (dds_publisher->delete_datawriter(publisher_info->topic_writer_) != DDS::RETCODE_OK) {
    //      RMW_SET_ERROR_MSG("failed to delete datawriter");
    //      return RMW_RET_ERROR;
    //    }
    //    publisher_info->topic_writer_ = nullptr;
    //  }
    //  if (participant->delete_publisher(dds_publisher) != DDS::RETCODE_OK) {
    //    RMW_SET_ERROR_MSG("failed to delete publisher");
    //    return RMW_RET_ERROR;
    //  }
    //  publisher_info->dds_publisher_ = nullptr;
    //} else if (publisher_info->topic_writer_) {
    //  RMW_SET_ERROR_MSG("cannot delete datawriter because the publisher is null");
    //  return RMW_RET_ERROR;
    //}

    OpenDDSPublisherListener * pub_listener = publisher_info->listener_;
    if (pub_listener) {
      RMW_TRY_DESTRUCTOR(
        pub_listener->~OpenDDSPublisherListener(),
        OpenDDSPublisherListener, return RMW_RET_ERROR)
      rmw_free(pub_listener);
    }

    RMW_TRY_DESTRUCTOR(
      publisher_info->~OpenDDSStaticPublisherInfo(),
      OpenDDSStaticPublisherInfo, return RMW_RET_ERROR)
    rmw_free(publisher_info);
    publisher->data = nullptr;
  }
  if (publisher->topic_name) {
    rmw_free(const_cast<char *>(publisher->topic_name));
  }
  rmw_publisher_free(publisher);

  return RMW_RET_OK;
}
}  // extern "C"
