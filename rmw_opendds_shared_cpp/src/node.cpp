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
#include "rmw_opendds_shared_cpp/node.hpp"
#include "rmw_opendds_shared_cpp/types.hpp"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"

#include "dds/DdsDcpsCoreTypeSupportC.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/BuiltInTopicUtils.h"
#include <dds/DCPS/transport/framework/TransportRegistry.h>
#include <dds/DCPS/transport/framework/TransportConfig.h>
#include <dds/DCPS/transport/framework/TransportInst.h>
#include <dds/DCPS/transport/framework/TransportExceptions.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
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

void clean_node(DDS::DomainParticipantFactory * dpf, rmw_node_t * node)
{
  if (!node) {
    return;
  }
  if (node->name) {
    rmw_free(const_cast<char*>(node->name));
    node->name = nullptr;
  }
  if (node->namespace_) {
    rmw_free(const_cast<char*>(node->namespace_));
    node->namespace_ = nullptr;
  }
  //node->context
  auto info = static_cast<OpenDDSNodeInfo*>(node->data);
  if (info) {
    if (info->participant) {
      // unregisters types and destroys topics shared between publishers and subscribers
      if (info->participant->delete_contained_entities() != DDS::RETCODE_OK) {
        RMW_SET_ERROR_MSG("failed to delete contained entities of participant");
      }
      if (dpf->delete_participant(info->participant) != DDS::RETCODE_OK) {
        RMW_SET_ERROR_MSG("failed to delete participant");
      }
      info->participant = nullptr;
    }
    if (info->publisher_listener) {
      RMW_TRY_DESTRUCTOR_FROM_WITHIN_FAILURE(
        info->publisher_listener->~CustomPublisherListener(), CustomPublisherListener)
      rmw_free(info->publisher_listener);
      info->publisher_listener = nullptr;
    }
    if (info->subscriber_listener) {
      RMW_TRY_DESTRUCTOR_FROM_WITHIN_FAILURE(
        info->subscriber_listener->~CustomSubscriberListener(), CustomSubscriberListener)
      rmw_free(info->subscriber_listener);
      info->subscriber_listener = nullptr;
    }
    if (info->graph_guard_condition) {
      if (destroy_guard_condition(node->implementation_identifier, info->graph_guard_condition) != RMW_RET_OK) {
        RMW_SET_ERROR_MSG("failed to delete graph guard condition");
      }
      info->graph_guard_condition = nullptr;
    }
    RMW_TRY_DESTRUCTOR_FROM_WITHIN_FAILURE(info->~OpenDDSNodeInfo(), OpenDDSNodeInfo)
    rmw_free(info);
    node->data = nullptr;
  }
  rmw_node_free(node);
}

