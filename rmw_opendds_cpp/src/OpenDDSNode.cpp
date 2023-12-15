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

#include <dds/DCPS/RTPS/RtpsDiscoveryConfig.h>
#include <rmw_opendds_cpp/OpenDDSNode.hpp>
#include <rmw_opendds_cpp/demangle.hpp>
#include <rmw_opendds_cpp/identifier.hpp>
#include <rmw_opendds_cpp/init.hpp>
#include <rmw_opendds_cpp/types.hpp>

#include <dds/DCPS/BuiltInTopicUtils.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/transport/framework/TransportConfig.h>
#include <dds/DCPS/transport/framework/TransportExceptions.h>
#include <dds/DCPS/transport/framework/TransportInst.h>
#include <dds/DCPS/transport/framework/TransportRegistry.h>
#include <dds/DdsDcpsCoreTypeSupportC.h>
#ifdef OPENDDS_SECURITY
#include <dds/DCPS/security/framework/Properties.h>
#endif

#include <rcutils/filesystem.h>
#include <rcutils/strdup.h>

#include <rmw/allocators.h>
#include <rmw/convert_rcutils_ret_to_rmw_ret.h>
#include <rmw/error_handling.h>
#include <rmw/impl/cpp/key_value.hpp>
#include <rmw/impl/cpp/macros.hpp>
#include <rmw/sanity_checks.h>

OpenDDSNode *OpenDDSNode::from(const rmw_node_t *node) {
  RMW_CHECK_FOR_NULL_WITH_MSG(node, "node is null", return nullptr);
  if (!check_impl_id(node->implementation_identifier)) {
    return nullptr;
  }
  auto dds_node = static_cast<OpenDDSNode *>(node->data);
  RMW_CHECK_FOR_NULL_WITH_MSG(dds_node, "dds_node is null", return nullptr);
  return dds_node;
}

bool OpenDDSNode::validate_name_namespace(const char *node_name,
                                          const char *node_namespace) {
  if (!node_name || strlen(node_name) == 0) {
    RMW_SET_ERROR_MSG("node_name is null or empty");
    return false;
  }
  if (!node_namespace || strlen(node_namespace) == 0) {
    RMW_SET_ERROR_MSG("node_namespace is null or empty");
    return false;
  }
  return true;
}

void OpenDDSNode::add_pub(const DDS::InstanceHandle_t &pub,
                          const std::string &topic_name,
                          const std::string &type_name) {
  DDS::GUID_t part_guid = dpi_->get_repoid(dp_->get_instance_handle());
  DDS::GUID_t guid = dpi_->get_repoid(pub);
  pub_listener_->add_information(part_guid, guid, topic_name, type_name,
                                 EntityType::Publisher);
  pub_listener_->trigger_graph_guard_condition();
}

void OpenDDSNode::add_sub(const DDS::InstanceHandle_t &sub,
                          const std::string &topic_name,
                          const std::string &type_name) {
  DDS::GUID_t part_guid = dpi_->get_repoid(dp_->get_instance_handle());
  DDS::GUID_t guid = dpi_->get_repoid(sub);
  sub_listener_->add_information(part_guid, guid, topic_name, type_name,
                                 EntityType::Subscriber);
  sub_listener_->trigger_graph_guard_condition();
}

bool OpenDDSNode::remove_pub(const DDS::InstanceHandle_t &pub) {
  DDS::GUID_t guid = dpi_->get_repoid(pub);
  if (pub_listener_->remove_information(guid, EntityType::Publisher)) {
    pub_listener_->trigger_graph_guard_condition();
  }
  return true;
}

bool OpenDDSNode::remove_sub(const DDS::InstanceHandle_t &sub) {
  DDS::GUID_t guid = dpi_->get_repoid(sub);
  if (sub_listener_->remove_information(guid, EntityType::Subscriber)) {
    sub_listener_->trigger_graph_guard_condition();
  }
  return true;
}

