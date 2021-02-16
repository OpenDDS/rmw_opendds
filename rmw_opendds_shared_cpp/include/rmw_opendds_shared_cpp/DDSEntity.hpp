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

#ifndef RMW_OPENDDS_SHARED_CPP__DDSENTITY_HPP_
#define RMW_OPENDDS_SHARED_CPP__DDSENTITY_HPP_

#include <dds/DdsDcpsInfrastructureC.h>

#include <rmw/ret_types.h>

class DDSEntity
{
public:
  // Given DDS::StatusMask, fill the corresponding rmw_status and return RMW_RET_OK.
  // Otherwise return RMW_RET_UNSUPPORTED if DDS::StatusMask is unsupported,
  // RMW_RET_TIMEOUT, or RMW_RET_ERROR on other errors.
  virtual rmw_ret_t get_status(const DDS::StatusMask mask, void * rmw_status) = 0;
  virtual DDS::Entity * get_entity() = 0;
};

#endif  // RMW_OPENDDS_SHARED_CPP__DDSENTITY_HPP_
