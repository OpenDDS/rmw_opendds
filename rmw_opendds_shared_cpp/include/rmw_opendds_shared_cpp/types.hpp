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

#ifndef RMW_OPENDDS_SHARED_CPP__TYPES_HPP_
#define RMW_OPENDDS_SHARED_CPP__TYPES_HPP_

#include "topic_cache.hpp"
#include "rmw_opendds_shared_cpp/RmwAllocateFree.hpp"
#include "rmw_opendds_shared_cpp/visibility_control.h"

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DdsDcpsDomainC.h>
#include <dds/DCPS/WaitSet.h>

#include "rmw/rmw.h"

#include <cassert>
#include <exception>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <mutex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>

enum EntityType {Publisher, Subscriber};

using DDSTopicEndpointInfo = TopicCache<DDS::GUID_t>::TopicInfo;

class CustomDataReaderListener : public DDS::DataReaderListener
{
public:
  CustomDataReaderListener(rmw_guard_condition_t * gc) : graph_guard_condition_(gc) {}

  virtual bool add_information(
    const DDS::GUID_t& participant_guid,
    const DDS::GUID_t& guid,
    const std::string & topic_name,
    const std::string & type_name,
    // TODO: uncomment when underlying qos_profile logic is implemented
    // const rmw_qos_profile_t& qos_profile,
    EntityType entity_type);

  RMW_OPENDDS_SHARED_CPP_PUBLIC
  virtual bool remove_information(
    const DDS::GUID_t& guid,
    EntityType entity_type);

  RMW_OPENDDS_SHARED_CPP_PUBLIC
  virtual void trigger_graph_guard_condition();

  size_t count_topic(const char * topic_name);

  RMW_OPENDDS_SHARED_CPP_PUBLIC
  virtual void fill_topic_endpoint_infos(
    const std::string& topic_name,
    bool no_mangle,
    std::vector<const DDSTopicEndpointInfo*>& topic_endpoint_infos);

  virtual void fill_topic_names_and_types(
    bool no_demangle,
    std::map<std::string, std::set<std::string>> & topic_names_to_types);

  virtual void fill_service_names_and_types(
    std::map<std::string, std::set<std::string>> & services);

  virtual void fill_topic_names_and_types_by_guid(
    bool no_demangle,
    std::map<std::string, std::set<std::string>> & topic_names_to_types_by_guid,
    DDS::GUID_t& participant_guid);

  virtual void fill_service_names_and_types_by_guid(
    std::map<std::string, std::set<std::string>> & services,
    DDS::GUID_t& participant_guid,
    const std::string& suffix);

  virtual void on_requested_deadline_missed(
    ::DDS::DataReader_ptr, const ::DDS::RequestedDeadlineMissedStatus&) {}

  virtual void on_requested_incompatible_qos(
    ::DDS::DataReader_ptr, const ::DDS::RequestedIncompatibleQosStatus&) {}

  virtual void on_sample_rejected(
    ::DDS::DataReader_ptr, const ::DDS::SampleRejectedStatus&) {}

  virtual void on_liveliness_changed(
    ::DDS::DataReader_ptr, const ::DDS::LivelinessChangedStatus&) {}

  virtual void on_subscription_matched(
    ::DDS::DataReader_ptr, const ::DDS::SubscriptionMatchedStatus&) {}

  virtual void on_sample_lost(
    ::DDS::DataReader_ptr, const ::DDS::SampleLostStatus&) {}

protected:
  std::mutex mutex_;
  TopicCache<DDS::GUID_t> topic_cache;

private:
  rmw_guard_condition_t * graph_guard_condition_;
};

class CustomPublisherListener : public CustomDataReaderListener
{
public:
  typedef RmwAllocateFree<CustomPublisherListener> Raf;

  CustomPublisherListener(rmw_guard_condition_t * gc) : CustomDataReaderListener(gc) {}
  ~CustomPublisherListener() {}

  virtual void on_data_available(DDS::DataReader * reader);
};

class CustomSubscriberListener : public CustomDataReaderListener
{
public:
  typedef RmwAllocateFree<CustomSubscriberListener> Raf;

  CustomSubscriberListener(rmw_guard_condition_t * gc) : CustomDataReaderListener(gc) {}
  ~CustomSubscriberListener() {}

  virtual void on_data_available(DDS::DataReader * reader);
};

struct OpenDDSPublisherGID
{
  DDS::InstanceHandle_t publication_handle;
};

struct OpenDDSWaitSetInfo
{
  DDS::WaitSet * wait_set;
  DDS::ConditionSeq * active_conditions;
  DDS::ConditionSeq * attached_conditions;
};

#endif  // RMW_OPENDDS_SHARED_CPP__TYPES_HPP_
