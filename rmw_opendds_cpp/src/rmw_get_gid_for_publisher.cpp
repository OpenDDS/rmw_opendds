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

#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/rmw.h"

#include "rmw_opendds_cpp/identifier.hpp"
#include "rmw_opendds_cpp/opendds_static_publisher_info.hpp"

extern "C"
{
rmw_ret_t
rmw_get_gid_for_publisher(const rmw_publisher_t * publisher, rmw_gid_t * gid)
{
  auto publisher_info = OpenDDSStaticPublisherInfo::get_from(publisher);
  if (!publisher_info) {
    return RMW_RET_ERROR;
  }
  if (!gid) {
    RMW_SET_ERROR_MSG("gid is null");
    return RMW_RET_ERROR;
  }
  *gid = publisher_info->publisher_gid_;
  return RMW_RET_OK;
}
}  // extern "C"
