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
ament_export_dependencies(rmw)
ament_export_dependencies(rosidl_cmake)
ament_export_dependencies(rosidl_generator_c)
ament_export_dependencies(rosidl_generator_cpp)
ament_export_dependencies(rosidl_generator_dds_idl)
ament_export_dependencies(rosidl_typesupport_interface)

ament_export_include_directories(include)

add_library(${PROJECT_NAME} SHARED
  src/identifier.cpp
  src/misc.cpp
  src/u_instanceHandle.c)

target_include_directories(${PROJECT_NAME}
  PUBLIC
  include
)

ament_target_dependencies(${PROJECT_NAME} "OpenDDS")
ament_export_libraries(${PROJECT_NAME})

ament_index_register_resource("rosidl_typesupport_cpp")

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

install(
  DIRECTORY cmake
  DESTINATION share/${PROJECT_NAME}
)

install(
  DIRECTORY include/
  DESTINATION include
)
install(
  TARGETS ${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
