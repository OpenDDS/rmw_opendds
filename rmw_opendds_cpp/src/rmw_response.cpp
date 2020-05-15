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

#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/rmw.h"

#include "rmw_opendds_cpp/identifier.hpp"
#include "rmw_opendds_cpp/opendds_static_client_info.hpp"
#include "rmw_opendds_cpp/opendds_static_service_info.hpp"

extern "C"
{
rmw_ret_t
rmw_take_response(
  const rmw_client_t * client,
  rmw_request_id_t * request_header,
  void * ros_response,
  bool * taken)
{
  RMW_CHECK_FOR_NULL_WITH_MSG(client, "client is null", return RMW_RET_ERROR);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(client, client->implementation_identifier, opendds_identifier, return RMW_RET_ERROR)
  RMW_CHECK_FOR_NULL_WITH_MSG(request_header, "request_header is null", return RMW_RET_ERROR);
  RMW_CHECK_FOR_NULL_WITH_MSG(ros_response, "ros_response is null", return RMW_RET_ERROR);
  RMW_CHECK_FOR_NULL_WITH_MSG(taken, "taken is null", return RMW_RET_ERROR);

  auto client_info = static_cast<OpenDDSStaticClientInfo *>(client->data);
  RMW_CHECK_FOR_NULL_WITH_MSG(client_info, "client_info is null", return RMW_RET_ERROR);
  RMW_CHECK_FOR_NULL_WITH_MSG(client_info->callbacks_, "callbacks_ is null", return RMW_RET_ERROR);
  RMW_CHECK_FOR_NULL_WITH_MSG(client_info->requester_, "requester_ is null", return RMW_RET_ERROR);

  *taken = client_info->callbacks_->take_response(client_info->requester_, request_header, ros_response);

  return RMW_RET_OK;
}

rmw_ret_t
rmw_send_response(
  const rmw_service_t * service,
  rmw_request_id_t * request_header,
  void * ros_response)
{
  RMW_CHECK_FOR_NULL_WITH_MSG(service, "service is null", return RMW_RET_ERROR);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(service, service->implementation_identifier, opendds_identifier, return RMW_RET_ERROR)
  RMW_CHECK_FOR_NULL_WITH_MSG(request_header, "request_header is null", return RMW_RET_ERROR);
  RMW_CHECK_FOR_NULL_WITH_MSG(ros_response, "ros_response is null", return RMW_RET_ERROR);

  auto service_info = static_cast<OpenDDSStaticServiceInfo *>(service->data);
  RMW_CHECK_FOR_NULL_WITH_MSG(service_info, "service_info is null", return RMW_RET_ERROR);
/*  TODO: uncommnet this block when type support is ready.
  RMW_CHECK_FOR_NULL_WITH_MSG(service_info->callbacks_, "callbacks_ is null", return RMW_RET_ERROR);
  RMW_CHECK_FOR_NULL_WITH_MSG(service_info->replier_, "replier_ is null", return RMW_RET_ERROR);

  service_info->callbacks_->send_response(service_info->replier_, request_header, ros_response);
*/
  return RMW_RET_OK;
}
}  // extern "C"
