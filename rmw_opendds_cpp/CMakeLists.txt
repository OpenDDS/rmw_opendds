cmake_minimum_required(VERSION 3.5)

project(rmw_opendds_cpp)

set(OPENDDS_STATIC_DISABLE $ENV{OPENDDS_STATIC_DISABLE}
  CACHE BOOL "If OpenDDS Static should be disabled.")

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)

find_package(opendds_cmake_module REQUIRED)
find_package(OpenDDS QUIET MODULE)
if(OpenDDS_FOUND)
  find_package(OpenDDS MODULE)
endif()
if(NOT OpenDDS_FOUND OR OPENDDS_STATIC_DISABLE)
  ament_package()
  if(NOT OpenDDS_FOUND)
    message(WARNING "Could not find OpenDDS - skipping '${PROJECT_NAME}'")
  else()
    message(STATUS "OpenDDS static rmw implementation explicitly disabled - skipping '${PROJECT_NAME}'")
  endif()
  return()
endif()

find_package(rcutils REQUIRED)

find_package(rosidl_typesupport_opendds_c)
if(NOT rosidl_typesupport_opendds_c_FOUND)
  ament_package()
  message(WARNING "Could not find ROSIDL TypeSupport for OpenDDS (rosidl_typesupport_opendds_c) - skipping '${PROJECT_NAME}'")
  return()
endif()

find_package(rosidl_typesupport_opendds_cpp)
if(NOT rosidl_typesupport_opendds_cpp_FOUND)
  ament_package()
  message(WARNING "Could not find ROSIDL TypeSupport for OpenDDS (rosidl_typesupport_opendds_cpp) - skipping '${PROJECT_NAME}'")
  return()
endif()

find_package(rmw REQUIRED)
find_package(rmw_opendds_shared_cpp REQUIRED)
find_package(rosidl_generator_c REQUIRED)
find_package(rosidl_generator_cpp REQUIRED)

ament_export_include_directories(include)
ament_export_dependencies(
  rcutils
  rmw
  rmw_opendds_shared_cpp
  rosidl_generator_c
  rosidl_generator_cpp
  rosidl_typesupport_opendds_c
  rosidl_typesupport_opendds_cpp)

register_rmw_implementation(
  "c:rosidl_typesupport_c:rosidl_typesupport_opendds_c"
  "cpp:rosidl_typesupport_cpp:rosidl_typesupport_opendds_cpp"
)


# Build RMW library.
include_directories(${OpenDDS_HEADER_DIRS})
link_directories(${OpenDDS_LIBRARY_DIRS})
add_library(
  rmw_opendds_cpp
  SHARED
  src/get_client.cpp
  src/get_participant.cpp
  src/get_publisher.cpp
  src/get_service.cpp
  src/get_subscriber.cpp
  src/identifier.cpp
  src/process_topic_and_service_names.cpp
  src/rmw_client.cpp
  src/rmw_compare_gid_equals.cpp
  src/rmw_count.cpp
  src/rmw_get_gid_for_publisher.cpp
  src/rmw_get_implementation_identifier.cpp
  src/rmw_get_serialization_format.cpp
  src/rmw_guard_condition.cpp
  src/rmw_init.cpp
  src/rmw_logging.cpp
  src/rmw_node.cpp
  src/rmw_node_info_and_types.cpp
  src/rmw_node_names.cpp
  src/rmw_publish.cpp
  src/rmw_publisher.cpp
  src/rmw_request.cpp
  src/rmw_response.cpp
  src/rmw_serialize.cpp
  src/rmw_service.cpp
  src/rmw_service_names_and_types.cpp
  src/rmw_service_server_is_available.cpp
  src/rmw_subscription.cpp
  src/rmw_take.cpp
  src/rmw_topic_names_and_types.cpp
  src/rmw_trigger_guard_condition.cpp
  src/rmw_wait.cpp
  src/rmw_wait_set.cpp
  src/serialization_format.cpp)

# Declare external package dependencies for building.
ament_target_dependencies(rmw_opendds_cpp
  "rcutils"
  "rmw"
  "rmw_opendds_shared_cpp"
  "rosidl_generator_c"
  "rosidl_generator_cpp"
  "rosidl_typesupport_opendds_c"
  "rosidl_typesupport_opendds_cpp"
  "OpenDDS")
configure_rmw_library(rmw_opendds_cpp)
ament_export_libraries(rmw_opendds_cpp)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

# Make this package available to other ROS-2 packages.
ament_package(CONFIG_EXTRAS "${PROJECT_NAME}-extras.cmake")

install(
  TARGETS rmw_opendds_cpp
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
