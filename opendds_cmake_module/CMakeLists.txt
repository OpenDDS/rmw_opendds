cmake_minimum_required(VERSION 3.5)

project(opendds_cmake_module NONE)

find_package(ament_cmake REQUIRED)

list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")
find_package(OpenDDS MODULE)
if(OpenDDS_HOME)
  normalize_path(OpenDDS_HOME "${OpenDDS_HOME}")
  # if the home is in the install space make the environment hook relocatable
  if(OpenDDS_HOME STREQUAL CMAKE_INSTALL_PREFIX)
    set(OpenDDS_HOME "$AMENT_CURRENT_PREFIX")
  endif()
  file(TO_NATIVE_PATH "${OpenDDS_HOME}" OpenDDS_HOME)
endif()

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package(
  CONFIG_EXTRAS "opendds_cmake_module-extras.cmake"
)

install(DIRECTORY cmake
  DESTINATION share/${PROJECT_NAME})

install(DIRECTORY config
  DESTINATION share/${PROJECT_NAME})
