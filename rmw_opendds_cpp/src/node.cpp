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

#include "rmw_opendds_cpp/node.hpp"
#include "rmw_opendds_cpp/init.hpp"
#include "rmw_opendds_cpp/identifier.hpp"
#include "rmw_opendds_cpp/OpenDDSNode.hpp"
#include "rmw_opendds_cpp/guard_condition.hpp"
#include "rmw_opendds_cpp/opendds_include.hpp"
#include "rmw_opendds_cpp/types.hpp"

#include "rmw/allocators.h"
#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"

#include "rcutils/filesystem.h"

#include "dds/DdsDcpsCoreTypeSupportC.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/BuiltInTopicUtils.h"
#include <dds/DCPS/transport/framework/TransportRegistry.h>
#include <dds/DCPS/transport/framework/TransportConfig.h>
#include <dds/DCPS/transport/framework/TransportInst.h>
#include <dds/DCPS/transport/framework/TransportExceptions.h>
#ifdef OPENDDS_SECURITY
#include <dds/DCPS/security/framework/Properties.h>
#endif

#include <string>

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

bool set_default_participant_qos(rmw_context_t & context, const char * name, const char * namespace_)
{
  DDS::DomainParticipantQos qos;
  if (context.impl->dpf_->get_default_participant_qos(qos) != DDS::RETCODE_OK) {
    RMW_SET_ERROR_MSG("get_default_participant_qos failed");
    return false;
  }
  // since participant name is not part of DDS spec, set node name in user_data
  size_t length = strlen(name) + strlen("name=;") + strlen(namespace_) + strlen("namespace=;") + 1;
  qos.user_data.value.length(static_cast<CORBA::Long>(length));
  int written = snprintf(reinterpret_cast<char *>(qos.user_data.value.get_buffer()),
                         length, "name=%s;namespace=%s;", name, namespace_);
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

rmw_node_t *
create_node(rmw_context_t & context, const char * name, const char * namespace_)
{
  if (!set_default_participant_qos(context, name, namespace_)) {
    return nullptr;
  }
  rmw_node_t * node = nullptr;
  try {
    node = rmw_node_allocate();
    if (!node) {
      throw std::runtime_error("rmw_node_allocate failed");
    }
    node->context = &context;
    node->implementation_identifier = opendds_identifier;
    node->data = OpenDDSNode::Raf::create(context);
    if (!node->data) {
      throw std::runtime_error("OpenDDSNode failed");
    }
    node->name = RmwStr::cpy(name);
    if (!node->name) {
      throw std::runtime_error("node->name failed");
    }
    node->namespace_ = RmwStr::cpy(namespace_);
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

rmw_ret_t
destroy_node(rmw_node_t * node)
{
  if (node && node->context && node->context->impl) {
    clean_node(node);
    return RMW_RET_OK;
  }
  return RMW_RET_ERROR;
}
