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

#include <rmw_opendds_shared_cpp/DDSClient.hpp>
#include <rmw_opendds_shared_cpp/OpenDDSNode.hpp>
#include <rmw_opendds_shared_cpp/identifier.hpp>
#include <rmw_opendds_shared_cpp/qos.hpp>
#include <rmw_opendds_shared_cpp/types.hpp>

#include <dds/DCPS/DomainParticipantImpl.h>
#include <dds/DCPS/Marked_Default_Qos.h>

// Uncomment this to get extra console output about discovery.
// This affects code in this file, but there is a similar variable in:
//   rmw_opendds_cpp/shared_functions.cpp
// #define DISCOVERY_DEBUG_LOGGING 1

#include <rmw/allocators.h>
#include <rmw/error_handling.h>
#include <rmw/rmw.h>

#include <string>

extern "C"
{

void clean_client(rmw_client_t * client)
{
  if (client) {
    auto dds_client = static_cast<DDSClient*>(client->data);
    if (dds_client) {
      DDSClient::Raf::destroy(dds_client);
      client->data = nullptr;
    }
    rmw_client_free(client);
  }
}

rmw_client_t *
rmw_create_client(
  const rmw_node_t * node,
  const rosidl_service_type_support_t * type_supports,
  const char * service_name,
  const rmw_qos_profile_t * rmw_qos)
{
  auto dds_node = OpenDDSNode::from(node);
  if (!dds_node) {
    return nullptr;
  }
  rmw_client_t * client = nullptr;
  try {
    client = rmw_client_allocate();
    if (!client) {
      throw std::runtime_error("rmw_client_allocate failed");
    }
    client->implementation_identifier = opendds_identifier;
    client->data = nullptr;
    auto dds_client = DDSClient::Raf::create(type_supports, service_name, rmw_qos, dds_node->dp());
    if (!dds_client) {
      throw std::runtime_error("DDSClient failed");
    }
    client->data = dds_client;
    client->service_name = dds_client->service_name().c_str();
    dds_client->add_to(dds_node);
    return client;
  } catch (const std::exception& e) {
    RMW_SET_ERROR_MSG(e.what());
  } catch (...) {
    RMW_SET_ERROR_MSG("rmw_create_client failed");
  }
  clean_client(client);
  return nullptr;
}

rmw_ret_t
rmw_destroy_client(rmw_node_t * node, rmw_client_t * client)
{
  auto dds_client = DDSClient::from(client);
  if (dds_client) {
    if (dds_client->remove_from(OpenDDSNode::from(node))) {
      clean_client(client);
      return RMW_RET_OK;
    }
  }
  return RMW_RET_ERROR;
}

rmw_ret_t
rmw_service_server_is_available(
  const rmw_node_t * node,
  const rmw_client_t * client,
  bool * is_available)
{
  auto dds_node = OpenDDSNode::from(node);
  if (!dds_node) {
    return RMW_RET_ERROR;
  }
  auto dds_client = DDSClient::from(client);
  if (!dds_client) {
    return RMW_RET_ERROR;
  }
  //?? cmp DomainParticipant?
  if (!is_available) {
    RMW_SET_ERROR_MSG("is_available is null");
    return RMW_RET_ERROR;
  }
  *is_available = dds_client->is_server_available();
  return RMW_RET_OK;
}

rmw_ret_t
rmw_send_request(
  const rmw_client_t * client,
  const void * ros_request,
  int64_t * sequence_id)
{
  auto dds_client = DDSClient::from(client);
  if (!dds_client) {
    return RMW_RET_INVALID_ARGUMENT;
  }
  if (!ros_request) {
    RMW_SET_ERROR_MSG("ros_request is null");
    return RMW_RET_INVALID_ARGUMENT;
  }
  if (!sequence_id) {
    RMW_SET_ERROR_MSG("sequence_id is null");
    return RMW_RET_INVALID_ARGUMENT;
  }
  *sequence_id = dds_client->send(ros_request);
  return RMW_RET_OK;
}

rmw_ret_t
rmw_take_response(
  const rmw_client_t * client,
  rmw_service_info_t * request_header,
  void * ros_response,
  bool * taken)
{
  auto dds_client = DDSClient::from(client);
  if (!dds_client) {
    return RMW_RET_INVALID_ARGUMENT;
  }
  if (!request_header) {
    RMW_SET_ERROR_MSG("request_header is null");
    return RMW_RET_INVALID_ARGUMENT;
  }
  if (!ros_response) {
    RMW_SET_ERROR_MSG("ros_response is null");
    return RMW_RET_INVALID_ARGUMENT;
  }
  if (!taken) {
    RMW_SET_ERROR_MSG("taken is null");
    return RMW_RET_INVALID_ARGUMENT;
  }
  *taken = dds_client->take(request_header, ros_response);
  return RMW_RET_OK;
}

}  // extern "C"
