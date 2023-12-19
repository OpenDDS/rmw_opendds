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

#include <rmw/qos_profiles.h>
#include <rmw/types.h>
#include <rmw_opendds_cpp/qos.hpp>

bool
get_datareader_qos(
  DDS::Subscriber * subscriber,
  const rmw_qos_profile_t & qos_profile,
  DDS::DataReaderQos & datareader_qos)
{
  if (subscriber->get_default_datareader_qos(datareader_qos) != DDS::RETCODE_OK) {
    RMW_SET_ERROR_MSG("failed to get default datareader qos");
    return false;
  }
  return rmw_qos_to_dds_qos(qos_profile, datareader_qos);
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
  return rmw_qos_to_dds_qos(qos_profile, datawriter_qos);
}

rmw_qos_policy_kind_t
dds_qos_policy_to_rmw_qos_policy(DDS::QosPolicyId_t policy_id)
{
  switch (policy_id) {
  case DDS::DURABILITY_QOS_POLICY_ID:
    return RMW_QOS_POLICY_DURABILITY;
  case DDS::DEADLINE_QOS_POLICY_ID:
    return RMW_QOS_POLICY_DEADLINE;
  case DDS::LIVELINESS_QOS_POLICY_ID:
    return RMW_QOS_POLICY_LIVELINESS;
  case DDS::RELIABILITY_QOS_POLICY_ID:
    return RMW_QOS_POLICY_RELIABILITY;
  case DDS::HISTORY_QOS_POLICY_ID:
    return RMW_QOS_POLICY_HISTORY;
  case DDS::LIFESPAN_QOS_POLICY_ID:
    return RMW_QOS_POLICY_LIFESPAN;
  default:
    return RMW_QOS_POLICY_INVALID;
  }
}

rmw_ret_t
rmw_qos_profile_check_compatible (
  const rmw_qos_profile_t publisher_profile, 
  const rmw_qos_profile_t subscriber_profile,
  rmw_qos_compatibility_type_t * compatibility,
  char * reason,
  size_t reason_size)
{
    (void) publisher_profile;
    (void) subscriber_profile;
    (void) compatibility;
    (void) reason;
    (void) reason_size;
    // Unused in current implementation.
    RMW_SET_ERROR_MSG("unimplemented");
    return RMW_RET_ERROR;
}
