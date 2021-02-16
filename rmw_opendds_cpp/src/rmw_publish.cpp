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

#include <rmw_opendds_shared_cpp/DDSPublisher.hpp>

#include <dds/DCPS/DataWriterImpl.h>

#include <ace/Message_Block.h>

#include <rmw/error_handling.h>
#include <rmw/rmw.h>
#include <rmw/types.h>

#include <limits>

static const size_t buffer_max = (std::numeric_limits<CORBA::ULong>::max)();

bool
publish(DDS::DataWriter * dds_data_writer, const rcutils_uint8_array_t * cdr_stream)
{
  OpenDDSStaticSerializedDataDataWriter_var writer = OpenDDSStaticSerializedDataDataWriter::_narrow(dds_data_writer);
  if (!writer) {
    RMW_SET_ERROR_MSG("failed to narrow data writer");
    return false;
  }
  if (cdr_stream->buffer_length > buffer_max) {
    RMW_SET_ERROR_MSG("cdr_stream->buffer_length > buffer_max");
    return false;
  }

  OpenDDSStaticSerializedData instance;
  instance.serialized_data.length(static_cast<CORBA::ULong>(cdr_stream->buffer_length));
  std::memcpy(instance.serialized_data.get_buffer(), cdr_stream->buffer, cdr_stream->buffer_length);

  DDS::ReturnCode_t status = writer->write(instance, DDS::HANDLE_NIL);
  return status == DDS::RETCODE_OK;
}

extern "C"
{
rmw_ret_t
rmw_publish(
  const rmw_publisher_t * publisher,
  const void * ros_message,
  rmw_publisher_allocation_t * allocation)
{
  ACE_UNUSED_ARG(allocation);
  auto dds_pub = DDSPublisher::from(publisher);
  if (!dds_pub) {
    return RMW_RET_ERROR; // error set
  }

  auto ret = RMW_RET_ERROR;
  rcutils_uint8_array_t cdr_stream = rcutils_get_zero_initialized_uint8_array();
  cdr_stream.allocator = rcutils_get_default_allocator();
  try {
    ret = dds_pub->to_cdr_stream(ros_message, cdr_stream);
    if (ret != RMW_RET_OK) {
      return ret; //error set
    }
    if (cdr_stream.buffer_length == 0) {
      throw std::runtime_error("no message length set");
    }
    if (!cdr_stream.buffer) {
      throw std::runtime_error("no serialized message attached");
    }
    if (!publish(dds_pub->writer(), &cdr_stream)) {
      throw std::runtime_error("failed to publish message");
    }
    ret = RMW_RET_OK;
  } catch (const std::exception& e) {
    RMW_SET_ERROR_MSG(e.what());
  } catch (...) {
    RMW_SET_ERROR_MSG("rmw_publish failed");
  }
  cdr_stream.allocator.deallocate(cdr_stream.buffer, cdr_stream.allocator.state);
  return ret;
}

rmw_ret_t
rmw_publish_serialized_message(
  const rmw_publisher_t * publisher,
  const rmw_serialized_message_t * serialized_message,
  rmw_publisher_allocation_t * allocation)
{
  ACE_UNUSED_ARG(allocation);
  auto dds_pub = DDSPublisher::from(publisher);
  if (!dds_pub) {
    return RMW_RET_ERROR;
  }
  if (!serialized_message) {
    RMW_SET_ERROR_MSG("serialized message is null");
    return RMW_RET_ERROR;
  }
  if (!publish(dds_pub->writer(), serialized_message)) {
    RMW_SET_ERROR_MSG("failed to publish message");
    return RMW_RET_ERROR;
  }
  return RMW_RET_OK;
}

rmw_ret_t
rmw_publish_loaned_message(
  const rmw_publisher_t * publisher,
  void * ros_message,
  rmw_publisher_allocation_t * allocation)
{
  (void) publisher;
  (void) ros_message;
  (void) allocation;

  RMW_SET_ERROR_MSG("rmw_publish_loaned_message not implemented for rmw_opendds_cpp");
  return RMW_RET_UNSUPPORTED;
}
}  // extern "C"
