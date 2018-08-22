cmake_minimum_required(VERSION 3.5)

project(opendds_cmake_module NONE)

find_package(ament_cmake REQUIRED)

# Discover OpenDDS.
list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")
find_package(OpenDDS MODULE)

# Configure Ament testing.
if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

# Make this package available to other ROS-2 packages.
ament_package(
  CONFIG_EXTRAS "opendds_cmake_module-extras.cmake"
)

# Install CMake utilities.
install(DIRECTORY cmake DESTINATION share/${PROJECT_NAME})
install(PROGRAMS ${PROJECT_NAME}/${PROJECT_NAME} DESTINATION lib/${PROJECT_NAME})