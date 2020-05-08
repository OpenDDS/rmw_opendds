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
#include "rmw/rmw.h"

#include "rmw_opendds_shared_cpp/qos.hpp"
#include "rmw_opendds_shared_cpp/types.hpp"

#include "rmw_opendds_cpp/identifier.hpp"
#include "process_topic_and_service_names.hpp"
#include "type_support_common.hpp"
#include "rmw_opendds_cpp/opendds_static_service_info.hpp"

#include <dds/DCPS/Marked_Default_Qos.h>

// Uncomment this to get extra console output about discovery.
// This affects code in this file, but there is a similar variable in:
//   rmw_opendds_shared_cpp/shared_functions.cpp
// #define DISCOVERY_DEBUG_LOGGING 1

extern "C"
{
rmw_ret_t clean_service(OpenDDSNodeInfo & node_info, rmw_service_t * service)
{
  auto ret = RMW_RET_OK;
  if (!service) {
    return ret;
  }
  auto info = static_cast<OpenDDSStaticServiceInfo*>(service->data);
  if (info) {
    if (info->request_reader_) {
      node_info.subscriber_listener->remove_information(
        info->request_reader_->get_instance_handle(), EntityType::Subscriber);
      node_info.subscriber_listener->trigger_graph_guard_condition();
      if (info->read_condition_) {
        if (info->request_reader_->delete_readcondition(info->read_condition_) != DDS::RETCODE_OK) {
          RMW_SET_ERROR_MSG("failed to delete_readcondition");
          ret = RMW_RET_ERROR;
        }
        info->read_condition_ = nullptr;
      }
      info->request_reader_ = nullptr;
    } else if (info->read_condition_) {
      RMW_SET_ERROR_MSG("cannot delete readcondition because the datareader is null");
      ret = RMW_RET_ERROR;
    }

    if (info->callbacks_ && info->replier_) {
      DDS::DataWriter_var reply_datawriter = static_cast<DDS::DataWriter *>(
        info->callbacks_->get_reply_datawriter(info->replier_));
      if (reply_datawriter) {
        node_info.publisher_listener->remove_information(
          reply_datawriter->get_instance_handle(), EntityType::Publisher);
        node_info.publisher_listener->trigger_graph_guard_condition();
      }
      info->callbacks_->destroy_replier(info->replier_, &rmw_free);
      info->replier_ = nullptr;
    }

    RMW_TRY_DESTRUCTOR(info->~OpenDDSStaticServiceInfo(), OpenDDSStaticServiceInfo, ret = RMW_RET_ERROR)
    rmw_free(info);
    service->data = nullptr;
  }

  if (service->service_name) {
    rmw_free(const_cast<char *>(service->service_name));
    service->service_name = nullptr;
  }
  rmw_service_free(service);
  return ret;
}

rmw_service_t *
rmw_create_service(
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
  CORBA::String_var request_topic;
  CORBA::String_var response_topic;
  if (!_process_service_name(service_name, qos_profile->avoid_ros_namespace_conventions,
      &request_topic.out(), &response_topic.out())) {
    return nullptr;
  }

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

  // create service
  rmw_service_t * service = rmw_service_allocate();
  if (!service) {
    RMW_SET_ERROR_MSG("failed to allocate service");
    return nullptr;
  }
  void * buf = nullptr;
  try {
    service->implementation_identifier = opendds_identifier;
    service->data = nullptr;
    service->service_name = reinterpret_cast<const char *>(rmw_allocate(strlen(service_name) + 1));
    if (!service->service_name) {
      throw std::string("failed to allocate memory for service name");
    }
    memcpy(const_cast<char*>(service->service_name), service_name, strlen(service_name) + 1);

    // create OpenDDSStaticServiceInfo
    buf = rmw_allocate(sizeof(OpenDDSStaticServiceInfo));
    if (!buf) {
      throw std::string("failed to allocate memory for service info");
    }
    OpenDDSStaticServiceInfo * info = nullptr;
    RMW_TRY_PLACEMENT_NEW(info, buf, throw 1, OpenDDSStaticServiceInfo,
      static_cast<const service_type_support_callbacks_t*>(type_support->data))
    service->data = info;
    buf = nullptr;
    if (!info->callbacks_) {
      throw std::string("callbacks handle is null");
    }

    // create replier
/* TODO: open this block when service typesupport is ready.
    info->replier_ = info->callbacks_->create_replier(node_info->participant,
      request_topic.in(), response_topic.in(), publisher, subscriber, &rmw_allocate);
    if (!info->replier_) {
      throw std::string("failed to create_replier");
    }

    info->request_reader_ = static_cast<DDS::DataReader*>(
      info->callbacks_->get_request_datareader(info->replier_));
    if (!info->request_reader_) {
      throw std::string("request_reader_ is null");
    }

    info->read_condition_ = info->request_reader_->create_readcondition(
      DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
    if (!info->read_condition_) {
      throw std::string("failed to create_readcondition");
    }

    // update node_info subscriber_listener
    DDS::TopicDescription_ptr rtopic_des = info->request_reader_->get_topicdescription();
    if (!rtopic_des) {
      throw std::string("failed to get_topicdescription");
    }
    CORBA::String_var name = rtopic_des->get_name();
    CORBA::String_var type_name = rtopic_des->get_type_name();
    if (!name || !type_name) {
      throw std::string("topicdescription name or type_name is null");
    }
    node_info->subscriber_listener->add_information(node_info->participant->get_instance_handle(),
      info->request_reader_->get_instance_handle(), name.in(), type_name.in(), EntityType::Subscriber);
    node_info->subscriber_listener->trigger_graph_guard_condition();

    // update node_info publisher_listener
    DDS::DataWriter_var writer = static_cast<DDS::DataWriter*>(
      info->callbacks_->get_reply_datawriter(info->replier_));
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
*/
    return service;

  } catch (const std::string& e) {
    RMW_SET_ERROR_MSG(e.c_str());
  } catch (...) {
    RMW_SET_ERROR_MSG("rmw_create_service failed");
  }
  clean_service(*node_info, service);
  if (buf) {
    rmw_free(buf);
  }
  return nullptr;
}

rmw_ret_t
rmw_destroy_service(rmw_node_t * node, rmw_service_t * service)
{
  RMW_CHECK_FOR_NULL_WITH_MSG(node, "node is null", return RMW_RET_ERROR);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(node, node->implementation_identifier,
    opendds_identifier, return RMW_RET_ERROR)
  auto node_info = static_cast<OpenDDSNodeInfo *>(node->data);
  RMW_CHECK_FOR_NULL_WITH_MSG(node_info, "node_info is null", return RMW_RET_ERROR);

  RMW_CHECK_FOR_NULL_WITH_MSG(service, "service is null", return RMW_RET_ERROR);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(service, service->implementation_identifier,
    opendds_identifier, return RMW_RET_ERROR)

  return clean_service(*node_info, service);
}

}  // extern "C"
