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

#ifndef RMW_OPENDDS_SHARED_CPP__QOS_HPP_
#define RMW_OPENDDS_SHARED_CPP__QOS_HPP_

#include <cassert>
#include <limits>

#include "opendds_include.hpp"

#include "rmw/error_handling.h"
#include "rmw/types.h"

#include "rmw_opendds_shared_cpp/visibility_control.h"

RMW_OPENDDS_SHARED_CPP_PUBLIC
bool
get_datareader_qos(
  DDS::Subscriber * subscriber,
  const rmw_qos_profile_t & qos_profile,
  DDS::DataReaderQos & datareader_qos);

RMW_OPENDDS_SHARED_CPP_PUBLIC
bool
get_datawriter_qos(
  DDS::Publisher * publisher,
  const rmw_qos_profile_t & qos_profile,
  DDS::DataWriterQos & datawriter_qos);

template<typename DDSEntityQos>
bool
set_entity_qos_from_profile(
  const rmw_qos_profile_t & qos_profile,
  DDSEntityQos & entity_qos)
{
  // Read properties from the rmw profile
  switch (qos_profile.history) {
    case RMW_QOS_POLICY_HISTORY_KEEP_LAST:
      entity_qos.history.kind = DDS::KEEP_LAST_HISTORY_QOS;
      break;
    case RMW_QOS_POLICY_HISTORY_KEEP_ALL:
      entity_qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
      break;
    case RMW_QOS_POLICY_HISTORY_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("Unknown QoS history policy");
      return false;
  }

  switch (qos_profile.reliability) {
    case RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT:
      entity_qos.reliability.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;
      break;
    case RMW_QOS_POLICY_RELIABILITY_RELIABLE:
      entity_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
      break;
    case RMW_QOS_POLICY_RELIABILITY_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("Unknown QoS reliability policy");
      return false;
  }

  switch (qos_profile.durability) {
    case RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL:
      entity_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
      break;
    case RMW_QOS_POLICY_DURABILITY_VOLATILE:
      entity_qos.durability.kind = DDS::VOLATILE_DURABILITY_QOS;
      break;
    case RMW_QOS_POLICY_DURABILITY_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("Unknown QoS durability policy");
      return false;
  }

  if (qos_profile.depth != RMW_QOS_POLICY_DEPTH_SYSTEM_DEFAULT) {
    entity_qos.history.depth = static_cast<CORBA::ULong>(qos_profile.depth);
  }

  // ensure the history depth is at least the requested queue size
  assert(entity_qos.history.depth >= 0);
  if (
    entity_qos.history.kind == DDS::KEEP_LAST_HISTORY_QOS &&
    static_cast<size_t>(entity_qos.history.depth) < qos_profile.depth)
  {
    if (qos_profile.depth > (std::numeric_limits<CORBA::ULong>::max)()) {
      RMW_SET_ERROR_MSG(
        "failed to set history depth since the requested queue size exceeds the DDS type");
      return false;
    }
    entity_qos.history.depth = static_cast<CORBA::ULong>(qos_profile.depth);
  }
  return true;
}

template<typename AttributeT>
void
dds_qos_to_rmw_qos(
  const AttributeT& dds_qos,
  rmw_qos_profile_t* qos)
{
  switch (dds_qos.history.kind) {
  case DDS::KEEP_LAST_HISTORY_QOS:
    qos->history = RMW_QOS_POLICY_HISTORY_KEEP_LAST;
    break;
  case DDS::KEEP_ALL_HISTORY_QOS:
    qos->history = RMW_QOS_POLICY_HISTORY_KEEP_ALL;
    break;
  default:
    qos->history = RMW_QOS_POLICY_HISTORY_UNKNOWN;
    break;
  }
  qos->depth = static_cast<size_t>(dds_qos.history.depth);

  switch (dds_qos.reliability.kind) {
  case DDS::BEST_EFFORT_RELIABILITY_QOS:
    qos->reliability = RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT;
    break;
  case DDS::RELIABLE_RELIABILITY_QOS:
    qos->reliability = RMW_QOS_POLICY_RELIABILITY_RELIABLE;
    break;
  default:
    qos->reliability = RMW_QOS_POLICY_RELIABILITY_UNKNOWN;
    break;
  }

  switch (dds_qos.durability.kind) {
  case DDS::TRANSIENT_LOCAL_DURABILITY_QOS:
    qos->durability = RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL;
    break;
  case DDS::VOLATILE_DURABILITY_QOS:
    qos->durability = RMW_QOS_POLICY_DURABILITY_VOLATILE;
    break;
  default:
    qos->durability = RMW_QOS_POLICY_DURABILITY_UNKNOWN;
    break;
  }

  qos->deadline.sec = dds_qos.deadline.period.sec;
  qos->deadline.nsec = dds_qos.deadline.period.nanosec;

  //dds_qos_lifespan_to_rmw_qos_lifespan(dds_qos, qos); //?? temp

  switch (dds_qos.liveliness.kind) {
  case DDS::AUTOMATIC_LIVELINESS_QOS:
    qos->liveliness = RMW_QOS_POLICY_LIVELINESS_AUTOMATIC;
    break;
  case DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS:
    qos->liveliness = RMW_QOS_POLICY_LIVELINESS_MANUAL_BY_NODE;
    break;
  case DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS:
    qos->liveliness = RMW_QOS_POLICY_LIVELINESS_MANUAL_BY_TOPIC;
    break;
  default:
    qos->liveliness = RMW_QOS_POLICY_LIVELINESS_UNKNOWN;
    break;
  }
  qos->liveliness_lease_duration.sec = dds_qos.liveliness.lease_duration.sec;
  qos->liveliness_lease_duration.nsec = dds_qos.liveliness.lease_duration.nanosec;
}

#endif  // RMW_OPENDDS_SHARED_CPP__QOS_HPP_
