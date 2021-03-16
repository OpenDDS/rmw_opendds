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

#include <rmw_opendds_cpp/DDSClient.hpp>
#include <rmw_opendds_cpp/identifier.hpp>

DDSClient * DDSClient::from(const rmw_client_t * client)
{
  if (!client) {
    RMW_SET_ERROR_MSG("rmw_client is null");
    return nullptr;
  }
  if (!check_impl_id(client->implementation_identifier)) {
    return nullptr;
  }
  auto dds_client = static_cast<DDSClient *>(client->data);
  if (!dds_client) {
    RMW_SET_ERROR_MSG("dds_client is null");
  }
  return dds_client;
}

int64_t DDSClient::send(const void * ros_request) const
{
  return service_.send_request(requester_, ros_request);
}

bool DDSClient::take(rmw_service_info_t * request_header, void * ros_reply) const
{
  return service_.take_reply(requester_, request_header, ros_reply);
}

void DDSClient::add_to(OpenDDSNode * dds_node)
{
  DDS::TopicDescription_var rtd = reader_->get_topicdescription();
  if (!rtd) {
    throw std::runtime_error("get_topicdescription failed");
  }
  CORBA::String_var name = rtd->get_name();
  CORBA::String_var type_name = rtd->get_type_name();
  if (!name || !type_name) {
    throw std::runtime_error("topicdescription name or type_name is null");
  }
  dds_node->add_sub(reader_->get_instance_handle(), name.in(), type_name.in()); //?? double-check

  DDS::Topic_var wt = writer_->get_topic();
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

bool DDSClient::remove_from(OpenDDSNode * dds_node)
{
  if (dds_node) {
    return dds_node->remove_pub(writer_->get_instance_handle())
        && dds_node->remove_sub(reader_->get_instance_handle());
  }
  return false;
}

bool DDSClient::count_matched_subscribers(size_t & count) const
{
  DDS::PublicationMatchedStatus s;
  if (writer_->get_publication_matched_status(s) == DDS::RETCODE_OK) {
    count = s.current_count;
    return true;
  }
  RMW_SET_ERROR_MSG("get_publication_matched_status failed");
  return false;
}

bool DDSClient::count_matched_publishers(size_t & count) const
{
  DDS::SubscriptionMatchedStatus s;
  if (reader_->get_subscription_matched_status(s) == DDS::RETCODE_OK) {
    count = s.current_count;
    return true;
  }
  RMW_SET_ERROR_MSG("get_subscription_matched_status failed");
  return false;
}

// Check if a service server is available for the service client.
// In the OpenDDS RPC implementation, a server is ready when:
//   - at least one subscriber is matched to the request publisher;
//   - at least one publisher is matched to the reponse subscription.
bool DDSClient::is_server_available() const
{
  size_t request_subscribers = 0;
  if (!count_matched_subscribers(request_subscribers)) {
    return false;
  }
  size_t reply_publishers = 0;
  if (!count_matched_publishers(reply_publishers)) {
    return false;
  }
  return (request_subscribers != 0) && (reply_publishers != 0);
}

DDSClient::DDSClient(const rosidl_service_type_support_t * ts
  , const char * service_name
  , const rmw_qos_profile_t * rmw_qos
  , DDS::DomainParticipant_var dp
) : service_(ts, service_name, rmw_qos, dp)
  , requester_(nullptr)
{
  try {
    requester_ = service_.create_requester();
    if (!requester_) {
      throw std::runtime_error("create_requester failed");
    }
    reader_ = service_.get_reply_reader(requester_);
    if (!reader_) {
      throw std::runtime_error("reply reader_ is null");
    }
    read_condition_ = reader_->create_readcondition(DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
    if (!read_condition_) {
      throw std::runtime_error("create_readcondition failed");
    }
    writer_ = service_.get_request_writer(requester_);
    if (!writer_) {
      throw std::runtime_error("request writer_ is null");
    }
  } catch (const std::exception& e) {
    RMW_SET_ERROR_MSG(e.what());
    cleanup();
    throw;
  } catch (...) {
    RMW_SET_ERROR_MSG("DDSClient constructor failed");
    cleanup();
    throw;
  }
}

void DDSClient::cleanup()
{
  if (requester_) {
    if (writer_) {
      writer_ = nullptr;
    }
    if (reader_) {
      if (read_condition_) {
        if (reader_->delete_readcondition(read_condition_) != DDS::RETCODE_OK) {
          RMW_SET_ERROR_MSG("DDSClient failed to delete_readcondition");
        }
        read_condition_ = nullptr;
      }
      reader_ = nullptr;
    }
    service_.destroy_requester(requester_);
    requester_ = nullptr;
  }
}
