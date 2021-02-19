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

#include "rmw_opendds_cpp/namespace_prefix.hpp"
#include "rmw_opendds_cpp/opendds_include.hpp"
#include "opendds_static_serialized_dataTypeSupportImpl.h"
#include "rosidl_typesupport_opendds_cpp/message_type_support.h"

#include <dds/DCPS/Marked_Default_Qos.h>

#include <rmw/error_handling.h>
#include <rmw/ret_types.h>
#include <rmw/types.h>

#include <string>

class DDSEntity
{
protected:
  const message_type_support_callbacks_t * callbacks_;
public:
  const std::string topic_name_;
  const std::string type_name_;

  DDSEntity(const rosidl_message_type_support_t & ros_ts, const char * topic_name, const rmw_qos_profile_t & rmw_qos)
    : callbacks_(static_cast<const message_type_support_callbacks_t*>(ros_ts.data))
    , topic_name_(create_topic_name(topic_name, rmw_qos.avoid_ros_namespace_conventions))
    , type_name_(callbacks_ ? create_type_name(callbacks_) : "")
  {
    if (!callbacks_) {
      throw std::runtime_error("callbacks_ is null");
    }
    if (topic_name_.empty()) {
      throw std::runtime_error("topic_name_ is empty");
    }
    if (type_name_.empty()) {
      throw std::runtime_error("type_name_ is empty");
    }
  }

  virtual ~DDSEntity() { callbacks_ = nullptr; }

  std::string create_topic_name(const char * topic_name, bool avoid_ros_namespace_conventions)
  {
    if (!topic_name || strlen(topic_name) == 0) {
      return "";
    }
    if (avoid_ros_namespace_conventions) {
      return topic_name;
    }
    return std::string(ros_topic_prefix) + topic_name;
  }

  std::string create_type_name(const message_type_support_callbacks_t * cb)
  {
    return std::string(cb->message_namespace) + "::dds_::" + cb->message_name + "_";
  }

  bool register_type(DDS::DomainParticipant_var dp)
  {
    OpenDDSStaticSerializedDataTypeSupport_var ts = new OpenDDSStaticSerializedDataTypeSupportImpl();
    return ts->register_type(dp, type_name_.c_str()) == DDS::RETCODE_OK;
  }

  DDS::Topic_var find_or_create_topic(DDS::DomainParticipant_var dp)
  {
    DDS::TopicDescription_var td = dp->lookup_topicdescription(topic_name_.c_str());
    DDS::Topic_var topic = td ? dp->find_topic(topic_name_.c_str(), DDS::Duration_t{0, 0}) :
      dp->create_topic(topic_name_.c_str(), type_name_.c_str(), TOPIC_QOS_DEFAULT, NULL, OpenDDS::DCPS::NO_STATUS_MASK);
    if (!topic) {
      RMW_SET_ERROR_MSG((std::string("failed to ") + (td ? "find" : "create") + " topic").c_str());
    }
    return topic;
  }

  // Given DDS::StatusMask, fill the corresponding rmw_status and return RMW_RET_OK.
  // Otherwise return RMW_RET_UNSUPPORTED if DDS::StatusMask is unsupported,
  // RMW_RET_TIMEOUT, or RMW_RET_ERROR on other errors.
  virtual rmw_ret_t get_status(const DDS::StatusMask mask, void * rmw_status) = 0;
  virtual DDS::Entity * get() = 0;
};

#endif  // RMW_OPENDDS_SHARED_CPP__DDSENTITY_HPP_
