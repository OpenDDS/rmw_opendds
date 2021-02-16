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

#include <rmw_opendds_shared_cpp/DDSSubscriber.hpp>
#include <rmw_opendds_shared_cpp/identifier.hpp>
#include <rmw_opendds_shared_cpp/types.hpp>

#include <rmw/error_handling.h>
#include <rmw/impl/cpp/macros.hpp>
#include <rmw/types.h>

#include <limits>

static const size_t buffer_max = (std::numeric_limits<CORBA::ULong>::max)();

static rmw_ret_t
take(
  DDSSubscriber & dds_sub,
  rmw_serialized_message_t * cdr_stream,
  bool & taken,
  rmw_message_info_t * message_info)
{
  RMW_CHECK_FOR_NULL_WITH_MSG(cdr_stream, "cdr_stream is null", return RMW_RET_ERROR);
  taken = false;
  OpenDDSStaticSerializedDataDataReader_var reader = OpenDDSStaticSerializedDataDataReader::_narrow(dds_sub.get_entity());
  if (!reader) {
    RMW_SET_ERROR_MSG("failed to narrow data reader");
    return RMW_RET_ERROR;
  }

  OpenDDSStaticSerializedDataSeq msgs;
  DDS::SampleInfoSeq infos;
  DDS::ReturnCode_t rc = reader->take(msgs, infos, 1, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  if (DDS::RETCODE_OK == rc) {
    DDS::SampleInfo & info = infos[0];
    if (info.valid_data) {
      const size_t length = msgs[0].serialized_data.length();
      if (length <= buffer_max) {
        cdr_stream->buffer_length = length;
        cdr_stream->buffer_capacity = length;
        cdr_stream->buffer = (uint8_t *)cdr_stream->allocator.allocate(length, cdr_stream->allocator.state);
        if (cdr_stream->buffer) {
          std::memcpy(cdr_stream->buffer, msgs[0].serialized_data.get_buffer(), length);
          taken = true;

          if (message_info) {
            message_info->publisher_gid.implementation_identifier = opendds_identifier;
            memset(message_info->publisher_gid.data, 0, RMW_GID_STORAGE_SIZE);
            auto detail = reinterpret_cast<OpenDDSPublisherGID *>(message_info->publisher_gid.data);
            detail->publication_handle = info.publication_handle;
          }
        } else {
          RMW_SET_ERROR_MSG("failed to allocate memory for uint8 array");
        }
      } else {
        RMW_SET_ERROR_MSG("dds message length > buffer_max");
      }
    }
  } else if (DDS::RETCODE_NO_DATA != rc) {
    RMW_SET_ERROR_MSG("take failed");
  }

  reader->return_loan(msgs, infos);
  return (taken || DDS::RETCODE_NO_DATA == rc) ? RMW_RET_OK : RMW_RET_ERROR;
}

rmw_ret_t
take(
  void * ros_message,
  const rmw_subscription_t * subscription,
  bool * taken,
  rmw_message_info_t * message_info)
{
  RMW_CHECK_FOR_NULL_WITH_MSG(ros_message, "ros_message is null", return RMW_RET_ERROR);
  auto dds_sub = DDSSubscriber::from(subscription);
  if (!dds_sub) {
    return RMW_RET_ERROR;
  }
  RMW_CHECK_FOR_NULL_WITH_MSG(taken, "taken is null", return RMW_RET_ERROR);
  rcutils_uint8_array_t cdr_stream = {nullptr, 0lu, 0lu, rcutils_get_default_allocator()};
  rmw_ret_t ret = take(*dds_sub, &cdr_stream, *taken, message_info);
  if (RMW_RET_OK == ret) {
    if (taken) {
      ret = dds_sub->to_ros_message(cdr_stream, ros_message);
      cdr_stream.allocator.deallocate(cdr_stream.buffer, cdr_stream.allocator.state);
    }
  }
  return ret;
}

extern "C"
{
rmw_ret_t
rmw_take(
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken,
  rmw_subscription_allocation_t *)
{
  return take(ros_message, subscription, taken, nullptr);
}

rmw_ret_t
rmw_take_with_info(
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken,
  rmw_message_info_t * message_info,
  rmw_subscription_allocation_t *)
{
  RMW_CHECK_FOR_NULL_WITH_MSG(message_info, "message info is null", return RMW_RET_ERROR);
  return take(ros_message, subscription, taken, message_info);
}

rmw_ret_t
rmw_take_serialized_message(
  const rmw_subscription_t * subscription,
  rmw_serialized_message_t * serialized_msg,
  bool * taken,
  rmw_subscription_allocation_t *)
{
  auto dds_sub = DDSSubscriber::from(subscription);
  if (!dds_sub) {
    return RMW_RET_ERROR;
  }
  return take(*dds_sub, serialized_msg, *taken, nullptr);
}

rmw_ret_t
rmw_take_serialized_message_with_info(
  const rmw_subscription_t * subscription,
  rmw_serialized_message_t * serialized_msg,
  bool * taken,
  rmw_message_info_t * message_info,
  rmw_subscription_allocation_t *)
{
  auto dds_sub = DDSSubscriber::from(subscription);
  if (!dds_sub) {
    return RMW_RET_ERROR;
  }
  RMW_CHECK_FOR_NULL_WITH_MSG(message_info, "message info is null", return RMW_RET_ERROR);
  return take(*dds_sub, serialized_msg, *taken, message_info);
}

rmw_ret_t
rmw_take_loaned_message(
  const rmw_subscription_t * subscription,
  void ** loaned_message,
  bool * taken,
  rmw_subscription_allocation_t * allocation)
{
  (void) subscription;
  (void) loaned_message;
  (void) taken;
  (void) allocation;
  RMW_SET_ERROR_MSG("rmw_take_loaned_message not implemented for rmw_opendds_cpp");
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t
rmw_take_loaned_message_with_info(
  const rmw_subscription_t * subscription,
  void ** loaned_message,
  bool * taken,
  rmw_message_info_t * message_info,
  rmw_subscription_allocation_t * allocation)
{
  (void) subscription;
  (void) loaned_message;
  (void) taken;
  (void) message_info;
  (void) allocation;
  RMW_SET_ERROR_MSG("rmw_take_loaned_message_with_info not implemented for rmw_opendds_cpp");
  return RMW_RET_UNSUPPORTED;
}

rmw_ret_t
rmw_return_loaned_message_from_subscription(
  const rmw_subscription_t * subscription,
  void * loaned_message)
{
  (void) subscription;
  (void) loaned_message;
  RMW_SET_ERROR_MSG("rmw_return_loaned_message_from_subscription not implemented for rmw_opendds_cpp");
  return RMW_RET_UNSUPPORTED;
}
}  // extern "C"
