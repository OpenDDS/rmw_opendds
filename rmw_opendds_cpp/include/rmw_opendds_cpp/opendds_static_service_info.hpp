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

#ifndef RMW_OPENDDS_CPP__OPENDDS_STATIC_SERVICE_INFO_HPP_
#define RMW_OPENDDS_CPP__OPENDDS_STATIC_SERVICE_INFO_HPP_

#include "rmw_opendds_shared_cpp/opendds_include.hpp"

#include "rosidl_typesupport_opendds_cpp/service_type_support.h"

extern "C"
{
struct OpenDDSStaticServiceInfo
{
  const service_type_support_callbacks_t * callbacks_;
  void * replier_;
  DDS::DataReader_var request_reader_;
  DDS::ReadCondition_var read_condition_;
  OpenDDSStaticServiceInfo(const service_type_support_callbacks_t * callbacks)
  : callbacks_(callbacks), replier_(nullptr) {}
};
}  // extern "C"

#endif  // RMW_OPENDDS_CPP__OPENDDS_STATIC_SERVICE_INFO_HPP_
