cmake_minimum_required(VERSION 3.5)

project(rosidl_typesupport_opendds_cpp)

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

# Declare RMW and ROS-IDL typesupport dependencies.
ament_export_dependencies(
    rmw
    rosidl_cmake
    rosidl_generator_cpp
    rosidl_generator_dds_idl
    rosidl_typesupport_interface
)

ament_index_register_resource("rosidl_typesupport_cpp")

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

# Make this package available to other ROS-2 packages.
ament_package(CONFIG_EXTRAS "${PROJECT_NAME}-extras.cmake")

# Install CMake utilities.
install(DIRECTORY cmake DESTINATION share/${PROJECT_NAME})