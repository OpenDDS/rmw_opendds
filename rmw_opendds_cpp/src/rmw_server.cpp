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

#include <rmw_opendds_cpp/DDSServer.hpp>
#include <rmw_opendds_cpp/OpenDDSNode.hpp>
#include <rmw_opendds_cpp/identifier.hpp>
#include <rmw_opendds_cpp/qos.hpp>
#include <rmw_opendds_cpp/types.hpp>

#include <dds/DCPS/DomainParticipantImpl.h>
#include <dds/DCPS/Marked_Default_Qos.h>

// Uncomment this to get extra console output about discovery.
// #define DISCOVERY_DEBUG_LOGGING 1

#include <rmw/allocators.h>
#include <rmw/error_handling.h>
#include <rmw/rmw.h>

#include <string>

extern "C"
{
void clean_service(rmw_service_t * service)
{
  if (service) {
    auto dds_server = static_cast<DDSServer*>(service->data);
    if (dds_server) {
      DDSServer::Raf::destroy(dds_server);
      service->data = nullptr;
    }
    rmw_service_free(service);
  }
}

rmw_service_t *
rmw_create_service(
  const rmw_node_t * node,
  const rosidl_service_type_support_t * type_supports,
  const char * service_name,
  const rmw_qos_profile_t * rmw_qos)
{
  auto dds_node = OpenDDSNode::from(node);
  if (!dds_node) {
    return nullptr;
  }
  rmw_service_t * service = nullptr;
  try {
    service = rmw_service_allocate();
    if (!service) {
      throw std::runtime_error("rmw_service_allocate failed");
    }
    service->implementation_identifier = opendds_identifier;
    service->data = nullptr;
    auto dds_server = DDSServer::Raf::create(type_supports, service_name, rmw_qos, dds_node->dp());
    if (!dds_server) {
      throw std::runtime_error("DDSServer failed");
    }
    service->data = dds_server;
    service->service_name = dds_server->service_name().c_str();
    dds_server->add_to(dds_node);
    return service;
  } catch (const std::exception& e) {
    RMW_SET_ERROR_MSG(e.what());
  } catch (...) {
    RMW_SET_ERROR_MSG("rmw_create_service failed");
  }
  clean_service(service);
  return nullptr;
}

rmw_ret_t
rmw_destroy_service(rmw_node_t * node, rmw_service_t * service)
{
  auto dds_server = DDSServer::from(service);
  if (dds_server) {
    if (dds_server->remove_from(OpenDDSNode::from(node))) {
      clean_service(service);
      return RMW_RET_OK;
    }
  }
  return RMW_RET_ERROR;
}

rmw_ret_t
rmw_take_request(
  const rmw_service_t * service,
  rmw_service_info_t * request_header,
  void * ros_request,
  bool * taken)
{
  auto dds_server = DDSServer::from(service);
  if (!dds_server) {
    return RMW_RET_INVALID_ARGUMENT;
  }
  if (!request_header) {
    RMW_SET_ERROR_MSG("request_header is null");
    return RMW_RET_INVALID_ARGUMENT;
  }
  if (!ros_request) {
    RMW_SET_ERROR_MSG("ros_request is null");
    return RMW_RET_INVALID_ARGUMENT;
  }
  if (!taken) {
    RMW_SET_ERROR_MSG("taken is null");
    return RMW_RET_INVALID_ARGUMENT;
  }
  *taken = dds_server->take(request_header, ros_request);
  return RMW_RET_OK;
}

rmw_ret_t
rmw_send_response(
  const rmw_service_t * service,
  rmw_request_id_t * request_header,
  void * ros_response)
{
  auto dds_server = DDSServer::from(service);
  if (!dds_server) {
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
  return dds_server->send(request_header, ros_response) ? RMW_RET_OK : RMW_RET_ERROR;
}

}  // extern "C"
