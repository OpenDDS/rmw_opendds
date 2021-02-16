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

#include <rmw_opendds_shared_cpp/DDSServer.hpp>
#include <rmw_opendds_shared_cpp/identifier.hpp>

DDSServer * DDSServer::from(const rmw_service_t * service)
{
  if (!service) {
    RMW_SET_ERROR_MSG("rmw_service is null");
    return nullptr;
  }
  if (!check_impl_id(service->implementation_identifier)) {
    return nullptr;
  }
  auto dds_server = static_cast<DDSServer *>(service->data);
  if (!dds_server) {
    RMW_SET_ERROR_MSG("dds_server is null");
  }
  return dds_server;
}

bool DDSServer::take(rmw_service_info_t * request_header, void * ros_request) const
{
  return service_.take_request(replier_, request_header, ros_request);
}

bool DDSServer::send(const rmw_request_id_t * request_header, const void * ros_reply) const
{
  return service_.send_reply(replier_, request_header, ros_reply);
}

void DDSServer::add_to(OpenDDSNode * dds_node)
{
  DDS::TopicDescription_ptr rtd = reader_->get_topicdescription();
  if (!rtd) {
    throw std::runtime_error("get_topicdescription failed");
  }
  CORBA::String_var name = rtd->get_name();
  CORBA::String_var type_name = rtd->get_type_name();
  if (!name || !type_name) {
    throw std::runtime_error("topicdescription name or type_name is null");
  }
  dds_node->add_sub(reader_->get_instance_handle(), name.in(), type_name.in());

  DDS::Topic_ptr wt = writer_->get_topic();
  if (!wt) {
    throw std::runtime_error("writer->get_topic failed");
  }
  name = wt->get_name();
  type_name = wt->get_type_name();
  if (!name || !type_name) {
    throw std::runtime_error("writer topic name or type_name is null");
  }
  dds_node->add_pub(writer_->get_instance_handle(), name.in(), type_name.in());
}

bool DDSServer::remove_from(OpenDDSNode * dds_node)
{
  if (dds_node) {
    return dds_node->remove_pub(writer_->get_instance_handle())
        && dds_node->remove_sub(reader_->get_instance_handle());
  }
  return false;
}

DDSServer::DDSServer(const rosidl_service_type_support_t * ts
  , const char * service_name
  , const rmw_qos_profile_t * rmw_qos
  , DDS::DomainParticipant_var dp
) : service_(ts, service_name, rmw_qos, dp)
  , replier_(nullptr)
{
  try {
    replier_ = service_.create_replier();
    if (!replier_) {
      throw std::runtime_error("create_replier failed");
    }
    reader_ = service_.get_request_reader(replier_);
    if (!reader_) {
      throw std::runtime_error("request reader_ is null");
    }
    read_condition_ = reader_->create_readcondition(DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
    if (!read_condition_) {
      throw std::runtime_error("create_readcondition failed");
    }
    writer_ = service_.get_reply_writer(replier_);
    if (!writer_) {
      throw std::runtime_error("reply writer_ is null");
    }
  } catch (const std::exception& e) {
    RMW_SET_ERROR_MSG(e.what());
    cleanup();
    throw;
  } catch (...) {
    RMW_SET_ERROR_MSG("DDSServer constructor failed");
    cleanup();
    throw;
  }
}

void DDSServer::cleanup()
{
  if (replier_) {
    if (writer_) {
      writer_ = nullptr;
    }
    if (reader_) {
      if (read_condition_) {
        if (reader_->delete_readcondition(read_condition_) != DDS::RETCODE_OK) {
          RMW_SET_ERROR_MSG("DDSServer failed to delete_readcondition");
        }
        read_condition_ = nullptr;
      }
      reader_ = nullptr;
    }
    service_.destroy_replier(replier_);
    replier_ = nullptr;
  }
}
