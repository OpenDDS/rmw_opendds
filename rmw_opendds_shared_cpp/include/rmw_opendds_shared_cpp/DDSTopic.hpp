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

#ifndef RMW_OPENDDS_SHARED_CPP__DDSTOPIC_HPP_
#define RMW_OPENDDS_SHARED_CPP__DDSTOPIC_HPP_

#include <opendds_static_serialized_dataTypeSupportImpl.h>

#include <rosidl_typesupport_opendds_cpp/message_type_support.h>

#include <rmw/types.h>

#include <string>

class DDSTopic
{
public:
  DDSTopic(const rosidl_message_type_support_t * ts, const char * topic_name,
           const rmw_qos_profile_t * rmw_qos, DDS::DomainParticipant_var dp);
  ~DDSTopic();
  const message_type_support_callbacks_t * callbacks() const { return cb_; }
  const std::string& name() const { return name_; }
  const std::string& type() const { return type_; }
  DDS::Topic_var get() const { return topic_; }

private:
  const rosidl_message_type_support_t & get_type_support(const rosidl_message_type_support_t * mts) const;
  const message_type_support_callbacks_t * get_callbacks(const rosidl_message_type_support_t * mts) const;
  std::string create_topic_name(const char * topic_name, const rmw_qos_profile_t * rmw_qos) const;
  std::string create_type_name() const;
  void register_type(DDS::DomainParticipant_var dp);
  void find_or_create_topic(DDS::DomainParticipant_var dp);

  const message_type_support_callbacks_t * cb_;
  const std::string name_;
  const std::string type_;
  DDS::Topic_var topic_;
};

#endif  // RMW_OPENDDS_SHARED_CPP__DDSTOPIC_HPP_
