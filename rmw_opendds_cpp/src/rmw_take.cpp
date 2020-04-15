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

#include <limits>

#include "rmw/error_handling.h"
#include "rmw/impl/cpp/macros.hpp"
#include "rmw/types.h"

#include "rmw_opendds_shared_cpp/types.hpp"

#include "rmw_opendds_cpp/opendds_static_subscriber_info.hpp"
#include "rmw_opendds_cpp/identifier.hpp"

// include patched generated code from the build folder
#include "opendds_static_serialized_dataTypeSupportC.h"

static bool
take(
  DDS::DataReader * dds_data_reader,
  bool ignore_local_publications,
  rcutils_uint8_array_t * cdr_stream,
  bool * taken,
  void * sending_publication)
{
  RMW_CHECK_FOR_NULL_WITH_MSG(dds_data_reader, "dds_data_reader is null", return false);
  RMW_CHECK_FOR_NULL_WITH_MSG(cdr_stream, "cdr_stream is null", return false);
  RMW_CHECK_FOR_NULL_WITH_MSG(taken, "taken is null", return false);
  *taken = false;

  OpenDDSStaticSerializedDataDataReader_var reader =
    OpenDDSStaticSerializedDataDataReader::_narrow(dds_data_reader);
  if (!reader) {
    RMW_SET_ERROR_MSG("failed to narrow data reader");
    return false;
  }

  OpenDDSStaticSerializedDataSeq dds_messages;
  DDS::SampleInfoSeq sample_infos;
  DDS::ReturnCode_t status = reader->take(dds_messages, sample_infos, 1,
    DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  if (status != DDS::RETCODE_OK) {
    reader->return_loan(dds_messages, sample_infos);
    if (status != DDS::RETCODE_NO_DATA) {
      RMW_SET_ERROR_MSG("take failed");
      return false;
    }
    return true;
  }

  DDS::SampleInfo & info = sample_infos[0];
  if (info.valid_data) {
    cdr_stream->buffer_length = dds_messages[0].serialized_data.length();
    if (cdr_stream->buffer_length <= (std::numeric_limits<unsigned int>::max)()) {
      cdr_stream->buffer = reinterpret_cast<uint8_t *>(malloc(cdr_stream->buffer_length * sizeof(uint8_t)));
      for (unsigned int i = 0; i < static_cast<unsigned int>(cdr_stream->buffer_length); ++i) {
        cdr_stream->buffer[i] = dds_messages[0].serialized_data[i];
      }
      *taken = true;

      if (sending_publication) {
        *static_cast<DDS::InstanceHandle_t *>(sending_publication) = info.publication_handle;
      }
    } else {
      RMW_SET_ERROR_MSG("cdr_stream->buffer_length > max unsigned int");
    }
  }

  reader->return_loan(dds_messages, sample_infos);
  return *taken;
}

extern "C"
{
rmw_ret_t
_take_serialized_message(
  const rmw_subscription_t * subscription,
  rmw_serialized_message_t * serialized_msg,
  bool * taken,
  DDS::InstanceHandle_t * sending_publication)
{
  RMW_CHECK_FOR_NULL_WITH_MSG(subscription, "subscription is null", return RMW_RET_ERROR);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(subscription handle,
    subscription->implementation_identifier, opendds_identifier, return RMW_RET_ERROR)
  RMW_CHECK_FOR_NULL_WITH_MSG(serialized_msg, "serialized_msg is null", return RMW_RET_ERROR);
  RMW_CHECK_FOR_NULL_WITH_MSG(taken, "taken is null", return RMW_RET_ERROR);

  auto info = static_cast<OpenDDSStaticSubscriberInfo *>(subscription->data);
  RMW_CHECK_FOR_NULL_WITH_MSG(info, "subscriber info is null", return RMW_RET_ERROR);
  RMW_CHECK_FOR_NULL_WITH_MSG(info->topic_reader_, "topic_reader_ is null", return RMW_RET_ERROR);
  RMW_CHECK_FOR_NULL_WITH_MSG(info->callbacks_, "callbacks_ is null", return RMW_RET_ERROR);

  // fetch the incoming message as cdr stream
  if (take(info->topic_reader_.in(), info->ignore_local_publications,
           serialized_msg, taken, sending_publication)) {
    return RMW_RET_OK;
  }
  return RMW_RET_ERROR;
}

rmw_ret_t
_take(
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken,
  DDS::InstanceHandle_t * sending_publication)
{
  rmw_ret_t ret = RMW_RET_ERROR;
  RMW_CHECK_FOR_NULL_WITH_MSG(ros_message, "ros_message is null", return ret);
  rcutils_uint8_array_t cdr_stream = rcutils_get_zero_initialized_uint8_array();
  ret = _take_serialized_message(subscription, &cdr_stream, taken, sending_publication);
  if (RMW_RET_OK == ret) {
    if (*taken) {
      // convert the cdr stream to the message
/*  TODO: uncommnet this block when type support is ready.
      if (!info->callbacks_->to_message(&cdr_stream, ros_message)) {
        RMW_SET_ERROR_MSG("can't convert cdr stream to ros message");
        ret = RMW_RET_ERROR;
      }
*/
      free(cdr_stream.buffer);
    }
  }
  return ret;
}

rmw_ret_t
rmw_take(
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken,
  rmw_subscription_allocation_t * allocation)
{
  return _take(subscription, ros_message, taken, nullptr);
}

rmw_ret_t
rmw_take_with_info(
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken,
  rmw_message_info_t * message_info,
  rmw_subscription_allocation_t* allocation)
{
  RMW_CHECK_FOR_NULL_WITH_MSG(message_info, "message info is null", return RMW_RET_ERROR);
  DDS::InstanceHandle_t sending_publication;
  if (_take(subscription, ros_message, taken, &sending_publication) == RMW_RET_OK) {
    rmw_gid_t * sender_gid = &message_info->publisher_gid;
    sender_gid->implementation_identifier = opendds_identifier;
    memset(sender_gid->data, 0, RMW_GID_STORAGE_SIZE);
    auto detail = reinterpret_cast<OpenDDSPublisherGID *>(sender_gid->data);
    detail->publication_handle = sending_publication;
    return RMW_RET_OK;
  }
  return RMW_RET_ERROR;
}

rmw_ret_t
rmw_take_serialized_message(
  const rmw_subscription_t * subscription,
  rmw_serialized_message_t * serialized_msg,
  bool * taken,
  rmw_subscription_allocation_t * allocation)
{
  return _take_serialized_message(subscription, serialized_msg, taken, nullptr);
}

rmw_ret_t
rmw_take_serialized_message_with_info(
  const rmw_subscription_t * subscription,
  rmw_serialized_message_t * serialized_msg,
  bool * taken,
  rmw_message_info_t * message_info,
  rmw_subscription_allocation_t * allocation)
{
  RMW_CHECK_FOR_NULL_WITH_MSG(message_info, "message info is null", return RMW_RET_ERROR);
  DDS::InstanceHandle_t pub;
  if (_take_serialized_message(subscription, serialized_msg, taken, &pub) == RMW_RET_OK) {
    rmw_gid_t * sender_gid = &message_info->publisher_gid;
    sender_gid->implementation_identifier = opendds_identifier;
    memset(sender_gid->data, 0, RMW_GID_STORAGE_SIZE);
    auto detail = reinterpret_cast<OpenDDSPublisherGID *>(sender_gid->data);
    detail->publication_handle = pub;
    return RMW_RET_OK;
  }
  return RMW_RET_ERROR;
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
