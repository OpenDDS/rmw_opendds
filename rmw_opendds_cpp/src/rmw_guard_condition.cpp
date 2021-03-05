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

#include <rmw/impl/cpp/macros.hpp>
#include <rmw/rmw.h>

extern "C"
{
void clean_guard_condition(rmw_guard_condition_t * guard_condition)
{
  if (guard_condition) {
    auto dds_gc = DDSGuardCondition::from(guard_condition->data);
    if (dds_gc) {
      DDSGuardCondition::Raf::destroy(dds_gc);
      guard_condition->data = nullptr;
    }
    rmw_guard_condition_free(guard_condition);
  }
}

rmw_guard_condition_t *
rmw_create_guard_condition(rmw_context_t * context)
{
  RMW_CHECK_FOR_NULL_WITH_MSG(context, "context is null", nullptr);
  if (!check_impl_id(context->implementation_identifier)) {
    return nullptr;
  }
  RMW_CHECK_FOR_NULL_WITH_MSG(context->impl, "context->impl is null", nullptr);

  rmw_guard_condition_t * guard_condition = rmw_guard_condition_allocate();
  if (!guard_condition) {
    RMW_SET_ERROR_MSG("rmw_guard_condition_allocate failed");
    return nullptr;
  }
  try {
    guard_condition->context = context;
    guard_condition->implementation_identifier = opendds_identifier;
    guard_condition->data = DDSGuardCondition::Raf::create();
    if (!guard_condition->data) {
      throw std::runtime_error("failed to create DDSGuardCondition");
    }
    return guard_condition;
  } catch (const std::exception& e) {
    RMW_SET_ERROR_MSG(e.what());
  } catch (...) {
    RMW_SET_ERROR_MSG("rmw_create_guard_condition unkown exception");
  }
  clean_guard_condition(guard_condition);
  return nullptr;
}

rmw_ret_t
rmw_destroy_guard_condition(rmw_guard_condition_t * guard_condition)
{
  if (!guard_condition) {
    RMW_SET_ERROR_MSG("guard condition is null");
    return RMW_RET_INVALID_ARGUMENT;
  }
  if (!check_impl_id(guard_condition->implementation_identifier)) {
    return RMW_RET_INCORRECT_RMW_IMPLEMENTATION;
  }
  clean_guard_condition(guard_condition);
  return RMW_RET_OK;
}

rmw_ret_t
rmw_trigger_guard_condition(const rmw_guard_condition_t * rmw_guard_condition)
{
  auto dds_gc = DDSGuardCondition::from(rmw_guard_condition);
  return dds_gc ? dds_gc->set() : RMW_RET_INVALID_ARGUMENT;
}
}  // extern "C"
