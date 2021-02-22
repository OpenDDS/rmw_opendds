cmake_minimum_required(VERSION 3.5)

project(rmw_opendds_cpp)

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(opendds_cmake_module REQUIRED)
find_package(OpenDDS QUIET)

if(NOT OpenDDS_FOUND)
  ament_package()
  message(WARNING "Could not find OpenDDS - skipping '${PROJECT_NAME}'")
  return()
endif()

find_package(rcutils REQUIRED)

# rcutils package changes CXX standard to 11
if(${CMAKE_CXX_STANDARD} LESS 14)
  set(CMAKE_CXX_STANDARD 14)
endif()

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
find_package(rosidl_generator_c REQUIRED)
find_package(rosidl_generator_cpp REQUIRED)

set(opendds_libs
  OpenDDS::Rtps_Udp
)

include_directories(include)

ament_export_include_directories(include)
ament_export_dependencies(
  rcutils
  rmw
  rosidl_generator_c
  rosidl_generator_cpp
  rosidl_typesupport_opendds_c
  rosidl_typesupport_opendds_cpp)

register_rmw_implementation(
  "c:rosidl_typesupport_c:rosidl_typesupport_opendds_c"
  "cpp:rosidl_typesupport_cpp:rosidl_typesupport_opendds_cpp"
)


# Build RMW library.
add_library(
  rmw_opendds_cpp
  SHARED
  src/rmw_client.cpp
  src/rmw_compare_gid_equals.cpp
  src/rmw_event.cpp
  src/rmw_get_implementation_identifier.cpp
  src/rmw_get_serialization_format.cpp
  src/rmw_guard_condition.cpp
  src/rmw_init.cpp
  src/rmw_logging.cpp
  src/rmw_node.cpp
  src/rmw_publish.cpp
  src/rmw_publisher.cpp
  src/rmw_serialize.cpp
  src/rmw_server.cpp
  src/rmw_subscription.cpp
  src/rmw_take.cpp
  src/rmw_trigger_guard_condition.cpp
  src/rmw_wait.cpp
  src/rmw_wait_set.cpp
  src/serialization_format.cpp
  src/rmw_get_topic_endpoint_info.cpp
  src/condition_error.cpp
  src/count.cpp
  src/demangle.cpp
  src/event.cpp
  src/event_converter.cpp
  src/guard_condition.cpp
  src/identifier.cpp
  src/init.cpp
  src/namespace_prefix.cpp
  src/node.cpp
  src/node_names.cpp
  src/OpenDDSNode.cpp
  src/qos.cpp
  src/names_and_types_helpers.cpp
  src/node_info_and_types.cpp
  src/service_names_and_types.cpp
  src/topic_names_and_types.cpp
  src/trigger_guard_condition.cpp
  src/wait_set.cpp
  src/types/custom_data_reader_listener.cpp
  src/types/custom_publisher_listener.cpp
  src/types/custom_subscriber_listener.cpp
  src/topic_endpoint_info.cpp
  src/wait.cpp)


OPENDDS_TARGET_SOURCES(rmw_opendds_cpp resources/opendds_static_serialized_data.idl TAO_IDL_OPTIONS "-Sa -St")
target_link_libraries(rmw_opendds_cpp ${opendds_libs})

# Declare external package dependencies for building.
ament_target_dependencies(rmw_opendds_cpp
  "rcutils"
  "rmw"
  "rosidl_generator_c"
  "rosidl_generator_cpp"
  "rosidl_typesupport_opendds_c"
  "rosidl_typesupport_opendds_cpp")

target_include_directories(rmw_opendds_cpp PUBLIC ${patched_directory})
ament_export_libraries(rmw_opendds_cpp)

if(WIN32)
  # Causes the visibility macros to use dllexport rather than dllimport
  # which is appropriate when building the dll but not consuming it.
  target_compile_definitions(rmw_opendds_cpp
    PRIVATE "RMW_OPENDDS_CPP_BUILDING_DLL"
            "_CRT_NONSTDC_NO_DEPRECATE")
endif()

# Causes the visibility macros to use dllexport rather than dllimport
# which is appropriate when building the library but not consuming it.
target_compile_definitions(rmw_opendds_cpp
  PRIVATE "RMW_OPENDDS_CPP_BUILDING_DLL")

# On Windows this adds the RMW_BUILDING_DLL definition.
# On Unix (GCC or Clang) it hides the symbols with -fvisibility=hidden.
configure_rmw_library(rmw_opendds_cpp)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()

install(
  DIRECTORY include/
  DESTINATION include
)

install(
  TARGETS rmw_opendds_cpp
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
