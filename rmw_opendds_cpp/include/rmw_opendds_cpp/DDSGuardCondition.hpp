// Copyright 2015 Open Source Robotics Foundation, Inc.
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

#ifndef RMW_OPENDDS_CPP__DDSGUARDCONDITION_HPP_
#define RMW_OPENDDS_CPP__DDSGUARDCONDITION_HPP_

#include <rmw_opendds_cpp/RmwAllocateFree.hpp>

#include <dds/DCPS/GuardCondition.h>

class DDSGuardCondition
{
public:
  typedef RmwAllocateFree<DDSGuardCondition> Raf;
  static DDSGuardCondition * from(const rmw_guard_condition_t * rmw_guard_condition);
  static DDSGuardCondition * from(void * guard_condition);
  rmw_ret_t set(bool trigger_value = true);
  bool is_in(const DDS::ConditionSeq cq) const;
  DDS::GuardCondition * gc() const { return gc_; }

private:
  friend Raf;
  DDSGuardCondition();
  ~DDSGuardCondition();
  DDS::GuardCondition * gc_;
};

#endif  // RMW_OPENDDS_CPP__DDSGUARDCONDITION_HPP_
