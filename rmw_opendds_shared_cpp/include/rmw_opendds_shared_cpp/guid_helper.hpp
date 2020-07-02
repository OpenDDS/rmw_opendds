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

#ifndef RMW_OPENDDS_SHARED_CPP__GUID_HELPER_HPP_
#define RMW_OPENDDS_SHARED_CPP__GUID_HELPER_HPP_

#include <cstring>
#include <iostream>
#include "rmw_opendds_shared_cpp/opendds_include.hpp"
#include "dds/DCPS/GuidUtils.h"

namespace DDS
{
  typedef OpenDDS::DCPS::GUID_t GUID_t;
}

inline void DDS_BuiltinTopicKey_to_GUID(DDS::GUID_t* guid, DDS::BuiltinTopicKey_t builtinTopicKey)
{
  std::memcpy(guid, builtinTopicKey.value, sizeof(DDS::BuiltinTopicKey_t));
}

#endif  // RMW_OPENDDS_SHARED_CPP__GUID_HELPER_HPP_
