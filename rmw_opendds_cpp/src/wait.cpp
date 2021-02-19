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

#include "rmw_opendds_cpp/opendds_include.hpp"

#include "rmw_opendds_cpp/condition_error.hpp"
#include "rmw_opendds_cpp/event_converter.hpp"
#include "rmw_opendds_cpp/identifier.hpp"
#include "rmw_opendds_cpp/types.hpp"
#include "rmw_opendds_cpp/visibility_control.h"
#include "rmw_opendds_cpp/opendds_static_event_info.hpp"

#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/types.h"

#include <unordered_map>
#include <unordered_set>
#include <utility>

rmw_ret_t __handle_active_event_conditions(rmw_events_t* events)
{
  // enable a status condition for each event
  if (events) {
    for (size_t i = 0; i < events->event_count; ++i) {
      auto current_event = static_cast<rmw_event_t*>(events->events[i]);
      RMW_CHECK_ARGUMENT_FOR_NULL(current_event->data, RMW_RET_INVALID_ARGUMENT);
      DDS::Entity* dds_entity = static_cast<OpenDDSCustomEventInfo*>(
        current_event->data)->get_entity();
      if (!dds_entity) {
        RMW_SET_ERROR_MSG("Event handle is null");
        return RMW_RET_ERROR;
      }

      DDS::StatusMask status_mask = dds_entity->get_status_changes();
      bool is_active = false;

      if (is_event_supported(current_event->event_type)) {
        is_active = static_cast<bool>(status_mask &
          get_status_kind_from_rmw(current_event->event_type));
      }
      // if status condition is not found in the active set
      // reset the subscriber handle
      if (!is_active) {
        events->events[i] = nullptr;
      }
    }
  }
  return RMW_RET_OK;
}
