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

#ifndef TYPE_SUPPORT_COMMON_HPP_
#define TYPE_SUPPORT_COMMON_HPP_

#include <rosidl_typesupport_opendds_c/identifier.h>
#include <rosidl_typesupport_opendds_cpp/identifier.hpp>
#include <rosidl_typesupport_opendds_cpp/message_type_support.h>
#include <rosidl_typesupport_opendds_cpp/service_type_support.h>

#include <rmw/allocators.h>
#include <rmw/error_handling.h>
#include <rmw/impl/cpp/macros.hpp>

#include <string>

inline const rosidl_message_type_support_t *
rmw_get_message_type_support(const rosidl_message_type_support_t * type_support)
{
  if (!type_support) {
    RMW_SET_ERROR_MSG("type support is null");
    return nullptr;
  }
  const rosidl_message_type_support_t * ts = get_message_typesupport_handle(
    type_support, rosidl_typesupport_opendds_c__identifier);
  if (!ts) {
    ts = get_message_typesupport_handle(type_support, rosidl_typesupport_opendds_cpp::typesupport_identifier);
    if (!ts) {
      RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("type support implementation '%s' does not match '%s'",
        type_support->typesupport_identifier, rosidl_typesupport_opendds_cpp::typesupport_identifier);
    }
  }
  return ts;
}

inline const rosidl_service_type_support_t *
rmw_get_service_type_support(const rosidl_service_type_support_t * type_support)
{
  if (!type_support) {
    RMW_SET_ERROR_MSG("type support is null");
    return nullptr;
  }
  const rosidl_service_type_support_t * ts = get_service_typesupport_handle(
    type_support, rosidl_typesupport_opendds_c__identifier);
  if (!ts) {
    ts = get_service_typesupport_handle(type_support, rosidl_typesupport_opendds_cpp::typesupport_identifier);
    if (!ts) {
      RMW_SET_ERROR_MSG_WITH_FORMAT_STRING("type support implementation '%s' does not match '%s'",
        type_support->typesupport_identifier, rosidl_typesupport_opendds_cpp::typesupport_identifier);
    }
  }
  return ts;
}

inline std::string
_create_type_name(const message_type_support_callbacks_t * callbacks)
{
  return std::string(callbacks->message_namespace) + "::dds_::" + callbacks->message_name + "_";
}

#endif  // TYPE_SUPPORT_COMMON_HPP_
