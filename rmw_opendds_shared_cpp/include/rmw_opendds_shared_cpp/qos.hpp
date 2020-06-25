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
#include "rmw/incompatible_qos_events_statuses.h"

#include "rmw_opendds_shared_cpp/visibility_control.h"

RMW_OPENDDS_SHARED_CPP_PUBLIC
bool
get_datareader_qos(
  DDS::Subscriber * subscriber,
  const rmw_qos_profile_t & qos_profile,
  DDS::DataReaderQos & datareader_qos);

RMW_OPENDDS_SHARED_CPP_PUBLIC
rmw_qos_policy_kind_t
dds_qos_policy_to_rmw_qos_policy(DDS::QosPolicyId_t policy_id);

RMW_OPENDDS_SHARED_CPP_PUBLIC
bool
get_datawriter_qos(
  DDS::Publisher * publisher,
  const rmw_qos_profile_t & qos_profile,
  DDS::DataWriterQos & datawriter_qos);

inline bool
is_default(const rmw_time_t & t) {
  return t.sec == 0 && t.nsec == 0;
}

inline void
time_to_dds(DDS::Duration_t & dds, const rmw_time_t & rmw) {
  dds.sec = rmw.sec;
  dds.nanosec = rmw.nsec;
}

inline void
time_to_rmw(rmw_time_t & rmw, const DDS::Duration_t & dds) {
  rmw.sec = dds.sec;
  rmw.nsec = dds.nanosec;
}

inline void
qos_lifespan_to_rmw(rmw_qos_profile_t & rmw_qos, const DDS::DataWriterQos & dds_qos) {
  time_to_rmw(rmw_qos.lifespan, dds_qos.lifespan.duration);
}

inline void
qos_lifespan_to_rmw(rmw_qos_profile_t &, const DDS::DataReaderQos &) {
  // no-op since no lifespan in DataReaderQos
}

template<typename DDSEntityQos>
bool
rmw_qos_to_dds_qos(
  const rmw_qos_profile_t & rmw_qos,
  DDSEntityQos & dds_qos)
{
  // Read properties from the rmw profile
  switch (rmw_qos.history) {
    case RMW_QOS_POLICY_HISTORY_KEEP_LAST:
      dds_qos.history.kind = DDS::KEEP_LAST_HISTORY_QOS;
      break;
    case RMW_QOS_POLICY_HISTORY_KEEP_ALL:
      dds_qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
      break;
    case RMW_QOS_POLICY_HISTORY_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("Unknown QoS history policy");
      return false;
  }
  if (rmw_qos.depth != RMW_QOS_POLICY_DEPTH_SYSTEM_DEFAULT) {
    dds_qos.history.depth = static_cast<CORBA::ULong>(rmw_qos.depth);
  }
  // ensure the history depth is at least the requested queue size
  assert(dds_qos.history.depth >= 0);
  if (dds_qos.history.kind == DDS::KEEP_LAST_HISTORY_QOS &&
    static_cast<size_t>(dds_qos.history.depth) < rmw_qos.depth) {
    if (rmw_qos.depth > (std::numeric_limits<CORBA::ULong>::max)()) {
      RMW_SET_ERROR_MSG("failed to set history depth since the requested queue size exceeds the DDS type");
      return false;
    }
    dds_qos.history.depth = static_cast<CORBA::ULong>(rmw_qos.depth);
  }

  switch (rmw_qos.reliability) {
    case RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT:
      dds_qos.reliability.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;
      break;
    case RMW_QOS_POLICY_RELIABILITY_RELIABLE:
      dds_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
      break;
    case RMW_QOS_POLICY_RELIABILITY_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("Unknown QoS reliability policy");
      return false;
  }

  switch (rmw_qos.durability) {
    case RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL:
      dds_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
      break;
    case RMW_QOS_POLICY_DURABILITY_VOLATILE:
      dds_qos.durability.kind = DDS::VOLATILE_DURABILITY_QOS;
      break;
    case RMW_QOS_POLICY_DURABILITY_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("Unknown QoS durability policy");
      return false;
  }

  switch (rmw_qos.liveliness) {
    case RMW_QOS_POLICY_LIVELINESS_AUTOMATIC:
      dds_qos.liveliness.kind = DDS::AUTOMATIC_LIVELINESS_QOS;
      break;
    case RMW_QOS_POLICY_LIVELINESS_MANUAL_BY_TOPIC:
      dds_qos.liveliness.kind = DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS;
      break;
    case RMW_QOS_POLICY_LIVELINESS_SYSTEM_DEFAULT:
      break;
    default:
      RMW_SET_ERROR_MSG("Unknown QoS liveliness policy");
      return false;
  }
  if (!is_default(rmw_qos.liveliness_lease_duration)) {
    time_to_dds(dds_qos.liveliness.lease_duration, rmw_qos.liveliness_lease_duration);
  }

  return true;
}

template<typename AttributeT>
void
dds_qos_to_rmw_qos(
  const AttributeT & dds_qos,
  rmw_qos_profile_t & rmw_qos)
{
  switch (dds_qos.history.kind) {
  case DDS::KEEP_LAST_HISTORY_QOS:
    rmw_qos.history = RMW_QOS_POLICY_HISTORY_KEEP_LAST;
    break;
  case DDS::KEEP_ALL_HISTORY_QOS:
    rmw_qos.history = RMW_QOS_POLICY_HISTORY_KEEP_ALL;
    break;
  default:
    rmw_qos.history = RMW_QOS_POLICY_HISTORY_UNKNOWN;
    break;
  }
  rmw_qos.depth = static_cast<size_t>(dds_qos.history.depth);

  switch (dds_qos.reliability.kind) {
  case DDS::BEST_EFFORT_RELIABILITY_QOS:
    rmw_qos.reliability = RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT;
    break;
  case DDS::RELIABLE_RELIABILITY_QOS:
    rmw_qos.reliability = RMW_QOS_POLICY_RELIABILITY_RELIABLE;
    break;
  default:
    rmw_qos.reliability = RMW_QOS_POLICY_RELIABILITY_UNKNOWN;
    break;
  }

  switch (dds_qos.durability.kind) {
  case DDS::TRANSIENT_LOCAL_DURABILITY_QOS:
    rmw_qos.durability = RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL;
    break;
  case DDS::VOLATILE_DURABILITY_QOS:
    rmw_qos.durability = RMW_QOS_POLICY_DURABILITY_VOLATILE;
    break;
  default:
    rmw_qos.durability = RMW_QOS_POLICY_DURABILITY_UNKNOWN;
    break;
  }

  time_to_rmw(rmw_qos.deadline, dds_qos.deadline.period);
  qos_lifespan_to_rmw(rmw_qos, dds_qos);

  switch (dds_qos.liveliness.kind) {
  case DDS::AUTOMATIC_LIVELINESS_QOS:
    rmw_qos.liveliness = RMW_QOS_POLICY_LIVELINESS_AUTOMATIC;
    break;
  case DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS:
    rmw_qos.liveliness = RMW_QOS_POLICY_LIVELINESS_MANUAL_BY_TOPIC;
    break;
  case DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS:
    rmw_qos.liveliness = RMW_QOS_POLICY_LIVELINESS_MANUAL_BY_TOPIC;
    break;
  default:
    rmw_qos.liveliness = RMW_QOS_POLICY_LIVELINESS_UNKNOWN;
    break;
  }
  time_to_rmw(rmw_qos.liveliness_lease_duration, dds_qos.liveliness.lease_duration);
}

#endif  // RMW_OPENDDS_SHARED_CPP__QOS_HPP_
