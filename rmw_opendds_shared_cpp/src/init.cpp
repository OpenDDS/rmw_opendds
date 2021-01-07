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

#include "rmw_opendds_shared_cpp/init.hpp"
#include "rmw_opendds_shared_cpp/RmwAllocateFree.hpp"
#include "rmw/init_options.h"
#include "rmw/error_handling.h"
#include "dds/DCPS/Service_Participant.h"

rmw_context_impl_t::rmw_context_impl_t()
  : dpf_(TheParticipantFactory)
{
  if (!dpf_) {
    const char* msg = "failed to get participant factory";
    RMW_SET_ERROR_MSG(msg);
    throw std::runtime_error(msg);
  }
  TheServiceParticipant->set_default_discovery(OpenDDS::DCPS::Discovery::DEFAULT_RTPS);
}

rmw_context_impl_t::~rmw_context_impl_t()
{
  TheServiceParticipant->shutdown();
  if (dpf_) {
    dpf_ = nullptr;
  }
}

rmw_ret_t
init(rmw_context_t& context)
{
  context.impl = RmwAllocateFree<rmw_context_impl_t>::create();
  if (context.impl) {
    return RMW_RET_OK;
  }
  context = rmw_get_zero_initialized_context();
  return RMW_RET_ERROR;
}

// See rmw/init.h: rmw_ret_t rmw_shutdown(rmw_context_t * context)
// "If context has been already invalidated (`rmw_shutdown()` was called on it),
// then this function is a no-op and `RMW_RET_OK` is returned."
rmw_ret_t
shutdown(rmw_context_t& context)
{
  if (context.impl) {
    RmwAllocateFree<rmw_context_impl_t>::destroy(context.impl);
    context.impl = nullptr;
  }
  return RMW_RET_OK;
}

bool is_zero_initialized(const rmw_init_options_t * p)
{
  return p && p->instance_id == 0
    && !(p->implementation_identifier)
    && p->domain_id == RMW_DEFAULT_DOMAIN_ID
    //?? RMW_SECURITY_ENFORCEMENT_PERMISSIVE in rmw_get_default_security_options()
    && p->security_options.enforce_security == 0 && !(p->security_options.security_root_path)
    && p->localhost_only == RMW_LOCALHOST_ONLY_DEFAULT
    && !(p->enclave)
    && !(p->impl);
}
