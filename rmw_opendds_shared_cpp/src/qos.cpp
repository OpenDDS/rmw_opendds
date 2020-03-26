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

#include "rmw_opendds_shared_cpp/qos.hpp"

bool
get_datareader_qos(
  DDS::DomainParticipant * participant,
  const rmw_qos_profile_t & qos_profile,
  DDS::DataReaderQos & datareader_qos)
{
  DDS::ReturnCode_t status; // @todo jwi = participant->get_default_datareader_qos(datareader_qos);
  if (status != DDS::RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to get default datareader qos");
    return false;
  }

  if (!set_entity_qos_from_profile(qos_profile, datareader_qos)) {
    return false;
  }

  return true;
}

bool
get_datawriter_qos(
  DDS::Publisher * publisher,
  const rmw_qos_profile_t & qos_profile,
  DDS::DataWriterQos & datawriter_qos)
{
  if (publisher->get_default_datawriter_qos(datawriter_qos) != DDS::RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to get default datawriter qos");
    return false;
  }
  return set_entity_qos_from_profile(qos_profile, datawriter_qos);
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

  dds_qos_lifespan_to_rmw_qos_lifespan(dds_qos, qos);

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
