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

#include "rmw_opendds_shared_cpp/types.hpp"
#include "rmw_opendds_shared_cpp/qos.hpp"

#include "rmw_opendds_cpp/opendds_static_client_info.hpp"
#include "rmw_opendds_cpp/identifier.hpp"
#include "process_topic_and_service_names.hpp"
#include "type_support_common.hpp"

#include <dds/DCPS/Marked_Default_Qos.h>

// Uncomment this to get extra console output about discovery.
// This affects code in this file, but there is a similar variable in:
//   rmw_opendds_shared_cpp/shared_functions.cpp
// #define DISCOVERY_DEBUG_LOGGING 1

extern "C"
{
rmw_ret_t clean_client(OpenDDSNodeInfo & node_info, rmw_client_t * client)
{
  auto ret = RMW_RET_OK;
  if (!client) {
    return ret;
  }
  auto info = static_cast<OpenDDSStaticClientInfo*>(client->data);
  if (info) {
    if (info->response_reader_) {
      node_info.subscriber_listener->remove_information(
        info->response_reader_->get_instance_handle(), EntityType::Subscriber);
      node_info.subscriber_listener->trigger_graph_guard_condition();
      if (info->read_condition_) {
        if (info->response_reader_->delete_readcondition(info->read_condition_) != DDS::RETCODE_OK) {
          RMW_SET_ERROR_MSG("failed to delete_readcondition");
          ret = RMW_RET_ERROR;
        }
        info->read_condition_ = nullptr;
      }
      info->response_reader_ = nullptr;
    } else if (info->read_condition_) {
      RMW_SET_ERROR_MSG("cannot delete readcondition because the datareader is null");
      ret = RMW_RET_ERROR;
    }

    if (info->callbacks_ && info->requester_) {
      DDS::DataWriter_var writer = static_cast<DDS::DataWriter *>(
        info->callbacks_->get_request_datawriter(info->requester_));
      if (writer) {
        node_info.publisher_listener->remove_information(
          writer->get_instance_handle(), EntityType::Publisher);
        node_info.publisher_listener->trigger_graph_guard_condition();
      }
      info->callbacks_->destroy_requester(info->requester_, &rmw_free);
      info->requester_ = nullptr;
    }

    RMW_TRY_DESTRUCTOR(info->~OpenDDSStaticClientInfo(), OpenDDSStaticClientInfo, ret = RMW_RET_ERROR)
    rmw_free(info);
    client->data = nullptr;
  }

  if (client->service_name) {
    rmw_free(const_cast<char *>(client->service_name));
    client->service_name = nullptr;
  }
  rmw_client_free(client);
  return ret;
}

rmw_client_t *
rmw_create_client(
  const rmw_node_t * node,
  const rosidl_service_type_support_t * type_supports,
  const char * service_name,
  const rmw_qos_profile_t * qos_profile)
{
  RMW_CHECK_FOR_NULL_WITH_MSG(node, "node is null", return nullptr);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(node, node->implementation_identifier, opendds_identifier, return nullptr)

  auto node_info = static_cast<OpenDDSNodeInfo *>(node->data);
  RMW_CHECK_FOR_NULL_WITH_MSG(node_info, "node_info is null", return nullptr);
  RMW_CHECK_FOR_NULL_WITH_MSG(node_info->participant.in(), "participant is null", return nullptr);
  RMW_CHECK_FOR_NULL_WITH_MSG(node_info->publisher_listener, "publisher_listener is null", return nullptr);
  RMW_CHECK_FOR_NULL_WITH_MSG(node_info->subscriber_listener, "subscriber_listener is null", return nullptr);

  const rosidl_service_type_support_t * type_support = rmw_get_service_type_support(type_supports);
  if (!type_support) {
    return nullptr;
  }

  RMW_CHECK_FOR_NULL_WITH_MSG(service_name, "service_name is null", return nullptr);
  std::string request_topic;
  std::string response_topic;
  get_service_topic_names(service_name, qos_profile->avoid_ros_namespace_conventions, request_topic, response_topic);

  RMW_CHECK_FOR_NULL_WITH_MSG(qos_profile, "qos_profile is null", return nullptr);

  // create publisher
  DDS::Publisher_var publisher = node_info->participant->create_publisher(
    PUBLISHER_QOS_DEFAULT, NULL, OpenDDS::DCPS::NO_STATUS_MASK);
  if (!publisher) {
    RMW_SET_ERROR_MSG("failed to create_publisher");
    return nullptr;
  }
  DDS::DataWriterQos writer_qos;
  if (!get_datawriter_qos(publisher, *qos_profile, writer_qos)) {
    RMW_SET_ERROR_MSG("failed to get_datawriter_qos");
    return nullptr;
  }
  if (publisher->set_default_datawriter_qos(writer_qos) != DDS::RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to set_default_datawriter_qos");
    return nullptr;
  }

  // create subscriber
  DDS::Subscriber_var subscriber = node_info->participant->create_subscriber(
    SUBSCRIBER_QOS_DEFAULT, NULL, OpenDDS::DCPS::NO_STATUS_MASK);
  if (!subscriber) {
    RMW_SET_ERROR_MSG("failed to create_subscriber");
    return nullptr;
  }
  DDS::DataReaderQos reader_qos;
  if (!get_datareader_qos(subscriber, *qos_profile, reader_qos)) {
    RMW_SET_ERROR_MSG("failed to get_datareader_qos");
    return nullptr;
  }
  if (subscriber->set_default_datareader_qos(reader_qos) != DDS::RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to set_default_datareader_qos");
    return nullptr;
  }

  rmw_client_t * client = rmw_client_allocate();
  if (!client) {
    RMW_SET_ERROR_MSG("failed to allocate client");
    return nullptr;
  }
  void * buf = nullptr;
  try {
    client->implementation_identifier = opendds_identifier;
    client->data = nullptr;
    client->service_name = reinterpret_cast<const char *>(rmw_allocate(strlen(service_name) + 1));
    if (!client->service_name) {
      throw std::string("failed to allocate memory for service name");
    }
    std::strcpy(const_cast<char*>(client->service_name), service_name);

    buf = rmw_allocate(sizeof(OpenDDSStaticClientInfo));
    if (!buf) {
      throw std::string("failed to allocate memory for client info");
    }
    OpenDDSStaticClientInfo * info = nullptr;
    RMW_TRY_PLACEMENT_NEW(info, buf, throw 1, OpenDDSStaticClientInfo,
      static_cast<const service_type_support_callbacks_t*>(type_support->data))
    client->data = info;
    buf = nullptr;
    if (!info->callbacks_) {
      throw std::string("callbacks handle is null");
    }

    info->requester_ = info->callbacks_->create_requester(node_info->participant,
      request_topic.c_str(), response_topic.c_str(), publisher, subscriber, &rmw_allocate);
    if (!info->requester_) {
      throw std::string("failed to create_requester");
    }

    info->response_reader_ = static_cast<DDS::DataReader*>(
      info->callbacks_->get_reply_datareader(info->requester_));
    if (!info->response_reader_) {
      throw std::string("response_reader_ is null");
    }

    info->read_condition_ = info->response_reader_->create_readcondition(
      DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
    if (!info->read_condition_) {
      throw std::string("failed to create_readcondition");
    }

    // update node_info subscriber_listener
    DDS::TopicDescription_ptr rtopic_des = info->response_reader_->get_topicdescription();
    if (!rtopic_des) {
      throw std::string("failed to get_topicdescription");
    }
    CORBA::String_var name = rtopic_des->get_name();
    CORBA::String_var type_name = rtopic_des->get_type_name();
    if (!name || !type_name) {
      throw std::string("topicdescription name or type_name is null");
    }
    node_info->subscriber_listener->add_information(node_info->participant->get_instance_handle(),
      info->response_reader_->get_instance_handle(), name.in(), type_name.in(), EntityType::Subscriber);
    node_info->subscriber_listener->trigger_graph_guard_condition();

    // update node_info publisher_listener
    DDS::DataWriter_var writer = static_cast<DDS::DataWriter*>(
      info->callbacks_->get_request_datawriter(info->requester_));
    if (!writer) {
      throw std::string("reply writer is null");
    }
    DDS::Topic_ptr wtopic = writer->get_topic();
    if (!wtopic) {
      throw std::string("writer->get_topic failed");
    }
    name = wtopic->get_name();
    type_name = wtopic->get_type_name();
    if (!name || !type_name) {
      throw std::string("writer topic name or type_name is null");
    }
    node_info->publisher_listener->add_information(node_info->participant->get_instance_handle(),
      writer->get_instance_handle(), name.in(), type_name.in(), EntityType::Publisher);
    node_info->publisher_listener->trigger_graph_guard_condition();

    return client;

  } catch (const std::string& e) {
    RMW_SET_ERROR_MSG(e.c_str());
  } catch (...) {
    RMW_SET_ERROR_MSG("rmw_create_client failed");
  }
  clean_client(*node_info, client);
  if (buf) {
    rmw_free(buf);
  }
  return nullptr;
}

rmw_ret_t
rmw_destroy_client(rmw_node_t * node, rmw_client_t * client)
{
  RMW_CHECK_FOR_NULL_WITH_MSG(node, "node is null", return RMW_RET_ERROR);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(node, node->implementation_identifier,
    opendds_identifier, return RMW_RET_ERROR)
  auto node_info = static_cast<OpenDDSNodeInfo *>(node->data);
  RMW_CHECK_FOR_NULL_WITH_MSG(node_info, "node_info is null", return RMW_RET_ERROR);

  RMW_CHECK_FOR_NULL_WITH_MSG(client, "client is null", return RMW_RET_ERROR);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(client, client->implementation_identifier,
    opendds_identifier, return RMW_RET_ERROR)

  return clean_client(*node_info, client);
}
}  // extern "C"
