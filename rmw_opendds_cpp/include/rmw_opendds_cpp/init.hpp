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

#ifndef RMW_OPENDDS_CPP__INIT_HPP_
#define RMW_OPENDDS_CPP__INIT_HPP_

#include "rmw/types.h"

#include "rmw_opendds_cpp/visibility_control.h"

#include <dds/DdsDcpsDomainC.h>

struct rmw_context_impl_t
{
  rmw_context_impl_t();
  ~rmw_context_impl_t();
  DDS::DomainParticipantFactory_var dpf_;
};

RMW_OPENDDS_CPP_PUBLIC
rmw_ret_t init(rmw_context_t& context);

RMW_OPENDDS_CPP_PUBLIC
rmw_ret_t shutdown(rmw_context_t& context);

bool is_zero_initialized(const rmw_init_options_t * o);

bool is_zero_initialized(const rmw_context_t * c);

#endif  // RMW_OPENDDS_CPP__INIT_HPP_
