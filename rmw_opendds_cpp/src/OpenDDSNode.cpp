// Copyright 2015-2017 Open Source Robotics Foundation, Inc.
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

#include "rmw_opendds_cpp/OpenDDSNode.hpp"

#include "rcutils/filesystem.h"

#include "rmw_opendds_cpp/guard_condition.hpp"
#include "rmw_opendds_cpp/opendds_include.hpp"
#include "rmw_opendds_cpp/init.hpp"
#include "rmw_opendds_cpp/node.hpp"
#include "rmw_opendds_cpp/types.hpp"
#include "rmw_opendds_cpp/identifier.hpp"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"

#include <dds/DdsDcpsCoreTypeSupportC.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/BuiltInTopicUtils.h>
#include <dds/DCPS/transport/framework/TransportRegistry.h>
#include <dds/DCPS/transport/framework/TransportConfig.h>
#include <dds/DCPS/transport/framework/TransportInst.h>
#include <dds/DCPS/transport/framework/TransportExceptions.h>
#ifdef OPENDDS_SECURITY
#include <dds/DCPS/security/framework/Properties.h>
#endif
#include <string>

OpenDDSNode* OpenDDSNode::from(const rmw_node_t * node)
{
  RMW_CHECK_FOR_NULL_WITH_MSG(node, "node is null", return nullptr);
  if (!check_impl_id(node->implementation_identifier)) {
    return nullptr;
  }
  auto dds_node = static_cast<OpenDDSNode *>(node->data);
  RMW_CHECK_FOR_NULL_WITH_MSG(dds_node, "dds_node is null", return nullptr);
  return dds_node;
}

void OpenDDSNode::add_pub(const DDS::InstanceHandle_t& pub, const std::string& topic_name, const std::string& type_name)
{
  DDS::GUID_t part_guid = dpi_->get_repoid(dp_->get_instance_handle());
  DDS::GUID_t guid = dpi_->get_repoid(pub);
  pub_listener_->add_information(part_guid, guid, topic_name, type_name, EntityType::Publisher);
  pub_listener_->trigger_graph_guard_condition();
}

void OpenDDSNode::add_sub(const DDS::InstanceHandle_t& sub, const std::string& topic_name, const std::string& type_name)
{
  DDS::GUID_t part_guid = dpi_->get_repoid(dp_->get_instance_handle());
  DDS::GUID_t guid = dpi_->get_repoid(sub);
  sub_listener_->add_information(part_guid, guid, topic_name, type_name, EntityType::Subscriber);
  sub_listener_->trigger_graph_guard_condition();
}

bool OpenDDSNode::remove_pub(const DDS::InstanceHandle_t& pub)
{
  DDS::GUID_t guid = dpi_->get_repoid(pub);
  bool r = pub_listener_->remove_information(guid, EntityType::Publisher);
  if (r) {
    pub_listener_->trigger_graph_guard_condition();
  } else {
    RMW_SET_ERROR_MSG("pub_listener_->remove_information failed");
  }
  return r;
}

bool OpenDDSNode::remove_sub(const DDS::InstanceHandle_t& sub)
{
  DDS::GUID_t guid = dpi_->get_repoid(sub);
  bool r = sub_listener_->remove_information(guid, EntityType::Subscriber);
  if (r) {
    sub_listener_->trigger_graph_guard_condition();
  } else {
    RMW_SET_ERROR_MSG("sub_listener_->remove_information failed");
  }
  return r;
}

rmw_ret_t OpenDDSNode::count_publishers(const char * topic_name, size_t * count)
{
  if (!topic_name) {
    RMW_SET_ERROR_MSG("topic name is null");
    return RMW_RET_ERROR;
  }
  if (!count) {
    RMW_SET_ERROR_MSG("count is null");
    return RMW_RET_ERROR;
  }
  *count = pub_listener_->count_topic(topic_name);
  return RMW_RET_OK;
}

rmw_ret_t OpenDDSNode::count_subscribers(const char * topic_name, size_t * count)
{
  if (!topic_name) {
    RMW_SET_ERROR_MSG("topic name is null");
    return RMW_RET_ERROR;
  }
  if (!count) {
    RMW_SET_ERROR_MSG("count is null");
    return RMW_RET_ERROR;
  }
  *count = sub_listener_->count_topic(topic_name);
  return RMW_RET_OK;
}

