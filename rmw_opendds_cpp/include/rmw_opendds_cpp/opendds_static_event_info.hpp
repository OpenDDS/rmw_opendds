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

#ifndef RMW_OPENDDS_CPP__OPENDDS_STATIC_EVENT_INFO_HPP_
#define RMW_OPENDDS_CPP__OPENDDS_STATIC_EVENT_INFO_HPP_

#include "rmw/ret_types.h"

struct OpenDDSCustomEventInfo
{
  virtual ~OpenDDSCustomEventInfo() = default;

  /// Get the corresponding rmw status given the status mask.
  /**
   * Return the corresponding RMW status given the input DDS_StatusMask and its corresponding event.
   * \param mask input DDS_StatusMask
   * \param event to fill
   * \return `RMW_RET_OK` if the event was filled or
   * \return `RMW_RET_UNSUPPORTED` if the status mask is unsupported
   */
  virtual rmw_ret_t get_status(const DDS::StatusMask mask, void * event) = 0;
  virtual DDS::Entity * get_entity() = 0;
};

#endif  // RMW_OPENDDS_CPP__OPENDDS_STATIC_EVENT_INFO_HPP_
