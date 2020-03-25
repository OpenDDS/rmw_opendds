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

#ifndef RMW_OPENDDS_STATIC_SUBSCRIBER_INFO_HPP_
#define RMW_OPENDDS_STATIC_SUBSCRIBER_INFO_HPP_

#include <atomic>

#include "rmw_opendds_shared_cpp/opendds_include.hpp"

#include "rosidl_typesupport_opendds_cpp/message_type_support.h"

class OpenDDSSubscriberListener;

extern "C"
{
struct OpenDDSStaticSubscriberInfo
{
  DDS::Subscriber * dds_subscriber_;
  OpenDDSSubscriberListener * listener_;
  DDS::DataReader * topic_reader_;
  DDS::ReadCondition * read_condition_;
  bool ignore_local_publications;
  const message_type_support_callbacks_t * callbacks_;
};
}  // extern "C"

class OpenDDSSubscriberListener : public DDS::SubscriberListener
{
public:
  virtual void on_subscription_matched(
    DDS::DataReader *,
    const DDS::SubscriptionMatchedStatus & status)
  {
    current_count_ = status.current_count;
  }

  std::size_t current_count() const
  {
    return current_count_;
  }

private:
  std::atomic<std::size_t> current_count_;
};

#endif  // RMW_OPENDDS_STATIC_SUBSCRIBER_INFO_HPP_
