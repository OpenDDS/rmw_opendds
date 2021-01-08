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

#include <string>

#include "rcutils/filesystem.h"

#include "rmw_opendds_shared_cpp/guard_condition.hpp"
#include "rmw_opendds_shared_cpp/opendds_include.hpp"
#include "rmw_opendds_shared_cpp/init.hpp"
#include "rmw_opendds_shared_cpp/node.hpp"
#include "rmw_opendds_shared_cpp/types.hpp"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"

#include "dds/DdsDcpsCoreTypeSupportC.h"
#include <dds/DCPS/Marked_Default_Qos.h>
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/BuiltInTopicUtils.h"
#include <dds/DCPS/transport/framework/TransportRegistry.h>
#include <dds/DCPS/transport/framework/TransportConfig.h>
#include <dds/DCPS/transport/framework/TransportInst.h>
#include <dds/DCPS/transport/framework/TransportExceptions.h>
#ifdef OPENDDS_SECURITY
#include <dds/DCPS/security/framework/Properties.h>
#endif

void append(DDS::PropertySeq& props, const char* name, const char* value, bool propagate = false) {
  const DDS::Property_t prop = {name, value, propagate};
  const unsigned int len = props.length();
  props.length(len + 1);
  props[len] = prop;
}

#ifdef OPENDDS_SECURITY
bool enable_security(const char * root, DDS::PropertySeq& props) {
  try {
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    char * buf = rcutils_join_path(root, "identity_ca.cert.pem", allocator);
    if (buf) {
      append(props, DDS::Security::Properties::AuthIdentityCA, buf);
      allocator.deallocate(buf, allocator.state);
      buf = nullptr;
    } else { throw std::string("failed to allocate memory for identity_ca_cert_fn"); }

    buf = rcutils_join_path(root, "permissions_ca.cert.pem", allocator);
    if (buf) {
      append(props, DDS::Security::Properties::AccessPermissionsCA, buf);
      allocator.deallocate(buf, allocator.state);
      buf = nullptr;
    } else { throw std::string("failed to allocate memory for permissions_ca_cert_fn"); }

    buf = rcutils_join_path(root, "cert.pem", allocator);
    if (buf) {
      append(props, DDS::Security::Properties::AuthIdentityCertificate, buf);
      allocator.deallocate(buf, allocator.state);
      buf = nullptr;
    } else { throw std::string("failed to allocate memory for cert_fn"); }

    buf = rcutils_join_path(root, "key.pem", allocator);
    if (buf) {
      append(props, DDS::Security::Properties::AuthPrivateKey, buf);
      allocator.deallocate(buf, allocator.state);
      buf = nullptr;
    } else { throw std::string("failed to allocate memory for key_fn"); }

    buf = rcutils_join_path(root, "governance.p7s", allocator);
    if (buf) {
      append(props, DDS::Security::Properties::AccessGovernance, buf);
      allocator.deallocate(buf, allocator.state);
      buf = nullptr;
    } else { throw std::string("failed to allocate memory for gov_fn"); }

    buf = rcutils_join_path(root, "permissions.p7s", allocator);
    if (buf) {
      append(props, DDS::Security::Properties::AccessPermissions, buf);
      allocator.deallocate(buf, allocator.state);
      buf = nullptr;
    } else { throw std::string("failed to allocate memory for perm_fn"); }

    return true;

  } catch (const std::string& e) {
    RMW_SET_ERROR_MSG(e.c_str());
  } catch (...) {}
  return false;
}
#endif

typedef std::mutex LockType;
typedef std::lock_guard<std::mutex> GuardType;

bool configureTransport(DDS::DomainParticipant_ptr participant, size_t domain_id)
{
  try {
    static int i = 0;
    static LockType lock;
    const GuardType guard(lock);
    std::string str_domain_id = std::to_string(domain_id) + "_" + std::to_string(++i);
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
    TheTransportRegistry->bind_config(cfg, participant);
    return true;
  } catch (const OpenDDS::DCPS::Transport::Exception& e) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: %C%m\n"), typeid(e).name()), false);
  } catch (const CORBA::Exception& e) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: %C%m\n"), e._info().c_str()), false);
  } catch (...) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: in configureTransport()%m\n")), false);
  }
}

