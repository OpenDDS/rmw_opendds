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

static rmw_ret_t
take(
  const rmw_subscription_t * subscription,
  rmw_serialized_message_t * cdr_stream,
  bool * taken,
  rmw_message_info_t * message_info)
{
  RMW_CHECK_FOR_NULL_WITH_MSG(subscription, "subscription is null", return RMW_RET_ERROR);
  RMW_CHECK_TYPE_IDENTIFIERS_MATCH(subscription handle,
    subscription->implementation_identifier, opendds_identifier, return RMW_RET_ERROR)

  auto info = static_cast<OpenDDSStaticSubscriberInfo *>(subscription->data);
  RMW_CHECK_FOR_NULL_WITH_MSG(info, "subscriber info is null", return RMW_RET_ERROR);
  RMW_CHECK_FOR_NULL_WITH_MSG(info->topic_reader_, "topic_reader_ is null", return RMW_RET_ERROR);
  RMW_CHECK_FOR_NULL_WITH_MSG(info->callbacks_, "callbacks_ is null", return RMW_RET_ERROR);

  RMW_CHECK_FOR_NULL_WITH_MSG(cdr_stream, "cdr_stream is null", return RMW_RET_ERROR);
  RMW_CHECK_FOR_NULL_WITH_MSG(taken, "taken is null", return RMW_RET_ERROR);
  *taken = false;

  OpenDDSStaticSerializedDataDataReader_var reader =
    OpenDDSStaticSerializedDataDataReader::_narrow(info->topic_reader_);
  if (!reader) {
    RMW_SET_ERROR_MSG("failed to narrow data reader");
    return RMW_RET_ERROR;
  }

  OpenDDSStaticSerializedDataSeq dds_messages;
  DDS::SampleInfoSeq sample_infos;
  DDS::ReturnCode_t status = reader->take(dds_messages, sample_infos, 1,
    DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  if (DDS::RETCODE_OK == status) {
    DDS::SampleInfo & info = sample_infos[0];
    if (info.valid_data) {
      cdr_stream->buffer_length = dds_messages[0].serialized_data.length();
      if (cdr_stream->buffer_length <= (std::numeric_limits<unsigned int>::max)()) {
        cdr_stream->buffer = reinterpret_cast<uint8_t *>(malloc(cdr_stream->buffer_length * sizeof(uint8_t)));
        for (unsigned int i = 0; i < static_cast<unsigned int>(cdr_stream->buffer_length); ++i) {
          cdr_stream->buffer[i] = dds_messages[0].serialized_data[i];
        }
        *taken = true;

        if (message_info) {
          message_info->publisher_gid->implementation_identifier = opendds_identifier;
          memset(message_info->publisher_gid->data, 0, RMW_GID_STORAGE_SIZE);
          auto detail = reinterpret_cast<OpenDDSPublisherGID *>(message_info->publisher_gid->data);
          detail->publication_handle = info.publication_handle;
        }
      } else {
        RMW_SET_ERROR_MSG("cdr_stream->buffer_length > max unsigned int");
      }
    }
  } else if (DDS::RETCODE_NO_DATA != status) {
    RMW_SET_ERROR_MSG("take failed");
  }

  reader->return_loan(dds_messages, sample_infos);
  return (*taken || DDS::RETCODE_NO_DATA == status) ? RMW_RET_OK : RMW_RET_ERROR;
}

rmw_ret_t
take(
  void * ros_message,
  const rmw_subscription_t * subscription,
  bool * taken,
  rmw_message_info_t * message_info)
{
  RMW_CHECK_FOR_NULL_WITH_MSG(ros_message, "ros_message is null", return RMW_RET_ERROR);
  rcutils_uint8_array_t cdr_stream = rcutils_get_zero_initialized_uint8_array();
  rmw_ret_t ret = take(subscription, &cdr_stream, taken, message_info);
  if (RMW_RET_OK == ret) {
    if (*taken) {
      auto info = static_cast<OpenDDSStaticSubscriberInfo *>(subscription->data);
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
take_serialized_message(
  rmw_serialized_message_t * serialized_msg,
  const rmw_subscription_t * subscription,
  bool * taken,
  rmw_message_info_t * message_info)
{
  RMW_CHECK_FOR_NULL_WITH_MSG(serialized_msg, "serialized_msg is null", return RMW_RET_ERROR);
  return take(subscription, serialized_msg, taken, message_info);
}

extern "C"
{
rmw_ret_t
rmw_take(
  const rmw_subscription_t * subscription,
  void * ros_message,
  bool * taken,
  rmw_subscription_allocation_t * allocation)
{
  return take(ros_message, subscription, taken, nullptr);
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
  return take(ros_message, subscription, taken, message_info);
}

rmw_ret_t
rmw_take_serialized_message(
  const rmw_subscription_t * subscription,
  rmw_serialized_message_t * serialized_msg,
  bool * taken,
  rmw_subscription_allocation_t * allocation)
{
  return take_serialized_message(serialized_msg, subscription, taken, nullptr);
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
  return take_serialized_message(serialized_msg, subscription, taken, message_info);
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
