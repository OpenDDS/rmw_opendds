if(DEFINED DDS_FOUND)
  return()
endif()

set(DDS_FOUND FALSE)

if(DEFINED ENV{DDS_ROOT})

  # Configure OpenDDS root variable.
  normalize_path(DDS_ROOT $ENV{DDS_ROOT})
  if(DDS_ROOT STREQUAL CMAKE_INSTALL_PREFIX)
    set(DDS_ROOT "$AMENT_CURRENT_PREFIX")
  endif()
  file(TO_NATIVE_PATH "${DDS_ROOT}" DDS_ROOT)
  message(STATUS "DDS_ROOT is: ${DDS_ROOT}")

  # Ensure existence of tao_idl compiler.
  if(NOT EXISTS "$ENV{TAO_ROOT}/TAO_IDL/tao_idl")
  	message(FATAL_ERROR "tao_idl compiler not found. Is $TAO_ROOT configured correctly?")
  else()
    set(OpenDDS_TaoIdlProcessor "$ENV{TAO_ROOT}/TAO_IDL/tao_idl")
  endif()

  # Ensure existence of opendds_idl compiler.
  if(NOT EXISTS "$ENV{DDS_ROOT}/bin/opendds_idl")
  	message(FATAL_ERROR "opendds_idl compiler not found. Is $DDS_ROOT configured correctly?")
  else()
    set(OpenDDS_OpenDdsIdlProcessor "$ENV{DDS_ROOT}/bin/opendds_idl")
  endif()

  # Set OpenDDS library directory.
  set(OpenDDS_HEADER_DIRS "${DDS_ROOT}/dds")
  set(OpenDDS_LIBRARY_DIRS "${DDS_ROOT}/lib")

  set(DDS_FOUND TRUE)
else()

	# DDS_ROOT MUST be set!
	message(FATAL_ERROR "DDS_ROOT must be set. Have you sourced $DDS_ROOT/setenv.sh?")
endif()