OpenDDSNode::OpenDDSNode(rmw_context_t & context)
  : context_(context)
  , gc_(nullptr)
  , pub_listener_(nullptr)
  , sub_listener_(nullptr)
  , dp_()
  , dpi_(nullptr)
{
  try {
    gc_ = create_guard_condition();
    if (!gc_) {
      throw std::runtime_error("create_guard_condition failed");
    }
    pub_listener_ = CustomPublisherListener::Raf::create(gc_);
    if (!pub_listener_) {
      throw std::runtime_error("CustomPublisherListener failed");
    }
    sub_listener_ = CustomSubscriberListener::Raf::create(gc_);
    if (!sub_listener_) {
      throw std::runtime_error("CustomSubscriberListener failed");
    }
    dp_ = context.impl->dpf_->create_participant(
      static_cast<DDS::DomainId_t>(context.options.domain_id), PARTICIPANT_QOS_DEFAULT, 0, 0);
    if (!dp_) {
      throw std::runtime_error("create_participant failed");
    }
    dpi_ = dynamic_cast<OpenDDS::DCPS::DomainParticipantImpl*>(dp_.in());
    if (!dpi_) {
      throw std::runtime_error("casting to DomainParticipantImpl failed");
    }

    if (!configureTransport()) {
      throw std::runtime_error("configureTransport failed");
    }

    DDS::Subscriber_ptr sub = dp_->get_builtin_subscriber();
    if (!sub) {
      throw std::runtime_error("get_builtin_subscriber failed");
    }
    // setup publisher listener
    DDS::DataReader_ptr dr = sub->lookup_datareader(OpenDDS::DCPS::BUILT_IN_PUBLICATION_TOPIC);
    auto pub_dr = dynamic_cast<DDS::PublicationBuiltinTopicDataDataReader*>(dr);
    if (!pub_dr) {
      throw std::runtime_error("builtin publication datareader is null");
    }
    pub_dr->set_listener(pub_listener_, DDS::DATA_AVAILABLE_STATUS);

    // setup subscriber listener
    dr = sub->lookup_datareader(OpenDDS::DCPS::BUILT_IN_SUBSCRIPTION_TOPIC);
    auto sub_dr = dynamic_cast<DDS::SubscriptionBuiltinTopicDataDataReader*>(dr);
    if (!sub_dr) {
      throw std::runtime_error("builtin subscription datareader is null");
    }
    sub_dr->set_listener(sub_listener_, DDS::DATA_AVAILABLE_STATUS);

  } catch (const std::exception& e) {
    RMW_SET_ERROR_MSG(e.what());
    cleanup();
    throw;
  } catch (...) {
    RMW_SET_ERROR_MSG("OpenDDSNode constructor failed");
    cleanup();
    throw;
  }
}

void OpenDDSNode::cleanup()
{
  if (dp_) {
    if (dp_->delete_contained_entities() != DDS::RETCODE_OK) {
      RMW_SET_ERROR_MSG("dp_->delete_contained_entities failed");
    }
    if (context_.impl && context_.impl->dpf_) {
      if (context_.impl->dpf_->delete_participant(dp_) != DDS::RETCODE_OK) {
        RMW_SET_ERROR_MSG("delete_participant failed");
      }
    } else {
      RMW_SET_ERROR_MSG("context_.impl is null or invalid");
    }
    dp_ = nullptr;
  }

  CustomSubscriberListener::Raf::destroy(sub_listener_);
  CustomPublisherListener::Raf::destroy(pub_listener_);

  if (gc_) {
    if (destroy_guard_condition(gc_) != RMW_RET_OK) {
      RMW_SET_ERROR_MSG("destroy_guard_condition failed");
    }
    gc_ = nullptr;
  }
}

bool OpenDDSNode::configureTransport()
{
  try {
    static int i = 0;
    const Guard guard(lock_);
    std::string str_domain_id = std::to_string(context_.options.domain_id) + "_" + std::to_string(++i);
    std::string config_name = "cfg" + str_domain_id;
    std::string inst_name = "rtps" + str_domain_id;
    OpenDDS::DCPS::TransportInst_rch inst = TheTransportRegistry->get_inst(inst_name);
    if (inst.is_nil()) {
      inst = TheTransportRegistry->create_inst(inst_name, "rtps_udp");
    }
    OpenDDS::DCPS::TransportConfig_rch cfg = TheTransportRegistry->get_config(config_name);
    if (cfg.is_nil()) {
      cfg = TheTransportRegistry->create_config(config_name);
    }
    cfg->instances_.push_back(inst);
    TheTransportRegistry->bind_config(cfg, dp_.in());
    return true;
  } catch (const OpenDDS::DCPS::Transport::Exception& e) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: %C%m\n"), typeid(e).name()), false);
  } catch (const CORBA::Exception& e) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: %C%m\n"), e._info().c_str()), false);
  } catch (...) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: in configureTransport()%m\n")), false);
  }
}
