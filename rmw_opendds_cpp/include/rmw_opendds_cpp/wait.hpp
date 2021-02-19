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

#ifndef RMW_OPENDDS_SHARED_CPP__WAIT_HPP_
#define RMW_OPENDDS_SHARED_CPP__WAIT_HPP_

#include "opendds_include.hpp"

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

template<typename SubscriberInfo, typename ServiceInfo, typename ClientInfo>
rmw_ret_t
wait(
  rmw_subscriptions_t * subscriptions,
  rmw_guard_conditions_t * guard_conditions,
  rmw_services_t * services,
  rmw_clients_t * clients,
  rmw_events_t* events,
  rmw_wait_set_t * wait_set,
  const rmw_time_t * wait_timeout)
{
  RMW_CHECK_ARGUMENT_FOR_NULL(wait_set, RMW_RET_ERROR);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(wait_set, wait_set->implementation_identifier,
    opendds_identifier, return RMW_RET_ERROR);

  auto wait_set_info = static_cast<OpenDDSWaitSetInfo *>(wait_set->data);
  RMW_CHECK_FOR_NULL_WITH_MSG(wait_set_info, "wait_set_info is null", return RMW_RET_ERROR);

  RMW_CHECK_FOR_NULL_WITH_MSG(wait_set_info->wait_set, "dds_wait_set is null", return RMW_RET_ERROR);
  DDS::WaitSet & dds_wait_set = *(wait_set_info->wait_set);

  RMW_CHECK_FOR_NULL_WITH_MSG(wait_set_info->active_conditions, "active_conditions is null", return RMW_RET_ERROR);
  DDS::ConditionSeq & active_conditions = *(wait_set_info->active_conditions);

  // Cleaner destructor will clean up the wait set (previously done in wait_set destructor)
  struct Cleaner
  {
    OpenDDSWaitSetInfo & info;
    DDS::WaitSet & waitset;
    Cleaner(OpenDDSWaitSetInfo & i, DDS::WaitSet & w) : info(i), waitset(w) {}
    ~Cleaner()
    {
      if (info.attached_conditions && waitset.get_conditions(*(info.attached_conditions)) == DDS::RETCODE_OK) {
        for (::CORBA::ULong i = 0; i < info.attached_conditions->length(); ++i) {
          if (waitset.detach_condition((*(info.attached_conditions))[i]) != DDS::RETCODE_OK) {
            RMW_SET_ERROR_MSG("failed to detach condition from wait set");
          }
        }
      } else {
        RMW_SET_ERROR_MSG("failed to get attached conditions");
      }
    }
  } cleaner(*wait_set_info, dds_wait_set);

  // add a condition for each subscriber
  if (subscriptions) {
    for (size_t i = 0; i < subscriptions->subscriber_count; ++i) {
      auto info = static_cast<SubscriberInfo *>(subscriptions->subscribers[i]);
      if (!info) {
        RMW_SET_ERROR_MSG("subscriber info is null");
        return RMW_RET_ERROR;
      }
      DDS::ReadCondition_var read_condition = info->read_condition();
      if (!read_condition) {
        RMW_SET_ERROR_MSG("read condition is null");
        return RMW_RET_ERROR;
      }
      rmw_ret_t ret = check_attach_condition_error(dds_wait_set.attach_condition(read_condition));
      if (ret != RMW_RET_OK) {
        return ret;
      }
    }
  }

  // add each guard condition
  if (guard_conditions) {
    for (size_t i = 0; i < guard_conditions->guard_condition_count; ++i) {
      auto condition = static_cast<DDS::GuardCondition *>(guard_conditions->guard_conditions[i]);
      if (!condition) {
        RMW_SET_ERROR_MSG("guard condition is null");
        return RMW_RET_ERROR;
      }
      rmw_ret_t ret = check_attach_condition_error(dds_wait_set.attach_condition(condition));
      if (ret != RMW_RET_OK) {
        return ret;
      }
    }
  }

  // add a condition for each service
  if (services) {
    for (size_t i = 0; i < services->service_count; ++i) {
      auto info = static_cast<ServiceInfo *>(services->services[i]);
      if (!info) {
        RMW_SET_ERROR_MSG("service info is null");
        return RMW_RET_ERROR;
      }
      if (!info->read_condition_) {
        RMW_SET_ERROR_MSG("read condition is null");
        return RMW_RET_ERROR;
      }
      rmw_ret_t ret = check_attach_condition_error(dds_wait_set.attach_condition(info->read_condition_));
      if (ret != RMW_RET_OK) {
        return ret;
      }
    }
  }

  // add a condition for each client
  if (clients) {
    for (size_t i = 0; i < clients->client_count; ++i) {
      auto info = static_cast<ClientInfo *>(clients->clients[i]);
      if (!info) {
        RMW_SET_ERROR_MSG("client info is null");
        return RMW_RET_ERROR;
      }
      if (!info->read_condition_) {
        RMW_SET_ERROR_MSG("read condition is null");
        return RMW_RET_ERROR;
      }
      rmw_ret_t ret = check_attach_condition_error(dds_wait_set.attach_condition(info->read_condition_));
      if (ret != RMW_RET_OK) {
        return ret;
      }
    }
  }

  // wait until one condition triggers or timeout
  DDS::Duration_t timeout = {
    wait_timeout ? static_cast<::CORBA::Long>(wait_timeout->sec) : DDS::DURATION_INFINITE_SEC,
    wait_timeout ? static_cast<::CORBA::ULong>(wait_timeout->nsec) : DDS::DURATION_INFINITE_NSEC
  };
  DDS::ReturnCode_t status = dds_wait_set.wait(active_conditions, timeout);
  if (status != DDS::RETCODE_OK && status != DDS::RETCODE_TIMEOUT) {
    RMW_SET_ERROR_MSG("failed to wait on wait set");
    return RMW_RET_ERROR;
  }

  // reset subscriber for all untriggered conditions
  if (subscriptions) {
    for (size_t i = 0; i < subscriptions->subscriber_count; ++i) {
      auto info = static_cast<SubscriberInfo *>(subscriptions->subscribers[i]);
      if (!info) {
        RMW_SET_ERROR_MSG("subscriber info is null");
        return RMW_RET_ERROR;
      }
      DDS::ReadCondition_var read_condition = info->read_condition();
      if (!read_condition) {
        RMW_SET_ERROR_MSG("read condition is null");
        return RMW_RET_ERROR;
      }

      // reset the subscriber if its read_condition is not in the active set
      ::CORBA::ULong j = 0;
      while (j < active_conditions.length() && active_conditions[j] != read_condition) { ++j; }
      if (!(j < active_conditions.length())) {
        subscriptions->subscribers[i] = nullptr;
      }
    }
  }

  // reset guard condition for all untriggered conditions
  if (guard_conditions) {
    for (size_t i = 0; i < guard_conditions->guard_condition_count; ++i) {
      auto condition = static_cast<DDS::Condition *>(guard_conditions->guard_conditions[i]);
      if (!condition) {
        RMW_SET_ERROR_MSG("guard_condition is null");
        return RMW_RET_ERROR;
      }

      // reset the guard condition
      ::CORBA::ULong j = 0;
      while (j < active_conditions.length() && active_conditions[j] != condition) { ++j; }
      if (j < active_conditions.length()) {
        DDS::GuardCondition_var guard = DDS::GuardCondition::_narrow(condition);
        if (guard->set_trigger_value(false) != DDS::RETCODE_OK) {
          RMW_SET_ERROR_MSG("failed to set trigger value");
          return RMW_RET_ERROR;
        }
      } else {
        guard_conditions->guard_conditions[i] = nullptr;
      }
    }
  }

  // reset service for all untriggered conditions
  if (services) {
    for (size_t i = 0; i < services->service_count; ++i) {
      auto info = static_cast<ServiceInfo *>(services->services[i]);
      if (!info) {
        RMW_SET_ERROR_MSG("service info is null");
        return RMW_RET_ERROR;
      }
      if (!info->read_condition_) {
        RMW_SET_ERROR_MSG("read condition is null");
        return RMW_RET_ERROR;
      }

      // reset the service if its read_condition is not in the active set
      ::CORBA::ULong j = 0;
      while (j < active_conditions.length() && active_conditions[j] != info->read_condition_) { ++j; }
      if (!(j < active_conditions.length())) {
        services->services[i] = nullptr;
      }
    }
  }

  // reset client for all untriggered conditions
  if (clients) {
    for (size_t i = 0; i < clients->client_count; ++i) {
      auto info = static_cast<ClientInfo *>(clients->clients[i]);
      if (!info) {
        RMW_SET_ERROR_MSG("client info is null");
        return RMW_RET_ERROR;
      }
      if (!info->read_condition_) {
        RMW_SET_ERROR_MSG("read condition is null");
        return RMW_RET_ERROR;
      }

      // reset the client if its read_condition is not in the active set
      ::CORBA::ULong j = 0;
      while (j < active_conditions.length() && active_conditions[j] != info->read_condition_) { ++j; }
      if (!(j < active_conditions.length())) {
        clients->clients[i] = nullptr;
      }
    }
  }

  {
    rmw_ret_t rmw_ret_code = __handle_active_event_conditions(events);
    if (rmw_ret_code != RMW_RET_OK) {
      return rmw_ret_code;
    }
  }

  return (status == DDS::RETCODE_TIMEOUT) ? RMW_RET_TIMEOUT : RMW_RET_OK;
}

#endif  // RMW_OPENDDS_SHARED_CPP__WAIT_HPP_
