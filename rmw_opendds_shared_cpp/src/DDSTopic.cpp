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

#include <rmw_opendds_shared_cpp/DDSTopic.hpp>
#include <rmw_opendds_shared_cpp/namespace_prefix.hpp>
#include <rmw_opendds_shared_cpp/opendds_include.hpp>

#include <rosidl_typesupport_opendds_c/identifier.h>
#include <rosidl_typesupport_opendds_cpp/identifier.hpp>

#include <dds/DCPS/Marked_Default_Qos.h>

#include <rmw/error_handling.h>
#include <rmw/ret_types.h>
#include <rmw/types.h>

#include <sstream>

DDSTopic::DDSTopic(const rosidl_message_type_support_t * ts
  , const char * topic_name
  , const rmw_qos_profile_t * rmw_qos
  , DDS::DomainParticipant_var dp
) : cb_(get_callbacks(ts))
  , name_(create_topic_name(topic_name, rmw_qos))
  , type_(create_type_name())
{
  register_type(dp);
  find_or_create_topic(dp);
}

DDSTopic::~DDSTopic()
{
  cb_ = nullptr;
}

const rosidl_message_type_support_t & DDSTopic::get_type_support(const rosidl_message_type_support_t * mts) const
{
  if (!mts) {
    throw std::runtime_error("DDSTopic type support is null");
  }
  const rosidl_message_type_support_t * ts = get_message_typesupport_handle(mts, rosidl_typesupport_opendds_c__identifier);
  if (!ts) {
    ts = get_message_typesupport_handle(mts, rosidl_typesupport_opendds_cpp::typesupport_identifier);
    if (!ts) {
      std::ostringstream s; s << "type support implementation '" << mts->typesupport_identifier
        << "' does not match '" << rosidl_typesupport_opendds_cpp::typesupport_identifier << '\'';
      throw std::runtime_error(s.str());
    }
  }
  return *ts;
}

const message_type_support_callbacks_t * DDSTopic::get_callbacks(const rosidl_message_type_support_t * mts) const
{
  auto cb = static_cast<const message_type_support_callbacks_t *>(get_type_support(mts).data);
  if (cb) {
    return cb;
  }
  throw std::runtime_error("DDSTopic callbacks_ is null");
}

std::string DDSTopic::create_topic_name(const char * topic_name, const rmw_qos_profile_t * rmw_qos) const
{
  if (!topic_name || strlen(topic_name) == 0) {
    throw std::runtime_error("topic_name is null or empty");
  }
  if (!rmw_qos) {
    throw std::runtime_error("rmw_qos is null");
  }
  if (rmw_qos->avoid_ros_namespace_conventions) {
    return topic_name;
  }
  return std::string(ros_topic_prefix) + topic_name;
}

std::string DDSTopic::create_type_name() const
{
  if (cb_) {
    return std::string(cb_->message_namespace) + "::dds_::" + cb_->message_name + "_";
  }
  throw std::runtime_error("create_type_name failed");
}

void DDSTopic::register_type(DDS::DomainParticipant_var dp)
{
  if (!dp) {
    throw std::runtime_error("DomainParticipant is null");
  }
  OpenDDSStaticSerializedDataTypeSupport_var ts = new OpenDDSStaticSerializedDataTypeSupportImpl();
  if (ts->register_type(dp, type_.c_str()) != DDS::RETCODE_OK) {
    throw std::runtime_error("register_type failed");
  }
}

void DDSTopic::find_or_create_topic(DDS::DomainParticipant_var dp)
{
  if (!dp) {
    throw std::runtime_error("DomainParticipant is null");
  }
  DDS::TopicDescription_var td = dp->lookup_topicdescription(name_.c_str());
  topic_ = td ? dp->find_topic(name_.c_str(), DDS::Duration_t{0, 0}) :
    dp->create_topic(name_.c_str(), type_.c_str(), TOPIC_QOS_DEFAULT, NULL, OpenDDS::DCPS::NO_STATUS_MASK);
  if (!topic_) {
    throw std::runtime_error(std::string(td ? "find" : "create") + "_topic failed");
  }
}