rmw_node_t *
create_node(
  const char * implementation_identifier,
  const char * name,
  const char * namespace_,
  size_t domain_id)
{
  DDS::DomainParticipantFactory * dpf = TheParticipantFactory;
  if (!dpf) {
    RMW_SET_ERROR_MSG("failed to get participant factory");
    return nullptr;
  }

  // use loopback interface to enable cross vendor communication
  DDS::DomainParticipantQos qos;
  if (dpf->get_default_participant_qos(qos) != DDS::RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to get default participant qos");
    return nullptr;
  }
  // since participant name is not part of DDS spec, set node name in user_data
  size_t length = strlen(name) + strlen("name=;") + strlen(namespace_) + strlen("namespace=;") + 1;
  qos.user_data.value.length(static_cast<CORBA::Long>(length));
  int written = snprintf(reinterpret_cast<char *>(qos.user_data.value.get_buffer()),
      length, "name=%s;namespace=%s;", name, namespace_);
  if (written < 0 || written > static_cast<int>(length) - 1) {
    RMW_SET_ERROR_MSG("failed to populate user_data buffer");
    return nullptr;
  }

  rmw_node_t * node = nullptr;
  void * buf = nullptr;
  try {
    node = rmw_node_allocate();
    if (!node) {
      throw std::string("failed to allocate memory for node");
    }
    node->implementation_identifier = implementation_identifier;
    node->data = nullptr;

    node->name = reinterpret_cast<const char *>(rmw_allocate(sizeof(char) * strlen(name) + 1));
    if (!node->name) {
      throw std::string("failed to allocate memory for node name");
    }
    memcpy(const_cast<char *>(node->name), name, strlen(name) + 1);

    node->namespace_ = reinterpret_cast<const char *>(rmw_allocate(sizeof(char) * strlen(namespace_) + 1));
    if (!node->namespace_) {
      throw std::string("failed to allocate memory for node namespace");
    }
    memcpy(const_cast<char *>(node->namespace_), namespace_, strlen(namespace_) + 1);

    buf = rmw_allocate(sizeof(OpenDDSNodeInfo));
    if (!buf) {
      throw std::string("failed to allocate memory for OpenDDSNodeInfo");
    }
    OpenDDSNodeInfo * info = nullptr;
    RMW_TRY_PLACEMENT_NEW(info, buf, throw 1, OpenDDSNodeInfo,)
    node->data = info;
    buf = nullptr;

    OpenDDS::DCPS::Discovery_rch disco = TheServiceParticipant->get_discovery(domain_id);
    OpenDDS::RTPS::RtpsDiscovery_rch rtps_disco = OpenDDS::DCPS::dynamic_rchandle_cast<OpenDDS::RTPS::RtpsDiscovery>(disco);
    rtps_disco->use_xtypes(false);

    info->participant = dpf->create_participant(static_cast<DDS::DomainId_t>(domain_id), qos, 0, 0);
    if (!info->participant) {
      throw std::string("failed to create participant");
    }

    if (!configureTransport(info->participant, domain_id)) {
      throw std::string("failed to configureTransport");
    }

    info->graph_guard_condition = create_guard_condition(implementation_identifier);
    if (!info->graph_guard_condition) {
      throw std::string("failed to create graph guard condition");
    }

    DDS::Subscriber * sub = info->participant->get_builtin_subscriber();
    if (!sub) {
      throw std::string("builtin subscriber is null");
    }

    // setup publisher listener
    DDS::DataReader * dr = sub->lookup_datareader(OpenDDS::DCPS::BUILT_IN_PUBLICATION_TOPIC);
    DDS::PublicationBuiltinTopicDataDataReader * pub_dr = dynamic_cast<DDS::PublicationBuiltinTopicDataDataReader*>(dr);
    if (!pub_dr) {
      throw std::string("builtin publication datareader is null");
    }
    buf = rmw_allocate(sizeof(CustomPublisherListener));
    if (!buf) {
      throw std::string("failed to allocate memory for CustomPublisherListener");
    }
    RMW_TRY_PLACEMENT_NEW(info->publisher_listener, buf, throw 1, CustomPublisherListener,
      implementation_identifier, info->graph_guard_condition)
    buf = nullptr;
    pub_dr->set_listener(info->publisher_listener, DDS::DATA_AVAILABLE_STATUS);

    // setup subscriber listener
    dr = sub->lookup_datareader(OpenDDS::DCPS::BUILT_IN_SUBSCRIPTION_TOPIC);
    DDS::SubscriptionBuiltinTopicDataDataReader * sub_dr = dynamic_cast<DDS::SubscriptionBuiltinTopicDataDataReader*>(dr);
    if (!sub_dr) {
      throw std::string("builtin subscription datareader is null");
    }
    buf = rmw_allocate(sizeof(CustomSubscriberListener));
    if (!buf) {
      throw std::string("failed to allocate memory for CustomSubscriberListener");
    }
    RMW_TRY_PLACEMENT_NEW(info->subscriber_listener, buf, throw 1, CustomSubscriberListener,
      implementation_identifier, info->graph_guard_condition)
    buf = nullptr;
    sub_dr->set_listener(info->subscriber_listener, DDS::DATA_AVAILABLE_STATUS);

    return node;

  } catch (const std::string& e) {
    RMW_SET_ERROR_MSG(e.c_str());
  } catch (...) {}

  clean_node(dpf, node);
  if (buf) {
    rmw_free(buf);
  }
  return nullptr;
}

rmw_ret_t
destroy_node(const char * implementation_identifier, rmw_node_t * node)
{
  if (!node) {
    RMW_SET_ERROR_MSG("node handle is null");
    return RMW_RET_ERROR;
  }
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    node handle,
    node->implementation_identifier, implementation_identifier,
    return RMW_RET_ERROR)

  DDS::DomainParticipantFactory * dpf = TheParticipantFactory;
  if (!dpf) {
    RMW_SET_ERROR_MSG("failed to get participant factory");
    return RMW_RET_ERROR;
  }

  clean_node(dpf, node);
  return RMW_RET_OK;
}

RMW_OPENDDS_SHARED_CPP_PUBLIC
const rmw_guard_condition_t *
node_get_graph_guard_condition(const rmw_node_t * node)
{
  // node argument is checked in calling function.
  OpenDDSNodeInfo * node_info = static_cast<OpenDDSNodeInfo *>(node->data);
  if (!node_info) {
    RMW_SET_ERROR_MSG("node info handle is null");
    return nullptr;
  }

  return node_info->graph_guard_condition;
}

RMW_OPENDDS_SHARED_CPP_PUBLIC
rmw_ret_t
node_assert_liveliness(const rmw_node_t * node)
{
  auto node_info = static_cast<OpenDDSNodeInfo *>(node->data);
  if (nullptr == node_info) {
    RMW_SET_ERROR_MSG("node info is null");
    return RMW_RET_ERROR;
  }
  if (nullptr == node_info->participant) {
    RMW_SET_ERROR_MSG("node info participant is null");
    return RMW_RET_ERROR;
  }
  if (node_info->participant->assert_liveliness() != DDS::RETCODE_OK) {
   RMW_SET_ERROR_MSG("failed to assert liveliness of participant");
   return RMW_RET_ERROR;
  }
  return RMW_RET_OK;
}
