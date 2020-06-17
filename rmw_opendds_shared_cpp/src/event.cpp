// Copyright 2019 Open Source Robotics Foundation, Inc.
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
#include "rmw/incompatible_qos_events_statuses.h"
#include "rmw/visibility_control.h"

//#include "rmw_connext_shared_cpp/connext_static_event_info.hpp"
#include "rmw_opendds_shared_cpp/event.hpp"
#include "rmw_opendds_shared_cpp/event_converter.hpp"
#include "rmw_opendds_shared_cpp/types.hpp"
#include "rmw_opendds_shared_cpp/qos.hpp"

rmw_ret_t
__rmw_init_event(
  const char * identifier,
  rmw_event_t * rmw_event,
  const char * topic_endpoint_impl_identifier,
  void * data,
  rmw_event_type_t event_type)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(identifier, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(rmw_event, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(topic_endpoint_impl_identifier, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(data, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    topic endpoint,
    topic_endpoint_impl_identifier,
    identifier,
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION);
  if (!is_event_supported(event_type)) {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("provided event_type is not supported by %s", identifier);
    return RMW_RET_UNSUPPORTED;
  }

  rmw_event->implementation_identifier = topic_endpoint_impl_identifier;
  rmw_event->data = data;
  rmw_event->event_type = event_type;

  return RMW_RET_OK;
}

rmw_ret_t
__rmw_take_event(
  const char * implementation_identifier,
  const rmw_event_t * event_handle,
  void * event_info,
  bool * taken)
{
  // pointer error checking here
  RMW_CHECK_ARGUMENT_FOR_NULL(event_handle, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(event_info, RMW_RET_INVALID_ARGUMENT);
  RMW_CHECK_ARGUMENT_FOR_NULL(taken, RMW_RET_INVALID_ARGUMENT);

  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(
    event handle,
    event_handle->implementation_identifier,
    implementation_identifier,
    return RMW_RET_ERROR);

  // TODO: uncomment and remove next line
  //rmw_ret_t ret_code = RMW_RET_UNSUPPORTED;
  rmw_ret_t ret_code = RMW_RET_OK;

  // check if we support the input event type
  if (is_event_supported(event_handle->event_type)) {
    // lookup status mask from rmw_event_type
    CORBA::ULong status_kind = get_status_kind_from_rmw(event_handle->event_type);

    // TODO: implement
    /*
    // cast the event_handle to the appropriate type to get the appropriate
    // status from the handle
    // CustomConnextPublisher and CustomConnextSubscriber should implement this interface
    ConnextCustomEventInfo * custom_event_info =
      static_cast<ConnextCustomEventInfo *>(event_handle->data);

    // call get status with the appropriate mask
    // get_status should fill the event with the appropriate status information
    ret_code = custom_event_info->get_status(status_kind, event_info);
    */
    switch (status_kind) {
      case DDS::LIVELINESS_LOST_STATUS:
      {
        rmw_liveliness_lost_status_t* rmw_liveliness_lost =
          static_cast<rmw_liveliness_lost_status_t*>(event_info);
        rmw_liveliness_lost->total_count = 0;
        rmw_liveliness_lost->total_count_change = 0;
        break;
      }
      case DDS::OFFERED_DEADLINE_MISSED_STATUS:
      {
        rmw_offered_deadline_missed_status_t* rmw_offered_deadline_missed =
          static_cast<rmw_offered_deadline_missed_status_t*>(event_info);
        rmw_offered_deadline_missed->total_count = 0;
        rmw_offered_deadline_missed->total_count_change = 0;
        break;
      }
      case DDS::OFFERED_INCOMPATIBLE_QOS_STATUS:
      {
        rmw_offered_qos_incompatible_event_status_t* rmw_offered_qos_incompatible =
          static_cast<rmw_offered_qos_incompatible_event_status_t*>(event_info);
        rmw_offered_qos_incompatible->total_count = 0;
        rmw_offered_qos_incompatible->total_count_change = 0;
        rmw_offered_qos_incompatible->last_policy_kind = RMW_QOS_POLICY_INVALID;
        break;
      }
      case DDS::LIVELINESS_CHANGED_STATUS:
      {
        rmw_liveliness_changed_status_t* rmw_liveliness_changed_status =
          static_cast<rmw_liveliness_changed_status_t*>(event_info);
        rmw_liveliness_changed_status->alive_count = 0;
        rmw_liveliness_changed_status->not_alive_count = 0;
        rmw_liveliness_changed_status->alive_count_change = 0;
        rmw_liveliness_changed_status->not_alive_count_change = 0;
        break;
      }
      case DDS::REQUESTED_DEADLINE_MISSED_STATUS:
      {
        rmw_requested_deadline_missed_status_t* rmw_requested_deadline_missed_status =
          static_cast<rmw_requested_deadline_missed_status_t*>(event_info);
        rmw_requested_deadline_missed_status->total_count = 0;
        rmw_requested_deadline_missed_status->total_count_change = 0;
        break;
      }
      case DDS::REQUESTED_INCOMPATIBLE_QOS_STATUS:
      {
        rmw_requested_qos_incompatible_event_status_t* rmw_requested_qos_incompatible =
          static_cast<rmw_requested_qos_incompatible_event_status_t*>(event_info);
        rmw_requested_qos_incompatible->total_count = 0;
        rmw_requested_qos_incompatible->total_count_change = 0;
        rmw_requested_qos_incompatible->last_policy_kind = RMW_QOS_POLICY_INVALID;
        break;
      }
      default:
        return RMW_RET_UNSUPPORTED;
    }
  } else {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("event %d not supported", event_handle->event_type);
  }

  *taken = (ret_code == RMW_RET_OK);
  return ret_code;
}