rmw_ret_t OpenDDSNode::count_publishers(const char *topic_name, size_t *count) {
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

rmw_ret_t OpenDDSNode::count_subscribers(const char *topic_name,
                                         size_t *count) {
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

rmw_ret_t OpenDDSNode::get_names(rcutils_string_array_t *names,
                                 rcutils_string_array_t *namespaces,
                                 rcutils_string_array_t *enclaves) const {
  if (rmw_check_zero_rmw_string_array(names) != RMW_RET_OK) {
    RMW_SET_ERROR_MSG("rmw_check_zero_rmw_string_array(names) failed.");
    return RMW_RET_INVALID_ARGUMENT;
  }
  if (rmw_check_zero_rmw_string_array(namespaces) != RMW_RET_OK) {
    RMW_SET_ERROR_MSG("rmw_check_zero_rmw_string_array(namespaces) failed.");
    return RMW_RET_INVALID_ARGUMENT;
  }
  DDS::InstanceHandleSeq handles;
  if (dp_->get_discovered_participants(handles) != DDS::RETCODE_OK) {
    RMW_SET_ERROR_MSG("unable to fetch discovered participants.");
    return RMW_RET_ERROR;
  }
  const auto length = handles.length() + 1; // add yourself
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  try {
    if (rcutils_string_array_init(names, length, &allocator) !=
        RCUTILS_RET_OK) {
      throw std::runtime_error(rcutils_get_error_string().str);
    }
    if (rcutils_string_array_init(namespaces, length, &allocator) !=
        RCUTILS_RET_OK) {
      throw std::runtime_error(rcutils_get_error_string().str);
    }
    if (enclaves) {
      if (rcutils_string_array_init(enclaves, length, &allocator) !=
          RCUTILS_RET_OK) {
        throw std::runtime_error(rcutils_get_error_string().str);
      }
    }
    names->data[0] = rcutils_strdup(name_.c_str(), allocator);
    if (!names->data[0]) {
      throw std::runtime_error("could not allocate memory for node name");
    }
    namespaces->data[0] = rcutils_strdup(namespace_.c_str(), allocator);
    if (!namespaces->data[0]) {
      throw std::runtime_error("could not allocate memory for node namespace");
    }
    if (enclaves) {
      enclaves->data[0] = rcutils_strdup(context_.options.enclave, allocator);
      if (!enclaves->data[0]) {
        throw std::runtime_error(
            "could not allocate memory for a enclave name");
      }
    }
    for (CORBA::ULong i = 1; i < length; ++i) {
      DDS::ParticipantBuiltinTopicData pbtd;
      auto dds_ret = dp_->get_discovered_participant_data(pbtd, handles[i - 1]);
      std::string name;
      std::string ns;
      std::string enclave;
      if (DDS::RETCODE_OK == dds_ret) {
        auto data =
            static_cast<unsigned char *>(pbtd.user_data.value.get_buffer());
        std::vector<uint8_t> kv(data, data + pbtd.user_data.value.length());
        auto map = rmw::impl::cpp::parse_key_value(kv);
        auto name_found = map.find("name");
        auto ns_found = map.find("namespace");
        auto enclave_found = map.find("enclave");
        if (name_found != map.end()) {
          name =
              std::string(name_found->second.begin(), name_found->second.end());
        }
        if (name_found != map.end()) {
          ns = std::string(ns_found->second.begin(), ns_found->second.end());
        }
        if (enclave_found != map.end()) {
          enclave = std::string(enclave_found->second.begin(),
                                enclave_found->second.end());
        }
      }
      if (name.empty()) {
        // ignore discovered participants without a name
        names->data[i] = nullptr;
        namespaces->data[i] = nullptr;
        continue;
      }
      names->data[i] = rcutils_strdup(name.c_str(), allocator);
      if (!names->data[i]) {
        throw std::runtime_error("could not allocate memory for node name");
      }
      namespaces->data[i] = rcutils_strdup(ns.c_str(), allocator);
      if (!namespaces->data[i]) {
        throw std::runtime_error(
            "could not allocate memory for node namespace");
      }
      if (enclaves) {
        enclaves->data[i] = rcutils_strdup(enclave.c_str(), allocator);
        if (!enclaves->data[i]) {
          throw std::runtime_error(
              "could not allocate memory for enclave namespace");
        }
      }
    }
    return RMW_RET_OK;
  } catch (const std::exception &e) {
    RMW_SET_ERROR_MSG(e.what());
  } catch (...) {
    RMW_SET_ERROR_MSG("OpenDDSNode::get_names failed.");
  }
  if (names) {
    rcutils_string_array_fini(names);
  }
  if (namespaces) {
    rcutils_string_array_fini(namespaces);
  }
  if (enclaves) {
    rcutils_string_array_fini(enclaves);
  }
  return RMW_RET_ERROR;
}

rmw_ret_t
OpenDDSNode::get_pub_names_types(rmw_names_and_types_t *nt,
                                 const char *node_name,
                                 const char *node_namespace, bool no_demangle,
                                 rcutils_allocator_t *allocator) const {
  rmw_ret_t ret = rmw_names_and_types_check_zero(nt);
  if (ret != RMW_RET_OK) {
    return ret;
  }
  if (!validate_name_namespace(node_name, node_namespace)) {
    return RMW_RET_INVALID_ARGUMENT;
  }
  DDS::GUID_t key;
  auto get_guid_err = get_key(key, node_name, node_namespace);
  if (get_guid_err != RMW_RET_OK) {
    return get_guid_err;
  }
  NameTypeMap ntm;
  pub_listener_->fill_topic_names_and_types_by_guid(no_demangle, ntm, key);
  return copy_topic_names_types(nt, ntm, no_demangle, allocator);
}

rmw_ret_t
OpenDDSNode::get_sub_names_types(rmw_names_and_types_t *nt,
                                 const char *node_name,
                                 const char *node_namespace, bool no_demangle,
                                 rcutils_allocator_t *allocator) const {
  rmw_ret_t ret = rmw_names_and_types_check_zero(nt);
  if (ret != RMW_RET_OK) {
    return ret;
  }
  if (!validate_name_namespace(node_name, node_namespace)) {
    return RMW_RET_INVALID_ARGUMENT;
  }
  DDS::GUID_t key;
  auto get_guid_err = get_key(key, node_name, node_namespace);
  if (get_guid_err != RMW_RET_OK) {
    return get_guid_err;
  }
  NameTypeMap ntm;
  sub_listener_->fill_topic_names_and_types_by_guid(no_demangle, ntm, key);
  return copy_topic_names_types(nt, ntm, no_demangle, allocator);
}

rmw_ret_t
OpenDDSNode::get_topic_names_types(rmw_names_and_types_t *nt, bool no_demangle,
                                   rcutils_allocator_t *allocator) const {
  rmw_ret_t ret = rmw_names_and_types_check_zero(nt);
  if (ret != RMW_RET_OK) {
    return ret;
  }
  // combine publisher and subscriber information
  NameTypeMap ntm;
  pub_listener_->fill_topic_names_and_types(no_demangle, ntm);
  sub_listener_->fill_topic_names_and_types(no_demangle, ntm);
  return copy_topic_names_types(nt, ntm, no_demangle, allocator);
}

rmw_ret_t
OpenDDSNode::get_service_names_types(rmw_names_and_types_t *nt,
                                     rcutils_allocator_t *allocator) const {
  rmw_ret_t ret = rmw_names_and_types_check_zero(nt);
  if (ret != RMW_RET_OK) {
    return ret;
  }
  if (!allocator) {
    RMW_SET_ERROR_MSG("allocator is null");
    return RMW_RET_INVALID_ARGUMENT;
  }
  // combine publisher and subscriber information
  NameTypeMap ntm;
  pub_listener_->fill_service_names_and_types(ntm);
  sub_listener_->fill_service_names_and_types(ntm);
  return copy_service_names_types(nt, ntm, allocator);
}

rmw_ret_t OpenDDSNode::get_service_names_types(
    rmw_names_and_types_t *nt, const char *node_name,
    const char *node_namespace, const char *suffix,
    rcutils_allocator_t *allocator) const {
  rmw_ret_t ret = rmw_names_and_types_check_zero(nt);
  if (ret != RMW_RET_OK) {
    return ret;
  }
  if (!validate_name_namespace(node_name, node_namespace)) {
    return RMW_RET_INVALID_ARGUMENT;
  }
  if (!allocator) {
    RMW_SET_ERROR_MSG("allocator is null");
    return RMW_RET_INVALID_ARGUMENT;
  }
  DDS::GUID_t key;
  auto get_guid_err = get_key(key, node_name, node_namespace);
  if (get_guid_err != RMW_RET_OK) {
    return get_guid_err;
  }
  NameTypeMap ntm;
  sub_listener_->fill_service_names_and_types_by_guid(ntm, key, suffix);
  return copy_service_names_types(nt, ntm, allocator);
}

OpenDDSNode::OpenDDSNode(rmw_context_t &context, const char *name,
                         const char *name_space)
    : context_(context), name_(name ? name : ""),
      namespace_(name_space ? name_space : ""), gc_(nullptr),
      pub_listener_(nullptr), sub_listener_(nullptr), dp_(), dpi_(nullptr) {
  try {
    if (name_.empty()) {
      throw std::runtime_error("node name_ is null");
    }
    if (namespace_.empty()) {
      throw std::runtime_error("node namespace_ is null");
    }
    set_default_participant_qos();
    gc_ = rmw_create_guard_condition(&context);
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
    OpenDDS::DCPS::Discovery_rch disco =
        TheServiceParticipant->get_discovery(context.options.domain_id);
    OpenDDS::RTPS::RtpsDiscovery_rch rtps_disco =
        OpenDDS::DCPS::dynamic_rchandle_cast<OpenDDS::RTPS::RtpsDiscovery>(
            disco);
    rtps_disco->use_xtypes(OpenDDS::RTPS::RtpsDiscoveryConfig::XTYPES_MINIMAL);
    dp_ = context.impl->dpf_->create_participant(
        static_cast<DDS::DomainId_t>(context.options.domain_id),
        PARTICIPANT_QOS_DEFAULT, 0, 0);
    if (!dp_) {
      throw std::runtime_error("create_participant failed");
    }
    dpi_ = dynamic_cast<OpenDDS::DCPS::DomainParticipantImpl *>(dp_.in());
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
    DDS::DataReader_ptr dr =
        sub->lookup_datareader(OpenDDS::DCPS::BUILT_IN_PUBLICATION_TOPIC);
    auto pub_dr =
        dynamic_cast<DDS::PublicationBuiltinTopicDataDataReader *>(dr);
    if (!pub_dr) {
      throw std::runtime_error("builtin publication datareader is null");
    }
    pub_dr->set_listener(pub_listener_, DDS::DATA_AVAILABLE_STATUS);

    // setup subscriber listener
    dr = sub->lookup_datareader(OpenDDS::DCPS::BUILT_IN_SUBSCRIPTION_TOPIC);
    auto sub_dr =
        dynamic_cast<DDS::SubscriptionBuiltinTopicDataDataReader *>(dr);
    if (!sub_dr) {
      throw std::runtime_error("builtin subscription datareader is null");
    }
    sub_dr->set_listener(sub_listener_, DDS::DATA_AVAILABLE_STATUS);

  } catch (const std::exception &e) {
    RMW_SET_ERROR_MSG(e.what());
    cleanup();
    throw;
  } catch (...) {
    RMW_SET_ERROR_MSG("OpenDDSNode constructor failed");
    cleanup();
    throw;
  }
}

void OpenDDSNode::cleanup() {
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
    if (rmw_destroy_guard_condition(gc_) != RMW_RET_OK) {
      RMW_SET_ERROR_MSG("destroy_guard_condition failed");
    }
    gc_ = nullptr;
  }
}

#ifdef OPENDDS_SECURITY
void append(DDS::PropertySeq &props, const char *name, const char *value,
            bool propagate = false) {
  const DDS::Property_t prop = {name, value, propagate};
  const unsigned int len = props.length();
  props.length(len + 1);
  props[len] = prop;
}

bool enable_security(const char *root, DDS::PropertySeq &props) {
  try {
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    char *buf = rcutils_join_path(root, "identity_ca.cert.pem", allocator);
    if (buf) {
      append(props, DDS::Security::Properties::AuthIdentityCA, buf);
      allocator.deallocate(buf, allocator.state);
      buf = nullptr;
    } else {
      throw std::string("failed to allocate memory for identity_ca_cert_fn");
    }

    buf = rcutils_join_path(root, "permissions_ca.cert.pem", allocator);
    if (buf) {
      append(props, DDS::Security::Properties::AccessPermissionsCA, buf);
      allocator.deallocate(buf, allocator.state);
      buf = nullptr;
    } else {
      throw std::string("failed to allocate memory for permissions_ca_cert_fn");
    }

    buf = rcutils_join_path(root, "cert.pem", allocator);
    if (buf) {
      append(props, DDS::Security::Properties::AuthIdentityCertificate, buf);
      allocator.deallocate(buf, allocator.state);
      buf = nullptr;
    } else {
      throw std::string("failed to allocate memory for cert_fn");
    }

    buf = rcutils_join_path(root, "key.pem", allocator);
    if (buf) {
      append(props, DDS::Security::Properties::AuthPrivateKey, buf);
      allocator.deallocate(buf, allocator.state);
      buf = nullptr;
    } else {
      throw std::string("failed to allocate memory for key_fn");
    }

    buf = rcutils_join_path(root, "governance.p7s", allocator);
    if (buf) {
      append(props, DDS::Security::Properties::AccessGovernance, buf);
      allocator.deallocate(buf, allocator.state);
      buf = nullptr;
    } else {
      throw std::string("failed to allocate memory for gov_fn");
    }

    buf = rcutils_join_path(root, "permissions.p7s", allocator);
    if (buf) {
      append(props, DDS::Security::Properties::AccessPermissions, buf);
      allocator.deallocate(buf, allocator.state);
      buf = nullptr;
    } else {
      throw std::string("failed to allocate memory for perm_fn");
    }

    return true;
  } catch (const std::string &e) {
    RMW_SET_ERROR_MSG(e.c_str());
  } catch (...) {
  }
  return false;
}
#endif

void OpenDDSNode::set_default_participant_qos() {
  DDS::DomainParticipantQos qos;
  if (context_.impl->dpf_->get_default_participant_qos(qos) !=
      DDS::RETCODE_OK) {
    throw std::runtime_error("get_default_participant_qos failed");
  }
  // since participant name is not part of DDS spec, set node name in user_data
  const size_t length =
      name_.length() + namespace_.length() + strlen("name=;namespace=;") + 1;
  qos.user_data.value.length(static_cast<CORBA::Long>(length));
  int written = snprintf(
      reinterpret_cast<char *>(qos.user_data.value.get_buffer()), length,
      "name=%s;namespace=%s;", name_.c_str(), namespace_.c_str());
  if (written < 0 || written > static_cast<int>(length) - 1) {
    throw std::runtime_error("failed to set qos.user_data");
  }
  if (context_.impl->dpf_->set_default_participant_qos(qos) !=
      DDS::RETCODE_OK) {
    throw std::runtime_error("set_default_participant_qos failed");
  }
}

bool OpenDDSNode::configureTransport() {
  try {
    static int i = 0;
    const Guard guard(lock_);
    std::string str_domain_id =
        std::to_string(context_.options.domain_id) + "_" + std::to_string(++i);
    std::string config_name = "cfg" + str_domain_id;
    std::string inst_name = "rtps" + str_domain_id;
    OpenDDS::DCPS::TransportInst_rch inst =
        TheTransportRegistry->get_inst(inst_name);
    if (inst.is_nil()) {
      inst = TheTransportRegistry->create_inst(inst_name, "rtps_udp");
    }
    OpenDDS::DCPS::TransportConfig_rch cfg =
        TheTransportRegistry->get_config(config_name);
    if (cfg.is_nil()) {
      cfg = TheTransportRegistry->create_config(config_name);
    }
    cfg->instances_.push_back(inst);
    TheTransportRegistry->bind_config(cfg, dp_.in());
    return true;
  } catch (const OpenDDS::DCPS::Transport::Exception &e) {
    ACE_ERROR_RETURN(
        (LM_ERROR, ACE_TEXT("(%P|%t) ERROR: %C%m\n"), typeid(e).name()), false);
  } catch (const CORBA::Exception &e) {
    ACE_ERROR_RETURN(
        (LM_ERROR, ACE_TEXT("(%P|%t) ERROR: %C%m\n"), e._info().c_str()),
        false);
  } catch (...) {
    ACE_ERROR_RETURN(
        (LM_ERROR, ACE_TEXT("(%P|%t) ERROR: in configureTransport()%m\n")),
        false);
  }
}

bool OpenDDSNode::match(DDS::UserDataQosPolicy &user_data_qos,
                        const std::string &node_name,
                        const std::string &node_namespace) const {
  uint8_t *buf = user_data_qos.value.get_buffer();
  if (buf) {
    std::vector<uint8_t> kv(buf, buf + user_data_qos.value.length());
    const auto map = rmw::impl::cpp::parse_key_value(kv);
    const auto nv = map.find("name");
    const auto nsv = map.find("namespace");
    if (nv != map.end() && nsv != map.end()) {
      const std::string n(nv->second.begin(), nv->second.end());
      const std::string ns(nsv->second.begin(), nsv->second.end());
      return (node_name == n) && (node_namespace == ns);
    }
  }
  return false;
}

rmw_ret_t OpenDDSNode::get_key(DDS::GUID_t &key, const char *node_name,
                               const char *node_namespace) const {
  DDS::DomainParticipantQos qos;
  auto dds_ret = dp_->get_qos(qos);
  if (dds_ret == DDS::RETCODE_OK &&
      match(qos.user_data, node_name, node_namespace)) {
    key = dpi_->get_repoid(dp_->get_instance_handle());
    return RMW_RET_OK;
  }

  DDS::InstanceHandleSeq handles;
  if (dp_->get_discovered_participants(handles) != DDS::RETCODE_OK) {
    RMW_SET_ERROR_MSG("unable to fetch discovered participants.");
    return RMW_RET_ERROR;
  }

  for (CORBA::ULong i = 0; i < handles.length(); ++i) {
    DDS::ParticipantBuiltinTopicData pbtd;
    auto dds_ret = dp_->get_discovered_participant_data(pbtd, handles[i]);
    if (dds_ret == DDS::RETCODE_OK) {
      if (match(pbtd.user_data, node_name, node_namespace)) {
        DDS_BuiltinTopicKey_to_GUID(&key, pbtd.key);
        return RMW_RET_OK;
      }
    } else {
      RMW_SET_ERROR_MSG("unable to fetch discovered participants data.");
      return RMW_RET_ERROR;
    }
  }
  RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("Node name not found: ns='%s', name='%s",
                                       node_namespace, node_name);
  return RMW_RET_NODE_NAME_NON_EXISTENT;
}

constexpr char SAMPLE_PREFIX[] = "/Sample_";

rmw_ret_t
OpenDDSNode::copy_topic_names_types(rmw_names_and_types_t *nt,
                                    const NameTypeMap &ntm, bool no_demangle,
                                    rcutils_allocator_t *allocator) const {
  // Copy data to results handle
  if (!ntm.empty()) {
    // Setup string array to store names
    rmw_ret_t rmw_ret = rmw_names_and_types_init(nt, ntm.size(), allocator);
    if (rmw_ret != RMW_RET_OK) {
      return rmw_ret;
    }
    // Setup cleanup function, in case of failure below
    auto fail_cleanup = [&nt]() {
      if (rmw_names_and_types_fini(nt) != RMW_RET_OK) {
        RCUTILS_LOG_ERROR("rmw_names_and_types_fini failed: %s",
                          rmw_get_error_string().str);
      }
    };
    // Setup demangling functions based on no_demangle option
    auto demangle_topic = _demangle_if_ros_topic;
    auto demangle_type = _demangle_if_ros_type;
    if (no_demangle) {
      auto noop = [](const std::string &in) { return in; };
      demangle_topic = noop;
      demangle_type = noop;
    }
    // For each topic, store the name, initialize the string array for types,
    // and store all types
    size_t index = 0;
    for (const auto &topic_n_types : ntm) {
      // Duplicate and store the topic_name
      char *topic_name = rcutils_strdup(
          demangle_topic(topic_n_types.first).c_str(), *allocator);
      if (!topic_name) {
        RMW_SET_ERROR_MSG("failed to allocate memory for topic name");
        fail_cleanup();
        return RMW_RET_BAD_ALLOC;
      }
      nt->names.data[index] = topic_name;
      // Setup storage for types
      {
        rcutils_ret_t rcutils_ret = rcutils_string_array_init(
            &nt->types[index], topic_n_types.second.size(), allocator);
        if (rcutils_ret != RCUTILS_RET_OK) {
          RMW_SET_ERROR_MSG(rcutils_get_error_string().str);
          fail_cleanup();
          return rmw_convert_rcutils_ret_to_rmw_ret(rcutils_ret);
        }
      }
      // Duplicate and store each type for the topic
      size_t type_index = 0;
      for (const auto &type : topic_n_types.second) {
        char *type_name =
            rcutils_strdup(demangle_type(type).c_str(), *allocator);
        if (!type_name) {
          RMW_SET_ERROR_MSG("failed to allocate memory for type name");
          fail_cleanup();
          return RMW_RET_BAD_ALLOC;
        }
        nt->types[index].data[type_index] = type_name;
        ++type_index;
      } // for each type
      ++index;
    } // for each topic
  }
  return RMW_RET_OK;
}

rmw_ret_t
OpenDDSNode::copy_service_names_types(rmw_names_and_types_t *nt,
                                      const NameTypeMap &ntm,
                                      rcutils_allocator_t *allocator) const {
  if (!ntm.empty()) {
    // Setup string array to store names
    rmw_ret_t rmw_ret = rmw_names_and_types_init(nt, ntm.size(), allocator);
    if (rmw_ret != RMW_RET_OK) {
      return rmw_ret;
    }
    // Setup cleanup function, in case of failure below
    auto fail_cleanup = [&nt]() {
      if (rmw_names_and_types_fini(nt) != RMW_RET_OK) {
        RCUTILS_LOG_ERROR("rmw_names_and_types_fini failed: %s",
                          rmw_get_error_string().str);
      }
    };
    // For each service, store the name, initialize the string array for types,
    // and store all types
    size_t index = 0;
    for (const auto &service_n_types : ntm) {
      // Duplicate and store the service_name
      char *service_name =
          rcutils_strdup(service_n_types.first.c_str(), *allocator);
      if (!service_name) {
        RMW_SET_ERROR_MSG("failed to allocate memory for service name");
        fail_cleanup();
        return RMW_RET_BAD_ALLOC;
      }
      nt->names.data[index] = service_name;
      // Setup storage for types
      {
        rcutils_ret_t rcutils_ret = rcutils_string_array_init(
            &nt->types[index], service_n_types.second.size(), allocator);
        if (rcutils_ret != RCUTILS_RET_OK) {
          RMW_SET_ERROR_MSG(rcutils_get_error_string().str);
          fail_cleanup();
          return rmw_convert_rcutils_ret_to_rmw_ret(rcutils_ret);
        }
      }
      // Duplicate and store each type for the service
      size_t type_index = 0;
      for (const auto &type : service_n_types.second) {
        // Strip the SAMPLE_PREFIX if it is found (e.g. from services using
        // OpenSplice typesupport). It may not be found if services are detected
        // using other typesupports.
        size_t n = type.find(SAMPLE_PREFIX);
        std::string stripped_type = type;
        if (std::string::npos != n) {
          stripped_type =
              type.substr(0, n + 1) + type.substr(n + strlen(SAMPLE_PREFIX));
        }
        char *type_name = rcutils_strdup(stripped_type.c_str(), *allocator);
        if (!type_name) {
          RMW_SET_ERROR_MSG("failed to allocate memory for type name");
          fail_cleanup();
          return RMW_RET_BAD_ALLOC;
        }
        nt->types[index].data[type_index] = type_name;
        ++type_index;
      } // for each type
      ++index;
    } // for each service
  }
  return RMW_RET_OK;
}
