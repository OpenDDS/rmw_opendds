cmake_minimum_required(VERSION 3.5)

project(opendds_cmake_module NONE)

find_package(ament_cmake REQUIRED)

list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")

if(DEFINED ENV{DDS_ROOT})

  # Configure OpenDDS root variable.
  normalize_path(DDS_ROOT $ENV{DDS_ROOT})
  if(DDS_ROOT STREQUAL CMAKE_INSTALL_PREFIX)
    set(DDS_ROOT "$AMENT_CURRENT_PREFIX")
  endif()
  file(TO_NATIVE_PATH "${DDS_ROOT}" DDS_ROOT)
  message("DDS_ROOT is: ${DDS_ROOT}")

  # Ensure existence of tao_idl compiler.
  if(NOT EXISTS "$ENV{TAO_ROOT}/TAO_IDL/tao_idl")
  	message(FATAL_ERROR "tao_idl compiler not found. Is $TAO_ROOT configured correctly?")
  endif()

  # Ensure existence of opendds_idl compiler.
  if(NOT EXISTS "$ENV{DDS_ROOT}/bin/opendds_idl")
  	message(FATAL_ERROR "opendds_idl compiler not found. Is $DDS_ROOT configured correctly?")
  endif()
else()

	# DDS_ROOT MUST be set!
	message(FATAL_ERROR "DDS_ROOT must be set. Have you sourced $DDS_ROOT/setenv.sh?")
endif()

# Configure Ament testing.
if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

# Set installation information.
install(DIRECTORY cmake DESTINATION share/${PROJECT_NAME})
install(DIRECTORY config DESTINATION share/${PROJECT_NAME})
