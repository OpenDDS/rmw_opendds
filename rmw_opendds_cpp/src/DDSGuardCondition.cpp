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

#include <rmw_opendds_cpp/DDSGuardCondition.hpp>
#include <rmw_opendds_cpp/identifier.hpp>

DDSGuardCondition * DDSGuardCondition::from(const rmw_guard_condition_t * rmw_guard_condition)
{
  if (!rmw_guard_condition) {
    RMW_SET_ERROR_MSG("rmw_guard_condition is null");
    return nullptr;
  }
  if (!check_impl_id(rmw_guard_condition->implementation_identifier)) {
    return nullptr;
  }
  return from(rmw_guard_condition->data);
}

DDSGuardCondition * DDSGuardCondition::from(void * guard_condition)
{
  auto dds_guard_condition = static_cast<DDSGuardCondition *>(guard_condition);
  if (!dds_guard_condition) {
    RMW_SET_ERROR_MSG("dds_guard_condition is null");
  }
  return dds_guard_condition;
}

rmw_ret_t DDSGuardCondition::set(bool trigger_value)
{
  if (gc_->set_trigger_value(trigger_value) == DDS::RETCODE_OK) {
    return RMW_RET_OK;
  }
  RMW_SET_ERROR_MSG("DDSGuardCondition::set failed");
  return RMW_RET_ERROR;
}

bool DDSGuardCondition::is_in(const DDS::ConditionSeq& cq) const
{
  for (CORBA::ULong i = 0; i < cq.length(); ++i) {
    if (cq[i] == gc_) {
      return true;
    }
  }
  return false;
}

DDSGuardCondition::DDSGuardCondition()
  : gc_(RmwAllocateFree<DDS::GuardCondition>::create())
{
  if (!gc_) {
    throw std::runtime_error("DDSGuardCondition constructor failed");
  }
}

DDSGuardCondition::~DDSGuardCondition()
{
  RmwAllocateFree<DDS::GuardCondition>::destroy(gc_);
}
