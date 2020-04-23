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

#include <dds/DCPS/Marked_Default_Qos.h>

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

void clean_publisher(rmw_publisher_t * publisher){
  if (!publisher) {
    return;
  }

  auto info = static_cast<OpenDDSStaticPublisherInfo*>(publisher->data);
  if (info) {
    if (info->listener_) {
      RMW_TRY_DESTRUCTOR(info->listener_->~OpenDDSPublisherListener(), OpenDDSPublisherListener,)
      rmw_free(info->listener_);
      info->listener_ = nullptr;
    }

    RMW_TRY_DESTRUCTOR(info->~OpenDDSStaticPublisherInfo(), OpenDDSStaticPublisherInfo,)
    rmw_free(info);
    publisher->data = nullptr;
  }

  if (publisher->topic_name) {
    rmw_free(const_cast<char *>(publisher->topic_name));
  }

  rmw_publisher_free(publisher);
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
    RMW_SET_ERROR_MSG("node is null");
    return nullptr;
  }
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node handle, node->implementation_identifier, opendds_identifier,
    return nullptr)

  auto node_info = static_cast<OpenDDSNodeInfo *>(node->data);
  if (!node_info) {
    RMW_SET_ERROR_MSG("node info is null");
    return nullptr;
  }

  auto participant = static_cast<DDS::DomainParticipant *>(node_info->participant);
  if (!participant) {
    RMW_SET_ERROR_MSG("participant is null");
    return nullptr;
  }

  // message type support
  const rosidl_message_type_support_t * type_support = rmw_get_message_type_support(type_supports);
  if (!type_support) {
    return nullptr;
  }
  auto callbacks = static_cast<const message_type_support_callbacks_t*>(type_support->data);
  if (!callbacks) {
    RMW_SET_ERROR_MSG("callbacks handle is null");
    return nullptr;
  }
  std::string type_name = _create_type_name(callbacks, "msg");
  OpenDDSStaticSerializedDataTypeSupport_var ts = new OpenDDSStaticSerializedDataTypeSupportImpl();
  if (ts->register_type(participant, type_name.c_str()) != DDS::RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to register OpenDDS type");
    return nullptr;
  }

  if (!topic_name || strlen(topic_name) == 0) {
    RMW_SET_ERROR_MSG("publisher topic_name is null or empty");
    return nullptr;
  }
  if (!qos_policies) {
    RMW_SET_ERROR_MSG("qos_policies is null");
    return nullptr;
  }
  CORBA::String_var topic_str;
  if (!_process_topic_name(topic_name, qos_policies->avoid_ros_namespace_conventions, &topic_str.out())) {
    RMW_SET_ERROR_MSG("failed to allocate memory for topic_str");
    return nullptr;
  }

  // find or create DDS::Topic
  DDS::TopicDescription_var topic_description = participant->lookup_topicdescription(topic_str.in());
  DDS::Topic_var topic = topic_description ?
    participant->find_topic(topic_str.in(), DDS::Duration_t{0, 0}) :
    participant->create_topic(topic_str.in(), type_name.c_str(), TOPIC_QOS_DEFAULT, NULL, OpenDDS::DCPS::NO_STATUS_MASK);
  if (!topic) {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("failed to %s topic", (topic_description ? "find" : "create"));
    return nullptr;
  }

  rmw_publisher_t * publisher = nullptr;
  void * buf = nullptr;
  try {
    // create rmw_publisher_t
    publisher = rmw_publisher_allocate();
    if (!publisher) {
      throw std::string("failed to allocate publisher");
    }
    publisher->implementation_identifier = opendds_identifier;
    publisher->data = nullptr;
    publisher->topic_name = reinterpret_cast<const char*>(rmw_allocate(strlen(topic_name) + 1));
    if (!publisher->topic_name) {
      throw std::string("failed to allocate memory for topic name");
    }
    memcpy(const_cast<char*>(publisher->topic_name), topic_name, strlen(topic_name) + 1);
    if (publisher_options) {
      publisher->options = *publisher_options;
    }
    publisher->can_loan_messages = false;

    // create OpenDDSStaticPublisherInfo
    buf = rmw_allocate(sizeof(OpenDDSStaticPublisherInfo));
    if (!buf) {
      throw std::string("failed to allocate memory for publisher info");
    }
    OpenDDSStaticPublisherInfo * publisher_info = nullptr;
    RMW_TRY_PLACEMENT_NEW(publisher_info, buf, throw 1, OpenDDSStaticPublisherInfo,)
    publisher->data = publisher_info;
    buf = nullptr;

    // create OpenDDSPublisherListener
    buf = rmw_allocate(sizeof(OpenDDSPublisherListener));
    if (!buf) {
      throw std::string("failed to allocate memory for publisher listener");
    }
    RMW_TRY_PLACEMENT_NEW(publisher_info->listener_, buf, throw 1, OpenDDSPublisherListener,)
    buf = nullptr;

    // create DDS::Publisher
    publisher_info->dds_publisher_ = participant->create_publisher(
      PUBLISHER_QOS_DEFAULT, publisher_info->listener_, DDS::PUBLICATION_MATCHED_STATUS);
    if (!publisher_info->dds_publisher_) {
      throw std::string("failed to create publisher");
    }

    // create DDS::DataWriter
    DDS::DataWriterQos dw_qos;
    if (!get_datawriter_qos(publisher_info->dds_publisher_.in(), *qos_policies, dw_qos)) {
      throw std::string("get_datawriter_qos failed");
    }
    publisher_info->topic_writer_ = publisher_info->dds_publisher_->create_datawriter(
      topic, dw_qos, NULL, OpenDDS::DCPS::NO_STATUS_MASK);
    if (!publisher_info->topic_writer_) {
      throw std::string("failed to create datawriter");
    }

    // set remaining OpenDDSStaticPublisherInfo members
    publisher_info->callbacks_ = callbacks;
    publisher_info->publisher_gid.implementation_identifier = opendds_identifier;
    static_assert(sizeof(OpenDDSPublisherGID) <= RMW_GID_STORAGE_SIZE, "insufficient RMW_GID_STORAGE_SIZE");
    memset(publisher_info->publisher_gid.data, 0, RMW_GID_STORAGE_SIZE); // zero the data memory
    {
      auto publisher_gid = reinterpret_cast<OpenDDSPublisherGID*>(publisher_info->publisher_gid.data);
      publisher_gid->publication_handle = publisher_info->topic_writer_->get_instance_handle();
    }

    // update node_info
    std::string mangled_name;
    if (qos_policies->avoid_ros_namespace_conventions) {
      mangled_name = topic_name;
    } else {
      DDS::Topic_var topic = publisher_info->topic_writer_->get_topic();
      if (topic) {
        CORBA::String_var name = topic->get_name();
        if (name) { mangled_name = *name; }
      }
      if (mangled_name.empty()) {
        throw std::string("failed to get topic name");
      }
    }
    node_info->publisher_listener->add_information(participant->get_instance_handle(),
      publisher_info->dds_publisher_->get_instance_handle(), mangled_name, type_name, EntityType::Publisher);
    node_info->publisher_listener->trigger_graph_guard_condition();

    return publisher;

  } catch (const std::string& e) {
    RMW_SET_ERROR_MSG(e.c_str());
  } catch (...) {
    RMW_SET_ERROR_MSG("rmw_create_publisher failed");
  }

  clean_publisher(publisher);
  if (buf) {
    rmw_free(buf);
  }
  return nullptr;
}