OpenDDSNode* OpenDDSNode::get_from(const rmw_node_t * node, const char * implementation_identifier)
{
  RMW_CHECK_FOR_NULL_WITH_MSG(node, "node is null", return nullptr);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(node, node->implementation_identifier, implementation_identifier, return nullptr)
  auto dds_node = static_cast<OpenDDSNode *>(node->data);
  RMW_CHECK_FOR_NULL_WITH_MSG(dds_node, "dds_node is null", return nullptr);
  return dds_node;
}

DDS::DomainParticipant_var OpenDDSNode::create_dp(rmw_context_t& context)
{
  DDS::DomainParticipant_var dp = context.impl->dpf_->create_participant(
    static_cast<DDS::DomainId_t>(context.options.domain_id), PARTICIPANT_QOS_DEFAULT, 0, 0);
  if (!dp) {
    RMW_SET_ERROR_MSG("create_participant failed");
  }
  return dp;
}

void OpenDDSNode::cleanup()
{
  if (dp_) {
    if (dp_->delete_contained_entities() != DDS::RETCODE_OK) {
      RMW_SET_ERROR_MSG("dp_->delete_contained_entities failed");
    }
    if (context_.impl->dpf_->delete_participant(dp_) != DDS::RETCODE_OK) {
      RMW_SET_ERROR_MSG("delete_participant failed");
    }
    dp_ = nullptr;
  }

  CustomSubscriberListener::Raf::destroy(sub_listener_);
  CustomPublisherListener::Raf::destroy(pub_listener_);

  if (gc_) {
    if (destroy_guard_condition(context_.implementation_identifier, gc_) != RMW_RET_OK) {
      RMW_SET_ERROR_MSG("destroy_guard_condition failed");
    }
    gc_ = nullptr;
  }
}

OpenDDSNode::OpenDDSNode(rmw_context_t& context)
  : context_(context)
  , gc_(create_guard_condition(context_.implementation_identifier))
  , pub_listener_(gc_ ? CustomPublisherListener::Raf::create(context_.implementation_identifier, gc_) : 0)
  , sub_listener_(pub_listener_ ? CustomSubscriberListener::Raf::create(context_.implementation_identifier, gc_) : 0)
  , dp_(sub_listener_ ? create_dp(context_) : 0)
{
  try {
    if (!dp_) throw std::runtime_error("OpenDDSNode failed");

    if (!configureTransport(dp_, context_.options.domain_id)) {
      throw std::runtime_error("configureTransport failed");
    }

    DDS::Subscriber_ptr sub = dp_->get_builtin_subscriber();
    if (!sub) throw std::runtime_error("get_builtin_subscriber failed");

    // setup publisher listener
    DDS::DataReader_ptr dr = sub->lookup_datareader(OpenDDS::DCPS::BUILT_IN_PUBLICATION_TOPIC);
    auto pub_dr = dynamic_cast<DDS::PublicationBuiltinTopicDataDataReader*>(dr);
    if (!pub_dr) throw std::runtime_error("builtin publication datareader is null");
    pub_dr->set_listener(pub_listener_, DDS::DATA_AVAILABLE_STATUS);

    // setup subscriber listener
    dr = sub->lookup_datareader(OpenDDS::DCPS::BUILT_IN_SUBSCRIPTION_TOPIC);
    auto sub_dr = dynamic_cast<DDS::SubscriptionBuiltinTopicDataDataReader*>(dr);
    if (!sub_dr) throw std::runtime_error("builtin subscription datareader is null");
    sub_dr->set_listener(sub_listener_, DDS::DATA_AVAILABLE_STATUS);

  } catch (const std::exception& e) {
    RMW_SET_ERROR_MSG(e.what());
    cleanup();
    throw;
  } catch (...) {
    RMW_SET_ERROR_MSG("OpenDDSNode construtor failed");
    cleanup();
    throw;
  }
}

