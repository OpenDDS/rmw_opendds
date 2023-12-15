// Copyright 2014-2019 Open Source Robotics Foundation, Inc.
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

#include <rmw/error_handling.h>
#include <rmw/ret_types.h>
#include <rmw_opendds_cpp/event.hpp>
#include <rmw_opendds_cpp/identifier.hpp>

#include <rmw/rmw.h>

extern "C"
{
  rmw_ret_t
    rmw_publisher_event_init(
      rmw_event_t* rmw_event,
      const rmw_publisher_t* publisher,
      rmw_event_type_t event_type)
  {
    return __rmw_init_event(
      opendds_identifier,
      rmw_event,
      publisher->implementation_identifier,
      publisher->data,
      event_type);
  }

  rmw_ret_t
    rmw_subscription_event_init(
      rmw_event_t* rmw_event,
      const rmw_subscription_t* subscription,
      rmw_event_type_t event_type)
  {
    return __rmw_init_event(
      opendds_identifier,
      rmw_event,
      subscription->implementation_identifier,
      subscription->data,
      event_type);
  }

  rmw_ret_t
    rmw_take_event(
      const rmw_event_t* event_handle,
      void* event_info,
      bool* taken)
  {
    return __rmw_take_event(
      opendds_identifier,
      event_handle,
      event_info,
      taken);
  }

  rmw_ret_t
    rmw_event_set_callback (
      rmw_event_t * event,
      rmw_event_callback_t callback,
      const void * user_data)
  {
    (void) event;
    (void) callback;
    (void) user_data;
    // Unused in current implementation.
    RMW_SET_ERROR_MSG("unimplemented");
    return RMW_RET_ERROR;
  }

}  // extern "C"