rmw_ret_t
rmw_publisher_count_matched_subscriptions(
  const rmw_publisher_t * publisher,
  size_t * subscription_count)
{
  if (!publisher) {
    RMW_SET_ERROR_MSG("publisher is null");
    return RMW_RET_INVALID_ARGUMENT;
  }

  if (!subscription_count) {
    RMW_SET_ERROR_MSG("subscription_count is null");
    return RMW_RET_INVALID_ARGUMENT;
  }

  auto info = static_cast<OpenDDSStaticPublisherInfo *>(publisher->data);
  if (!info) {
    RMW_SET_ERROR_MSG("publisher info is null");
    return RMW_RET_ERROR;
  }
  if (!info->listener_) {
    RMW_SET_ERROR_MSG("publisher listener is null");
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
    RMW_SET_ERROR_MSG("publisher info is null");
    return RMW_RET_ERROR;
  }
  DDS::DataWriter_var data_writer = info->topic_writer_;
  if (!data_writer) {
    RMW_SET_ERROR_MSG("publisher writer is null");
    return RMW_RET_ERROR;
  }
  DDS::DataWriterQos dds_qos;
  DDS::ReturnCode_t status = data_writer->get_qos(dds_qos);
  if (DDS::RETCODE_OK != status) {
    RMW_SET_ERROR_MSG("publisher writer get_qos failed");
    return RMW_RET_ERROR;
  }
  dds_qos_to_rmw_qos(dds_qos, *qos);

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
