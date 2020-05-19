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

#include "rmw_opendds_shared_cpp/qos.hpp"
#include "rmw_opendds_shared_cpp/types.hpp"

#include "rmw_opendds_cpp/identifier.hpp"

#include "process_topic_and_service_names.hpp"
#include "type_support_common.hpp"
#include "rmw_opendds_cpp/opendds_static_subscriber_info.hpp"

// include patched generated code from the build folder
#include "opendds_static_serialized_dataTypeSupportImpl.h"

// Uncomment this to get extra console output about discovery.
// This affects code in this file, but there is a similar variable in:
//   rmw_opendds_shared_cpp/shared_functions.cpp
// #define DISCOVERY_DEBUG_LOGGING 1
#include <dds/DCPS/Marked_Default_Qos.h>

extern "C"
{
rmw_ret_t
rmw_init_subscription_allocation(
  const rosidl_message_type_support_t * type_support,
  const rosidl_message_bounds_t * message_bounds,
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

rmw_ret_t
clean_subscription(rmw_subscription_t * subscription, OpenDDSNodeInfo & node_info, DDS::DomainParticipant & participant)
{
  auto ret = RMW_RET_OK;
  if (!subscription) {
    return ret;
  }

  auto info = static_cast<OpenDDSStaticSubscriberInfo*>(subscription->data);
  if (info) {
    if (info->dds_subscriber_) {
      node_info.subscriber_listener->remove_information(
        info->dds_subscriber_->get_instance_handle(), EntityType::Subscriber);
      node_info.subscriber_listener->trigger_graph_guard_condition();
      if (info->topic_reader_) {
        if (info->read_condition_) {
          if (info->topic_reader_->delete_readcondition(info->read_condition_) != DDS::RETCODE_OK) {
            RMW_SET_ERROR_MSG("delete_readcondition failed");
            ret = RMW_RET_ERROR;
          }
          info->read_condition_ = nullptr;
        }
        if (info->dds_subscriber_->delete_datareader(info->topic_reader_) != DDS::RETCODE_OK) {
          RMW_SET_ERROR_MSG("delete_datareader failed");
          ret = RMW_RET_ERROR;
        }
        info->topic_reader_ = nullptr;
      } else if (info->read_condition_) {
        RMW_SET_ERROR_MSG("cannot delete readcondition because the datareader is null");
        ret = RMW_RET_ERROR;
      }
      if (participant.delete_subscriber(info->dds_subscriber_) != DDS::RETCODE_OK) {
        RMW_SET_ERROR_MSG("delete_subscriber failed");
        ret = RMW_RET_ERROR;
      }
      info->dds_subscriber_ = nullptr;
    } else if (info->topic_reader_) {
      RMW_SET_ERROR_MSG("cannot delete datareader because the subscriber is null");
      ret = RMW_RET_ERROR;
    }
    if (info->listener_) {
      RMW_TRY_DESTRUCTOR(info->listener_->~OpenDDSSubscriberListener(),
        OpenDDSSubscriberListener, ret = RMW_RET_ERROR)
      rmw_free(info->listener_);
      info->listener_ = nullptr;
    }
    RMW_TRY_DESTRUCTOR(info->~OpenDDSStaticSubscriberInfo(),
      OpenDDSStaticSubscriberInfo, ret = RMW_RET_ERROR)
    rmw_free(info);
    subscription->data = nullptr;
  }
  if (subscription->topic_name) {
    rmw_free(const_cast<char *>(subscription->topic_name));
  }
  rmw_subscription_free(subscription);
  return ret;
}

rmw_subscription_t *
rmw_create_subscription(
  const rmw_node_t * node,
  const rosidl_message_type_support_t * type_supports,
  const char * topic_name,
  const rmw_qos_profile_t * qos_profile,
  const rmw_subscription_options_t* subscription_options)
{
  RMW_CHECK_FOR_NULL_WITH_MSG(node, "node is null", return nullptr);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(node, node->implementation_identifier, opendds_identifier, return nullptr)

  auto node_info = static_cast<OpenDDSNodeInfo *>(node->data);
  RMW_CHECK_FOR_NULL_WITH_MSG(node_info, "node_info is null", return nullptr);
  RMW_CHECK_FOR_NULL_WITH_MSG(node_info->participant.in(), "participant is null", return nullptr);
  RMW_CHECK_FOR_NULL_WITH_MSG(node_info->subscriber_listener, "subscriber_listener is null", return nullptr);

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
  if (ts->register_type(node_info->participant.in(), type_name.c_str()) != DDS::RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to register OpenDDS type");
    return nullptr;
  }

  std::string topic_str = get_topic_str(topic_name, qos_profile->avoid_ros_namespace_conventions);
  if (topic_str.empty()) {
    RMW_SET_ERROR_MSG("subscription topic_name is null or empty");
    return nullptr;
  }
  if (!qos_profile) {
    RMW_SET_ERROR_MSG("qos_profile is null");
    return nullptr;
  }

  // find or create DDS::Topic
  DDS::TopicDescription_var topic_des = node_info->participant->lookup_topicdescription(topic_str.c_str());
  DDS::Topic_var topic = topic_des ?
    node_info->participant->find_topic(topic_str.c_str(), DDS::Duration_t{0, 0}) :
    node_info->participant->create_topic(topic_str.c_str(), type_name.c_str(), TOPIC_QOS_DEFAULT, NULL, OpenDDS::DCPS::NO_STATUS_MASK);
  if (!topic) {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("failed to %s topic", (topic_des ? "find" : "create"));
    return nullptr;
  }

  rmw_subscription_t * subscription = nullptr;
  void * buf = nullptr;
  try {
    // create rmw_subscription_t
    subscription = rmw_subscription_allocate();
    if (!subscription) {
      throw std::string("failed to allocate subscription");
    }
    subscription->implementation_identifier = opendds_identifier;
    subscription->data = nullptr;
    subscription->topic_name = reinterpret_cast<const char*>(rmw_allocate(strlen(topic_name) + 1));
    if (!subscription->topic_name) {
      throw std::string("failed to allocate memory for node name");
    }
    memcpy(const_cast<char*>(subscription->topic_name), topic_name, strlen(topic_name) + 1);
    if (subscription_options) {
      subscription->options = *subscription_options;
    }
    subscription->can_loan_messages = false;

    // create OpenDDSStaticSubscriberInfo
    buf = rmw_allocate(sizeof(OpenDDSStaticSubscriberInfo));
    if (!buf) {
      throw std::string("failed to allocate memory for subscriber info");
    }
    OpenDDSStaticSubscriberInfo * subscriber_info = nullptr;
    RMW_TRY_PLACEMENT_NEW(subscriber_info, buf, throw 1, OpenDDSStaticSubscriberInfo,)
    subscription->data = subscriber_info;
    buf = nullptr;

    // create OpenDDSSubscriberListener
    buf = rmw_allocate(sizeof(OpenDDSSubscriberListener));
    if (!buf) {
      throw std::string("failed to allocate memory for subscriber listener");
    }
    RMW_TRY_PLACEMENT_NEW(subscriber_info->listener_, buf, throw 1, OpenDDSSubscriberListener,)
    buf = nullptr;

    // create DDS::Subscriber
    subscriber_info->dds_subscriber_ = node_info->participant->create_subscriber(
      SUBSCRIBER_QOS_DEFAULT, subscriber_info->listener_, DDS::SUBSCRIPTION_MATCHED_STATUS);
    if (!subscriber_info->dds_subscriber_) {
      throw std::string("failed to create subscriber");
    }

    // create DDS::DataReader
    DDS::DataReaderQos dr_qos;
    if (!get_datareader_qos(subscriber_info->dds_subscriber_.in(), *qos_profile, dr_qos)) {
      throw std::string("get_datareader_qos failed");
    }
    subscriber_info->topic_reader_ = subscriber_info->dds_subscriber_->create_datareader(
      topic, dr_qos, NULL, OpenDDS::DCPS::NO_STATUS_MASK);
    if (!subscriber_info->topic_reader_) {
      throw std::string("failed to create datareader");
    }

    // create read_condition
    subscriber_info->read_condition_ = subscriber_info->topic_reader_->create_readcondition(
      DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
    if (!subscriber_info->read_condition_) {
      throw std::string("failed to create read condition");
    }

    subscriber_info->ignore_local_publications = false;
    subscriber_info->callbacks_ = callbacks;

    // update node_info
    std::string mangled_name;
    if (qos_profile->avoid_ros_namespace_conventions) {
      mangled_name = topic_name;
    } else {
      DDS::TopicDescription_var topic_des = subscriber_info->topic_reader_->get_topicdescription();
      if (topic_des) {
        CORBA::String_var name = topic_des->get_name();
        if (name) { mangled_name = *name; }
      }
      if (mangled_name.empty()) {
        throw std::string("failed to get topic name");
      }
    }
    node_info->subscriber_listener->add_information(
      node_info->participant->get_instance_handle(),
      subscriber_info->dds_subscriber_->get_instance_handle(),
      mangled_name, type_name, EntityType::Subscriber);
    node_info->subscriber_listener->trigger_graph_guard_condition();

    return subscription;

  } catch (const std::string& e) {
    RMW_SET_ERROR_MSG(e.c_str());
  } catch (...) {
    RMW_SET_ERROR_MSG("rmw_create_subscription failed");
  }
  clean_subscription(subscription, *node_info, *node_info->participant);
  if (buf) {
    rmw_free(buf);
  }
  return nullptr;
}

rmw_ret_t
rmw_subscription_count_matched_publishers(
  const rmw_subscription_t * subscription,
  size_t * publisher_count)
{
  if (!subscription) {
    RMW_SET_ERROR_MSG("subscription handle is null");
    return RMW_RET_INVALID_ARGUMENT;
  }

  if (!publisher_count) {
    RMW_SET_ERROR_MSG("publisher_count is null");
    return RMW_RET_INVALID_ARGUMENT;
  }

  auto info = static_cast<OpenDDSStaticSubscriberInfo *>(subscription->data);
  if (!info) {
    RMW_SET_ERROR_MSG("subscriber internal data is invalid");
    return RMW_RET_ERROR;
  }
  if (!info->listener_) {
    RMW_SET_ERROR_MSG("subscriber internal listener is invalid");
    return RMW_RET_ERROR;
  }

  *publisher_count = info->listener_->current_count();

  return RMW_RET_OK;
}

rmw_ret_t
rmw_subscription_get_actual_qos(
  const rmw_subscription_t * subscription,
  rmw_qos_profile_t * qos)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(subscription, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(subscription handle,
    subscription->implementation_identifier, opendds_identifier, return RMW_RET_ERROR)
  RMW_CHECK_ARGUMENT_FOR_NULL(qos, RMW_RET_INVALID_ARGUMENT);

  auto info = static_cast<OpenDDSStaticSubscriberInfo *>(subscription->data);
  RMW_CHECK_FOR_NULL_WITH_MSG(info, "subscriber info is null", return RMW_RET_ERROR);
  RMW_CHECK_FOR_NULL_WITH_MSG(info->topic_reader_, "topic_reader_ is null", return RMW_RET_ERROR);
  DDS::DataReaderQos dds_qos;
  if (info->topic_reader_->get_qos(dds_qos) == DDS::RETCODE_OK) {
    dds_qos_to_rmw_qos(dds_qos, *qos);
    return RMW_RET_OK;
  } else {
    RMW_SET_ERROR_MSG("subscription can't get data reader qos policies");
  }
  return RMW_RET_ERROR;
}

rmw_ret_t
rmw_destroy_subscription(rmw_node_t * node, rmw_subscription_t * subscription)
{
  RMW_CHECK_FOR_NULL_WITH_MSG(node, "node is null", return RMW_RET_ERROR);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(node, node->implementation_identifier,
    opendds_identifier, return RMW_RET_ERROR)

  auto node_info = static_cast<OpenDDSNodeInfo *>(node->data);
  RMW_CHECK_FOR_NULL_WITH_MSG(node_info, "node_info is null", return RMW_RET_ERROR);
  RMW_CHECK_FOR_NULL_WITH_MSG(node_info->participant.in(), "participant is null", return RMW_RET_ERROR);

  RMW_CHECK_FOR_NULL_WITH_MSG(subscription, "subscription is null", return RMW_RET_ERROR);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(subscription, subscription->implementation_identifier,
    opendds_identifier, return RMW_RET_ERROR)

  // TODO(wjwwood): need to figure out when to unregister types with the participant.
  return clean_subscription(subscription, *node_info, *node_info->participant);
}
}  // extern "C"
