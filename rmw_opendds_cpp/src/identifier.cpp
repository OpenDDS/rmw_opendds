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

#include "rmw_opendds_cpp/identifier.hpp"
#include <rmw/error_handling.h>

const char * const opendds_identifier = "rmw_opendds_cpp";

bool check_impl_id(const char * implementation_identifier)
{
  if (opendds_identifier == implementation_identifier) {
    return true;
  } else if (!implementation_identifier) {
    RMW_SET_ERROR_MSG("implementation_identifier is null");
  } else {
    RMW_SET_ERROR_MSG_WITH_FORMAT_STRING(
      "implementation '%s' does not match rmw implementation '%s'",
      implementation_identifier, opendds_identifier);
  }
  return false;
}
