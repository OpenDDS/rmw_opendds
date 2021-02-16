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

#include <rmw_opendds_shared_cpp/Service.hpp>
#include <rmw_opendds_shared_cpp/namespace_prefix.hpp>
#include <rmw_opendds_shared_cpp/qos.hpp>

#include <rosidl_typesupport_opendds_c/identifier.h>
#include <rosidl_typesupport_opendds_cpp/identifier.hpp>

#include <dds/DCPS/Marked_Default_Qos.h>

#include <rmw/allocators.h>

#include <sstream>

Service::Service(const rosidl_service_type_support_t * ts
  , const char * service_name
  , const rmw_qos_profile_t * rmw_qos
  , DDS::DomainParticipant_var dp
) : cb_(get_callbacks(ts))
  , name_(service_name ? service_name : "")
  , request_(create_request_name(rmw_qos)) // rmw_qos null-checked
  , reply_(create_reply_name(rmw_qos))
  , dp_(dp)
{
  try {
    if (name_.empty()) {
      throw std::runtime_error("Service name_ is empty");
    }
    if (!dp_) {
      throw std::runtime_error("Service DomainParticipant is null");
    }

    // create publisher
    pub_ = dp_->create_publisher(PUBLISHER_QOS_DEFAULT, NULL, OpenDDS::DCPS::NO_STATUS_MASK);
    if (!pub_) {
      throw std::runtime_error("create_publisher failed");
    }
    DDS::DataWriterQos writer_qos;
    if (!get_datawriter_qos(pub_, *rmw_qos, writer_qos)) {
      throw std::runtime_error("get_datawriter_qos failed");
    }
    if (pub_->set_default_datawriter_qos(writer_qos) != DDS::RETCODE_OK) {
      throw std::runtime_error("set_default_datawriter_qos failed");
    }

    // create subscriber
    sub_ = dp_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, NULL, OpenDDS::DCPS::NO_STATUS_MASK);
    if (!sub_) {
      throw std::runtime_error("create_subscriber failed");
    }
    DDS::DataReaderQos reader_qos;
    if (!get_datareader_qos(sub_, *rmw_qos, reader_qos)) {
      throw std::runtime_error("get_datareader_qos failed");
    }
    if (sub_->set_default_datareader_qos(reader_qos) != DDS::RETCODE_OK) {
      throw std::runtime_error("set_default_datareader_qos failed");
    }
  } catch (const std::exception& e) {
    RMW_SET_ERROR_MSG(e.what());
    cleanup();
    throw;
  } catch (...) {
    RMW_SET_ERROR_MSG("Service constructor failed");
    cleanup();
    throw;
  }
}

void Service::cleanup()
{
  if (sub_) {
    sub_ = nullptr;
  }
  if (pub_) {
    pub_ = nullptr;
  }
  if (dp_) {
    dp_ = nullptr;
  }
}

void * Service::create_requester()
{
  return cb_->create_requester(dp_, request_.c_str(), reply_.c_str(), pub_, sub_, &rmw_allocate, &rmw_free);
}

void * Service::create_replier()
{
  return cb_->create_replier(dp_, request_.c_str(), reply_.c_str(), pub_, sub_, &rmw_allocate, &rmw_free);
}

const char * Service::destroy_requester(void * requester) const
{
  return cb_->destroy_requester(requester, &rmw_free);
}

const char * Service::destroy_replier(void * replier) const
{
  return cb_->destroy_replier(replier, &rmw_free);
}

DDS::DataWriter * Service::get_request_writer(void * requester) const
{
  return static_cast<DDS::DataWriter *>(cb_->get_request_datawriter(requester));
}

DDS::DataReader * Service::get_request_reader(void * replier) const
{
  return static_cast<DDS::DataReader *>(cb_->get_request_datareader(replier));
}

DDS::DataWriter * Service::get_reply_writer(void * replier) const
{
  return static_cast<DDS::DataWriter *>(cb_->get_reply_datawriter(replier));
}

DDS::DataReader * Service::get_reply_reader(void * requester) const
{
  return static_cast<DDS::DataReader *>(cb_->get_reply_datareader(requester));
}

int64_t Service::send_request(void * requester, const void * ros_request) const
{
  return cb_->send_request(requester, ros_request);
}

bool Service::take_request(void * replier, rmw_service_info_t * request_header, void * ros_request) const
{
  return cb_->take_request(replier, request_header, ros_request);
}

bool Service::send_reply(void * replier, const rmw_request_id_t * request_header, const void * ros_reply) const
{
  return cb_->send_response(replier, request_header, ros_reply);
}

bool Service::take_reply(void * requester, rmw_service_info_t * request_header, void * ros_reply) const
{
  return cb_->take_response(requester, request_header, ros_reply);
}

const rosidl_service_type_support_t & Service::get_type_support(const rosidl_service_type_support_t * sts) const
{
  if (!sts) {
    throw std::runtime_error("service_type_support is null");
  }
  const rosidl_service_type_support_t * ts = get_service_typesupport_handle(sts, rosidl_typesupport_opendds_c__identifier);
  if (!ts) {
    ts = get_service_typesupport_handle(sts, rosidl_typesupport_opendds_cpp::typesupport_identifier);
    if (!ts) {
      std::ostringstream s; s << "type support implementation '" << sts->typesupport_identifier
        << "' does not match '" << rosidl_typesupport_opendds_cpp::typesupport_identifier << '\'';
      throw std::runtime_error(s.str());
    }
  }
  return *ts;
}

const service_type_support_callbacks_t * Service::get_callbacks(const rosidl_service_type_support_t * sts) const
{
  auto cb = static_cast<const service_type_support_callbacks_t *>(get_type_support(sts).data);
  if (cb) {
    return cb;
  }
  throw std::runtime_error("Service callbacks_ is null");
}

const std::string Service::create_request_name(const rmw_qos_profile_t * rmw_qos) const
{
  if (rmw_qos) {
    return ((rmw_qos->avoid_ros_namespace_conventions ? "" : ros_service_requester_prefix) + name_ + "Request");
  }
  throw std::runtime_error("rmw_qos is null");
}

const std::string Service::create_reply_name(const rmw_qos_profile_t * rmw_qos) const
{
  if (rmw_qos) {
    return ((rmw_qos->avoid_ros_namespace_conventions ? "" : ros_service_response_prefix) + name_ + "Reply");
  }
  throw std::runtime_error("rmw_qos is null");
}