bool set_default_participant_qos(rmw_context_t & context, const char * name, const char * name_space)
{
  DDS::DomainParticipantQos qos;
  if (context.impl->dpf_->get_default_participant_qos(qos) != DDS::RETCODE_OK) {
    RMW_SET_ERROR_MSG("get_default_participant_qos failed");
    return false;
  }
  // since participant name is not part of DDS spec, set node name in user_data
  size_t length = strlen(name) + strlen("name=;") + strlen(name_space) + strlen("namespace=;") + 1;
  qos.user_data.value.length(static_cast<CORBA::Long>(length));
  int written = snprintf(reinterpret_cast<char *>(qos.user_data.value.get_buffer()),
                         length, "name=%s;namespace=%s;", name, name_space);
  if (written < 0 || written > static_cast<int>(length) - 1) {
    RMW_SET_ERROR_MSG("failed to populate user_data buffer");
    return false;
  }
  if (context.impl->dpf_->set_default_participant_qos(qos) != DDS::RETCODE_OK) {
    RMW_SET_ERROR_MSG("set_default_participant_qos failed");
    return false;
  }
  return true;
}

void clean_node(rmw_node_t * node)
{
  if (!node) {
    return;
  }
  RmwStr::del(node->namespace_);
  RmwStr::del(node->name);
  auto dds_node = static_cast<OpenDDSNode*>(node->data);
  if (dds_node) {
    OpenDDSNode::Raf::destroy(dds_node);
    node->data = nullptr;
  }
  node->implementation_identifier = nullptr;
  node->context = nullptr;
  rmw_node_free(node);
}

// caller checks context (implementation_identifier, impl), name, name_space
rmw_node_t *
create_node(rmw_context_t & context, const char * name, const char * name_space)
{
  if (!set_default_participant_qos(context, name, name_space)) {
    return nullptr;
  }
  rmw_node_t * node = nullptr;
  try {
    node = rmw_node_allocate();
    if (!node) {
      throw std::runtime_error("rmw_node_allocate failed");
    }
    node->context = &context;
    node->implementation_identifier = context.implementation_identifier;
    node->data = OpenDDSNode::Raf::create(context);
    if (!node->data) {
      throw std::runtime_error("OpenDDSNode failed");
    }
    node->name = RmwStr::cpy(name);
    if (!node->name) {
      throw std::runtime_error("node->name failed");
    }
    node->namespace_ = RmwStr::cpy(name_space);
    if (!node->namespace_) {
      throw std::runtime_error("node->namespace_ failed");
    }
    return node;

  } catch (const std::exception& e) {
    RMW_SET_ERROR_MSG(e.what());
  } catch (...) {
    RMW_SET_ERROR_MSG("create_node unkown exception");
  }
  clean_node(node);
  return nullptr;
}

// caller checks implementation_identifier
rmw_ret_t
destroy_node(rmw_node_t * node)
{
  clean_node(node);
  return RMW_RET_OK;
}

RMW_OPENDDS_SHARED_CPP_PUBLIC
const rmw_guard_condition_t *
node_get_graph_guard_condition(const rmw_node_t * node)
{
  // node argument is checked in calling function.
  OpenDDSNode * dds_node = static_cast<OpenDDSNode *>(node->data);
  if (!dds_node) {
    RMW_SET_ERROR_MSG("dds_node is null");
    return nullptr;
  }
  return dds_node->gc_;
}

RMW_OPENDDS_SHARED_CPP_PUBLIC
rmw_ret_t
node_assert_liveliness(const rmw_node_t * node)
{
  auto dds_node = static_cast<OpenDDSNode *>(node->data);
  if (!dds_node) {
    RMW_SET_ERROR_MSG("dds_node is null");
    return RMW_RET_ERROR;
  }
  if (!dds_node->dp_) {
    RMW_SET_ERROR_MSG("dds_node participant is null");
    return RMW_RET_ERROR;
  }
  if (dds_node->dp_->assert_liveliness() != DDS::RETCODE_OK) {
   RMW_SET_ERROR_MSG("failed to assert liveliness of participant");
   return RMW_RET_ERROR;
  }
  return RMW_RET_OK;
}
