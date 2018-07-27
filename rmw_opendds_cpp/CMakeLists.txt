cmake_minimum_required(VERSION 3.5)

project(rmw_opendds_cpp)

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Load required CMake packages.
find_package(ament_cmake REQUIRED)
find_package(opendds_cmake_module REQUIRED)
find_package(OpenDDS MODULE)
find_package(rosidl_typesupport_opendds_c REQUIRED)
find_package(rosidl_typesupport_opendds_cpp REQUIRED)
find_package(rcutils REQUIRED)
find_package(rmw REQUIRED)
find_package(rosidl_generator_c REQUIRED)
find_package(rosidl_generator_cpp REQUIRED)

# Declare external package dependencies for CMake.
ament_export_dependencies(
  rcutils
  rmw
  rosidl_generator_c
  rosidl_generator_cpp
  rosidl_typesupport_opendds_c
  rosidl_typesupport_opendds_cpp
)

# Build RMW library.
include_directories(${OpenDDS_HEADER_DIRS})
link_directories(${OpenDDS_LIBRARY_DIRS})
add_library(rmw_opendds_cpp SHARED src/rmw_opendds.cpp)

# Declare external package dependencies for building.
ament_target_dependencies(rmw_opendds_cpp
  "rcutils"
  "rmw"
  "rosidl_generator_c"
  "rosidl_generator_cpp"
  "rosidl_typesupport_opendds_c"
  "rosidl_typesupport_opendds_cpp"
  "OpenDDS")
configure_rmw_library(rmw_opendds_cpp)
ament_export_libraries(rmw_opendds_cpp)

# Register our implementation for the C++ RMW.
register_rmw_implementation(
  "c:rosidl_typesupport_c:rosidl_typesupport_opendds_c"
  "cpp:rosidl_typesupport_cpp:rosidl_typesupport_opendds_cpp")



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