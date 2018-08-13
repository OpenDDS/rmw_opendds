# Name of this generator.
set(_rosidl_typesupport_name "rosidl_typesupport_opendds_c")

# Default _target_suffix is the target that invoked this one;
# e.g., __rosidl_generator_c. We must change this to our package
# name to be compliant with the expectations of generators.
set(_target_suffix "__${_rosidl_typesupport_name}")

# Path where generated interfaces and intermediaries will be stored.
set(_output_path "${CMAKE_CURRENT_BINARY_DIR}/${_rosidl_typesupport_name}/${PROJECT_NAME}")

# Print candidate MSG and SRV files to process into IDLs.
message("Generating OpenDDS C interfaces for ${rosidl_generate_interfaces_TARGET} in ${_output_path}.")

# Generate OMG IDLs from MSG and SRV files.
message("    Converting ROS-2 MSG/SRV files to OMG IDLs.")
rosidl_generate_dds_interfaces(
  "${rosidl_generate_interfaces_TARGET}${_target_suffix}__idls"
  IDL_FILES ${rosidl_generate_interfaces_IDL_FILES}
  DEPENDENCY_PACKAGE_NAMES ${target_dependencies}
)

# Print output location of OMG IDLs.
# get_filename_component(_msg_destination "${_msg_destination}" ABSOLUTE)
# get_filename_component(_srv_destination "${_srv_destination}" ABSOLUTE)
# message("    Installed OMG IDLs for msg files [${_generated_msg_files}] to ${_msg_destination}")
# message("    Installed OMG IDLs for srv files [${_generated_srv_files}] to ${_srv_destination}")

# Create a list of all the header and source files we expect the Tao and OpenDDS IDL processors to produce.
message("    Generating list of expected IDL processor artifacts.")
unset(OpenDDS_idlArtifacts)
foreach(file ${_generated_msg_files})

    # TODO: Prepend OpenDDS #pragma statements to each file.

    # Add artifacts for file to list.
    get_filename_component(file "${file}" NAME_WE)
    list(APPEND OpenDDS_idlArtifacts
        "${file}C.cpp"
        "${file}C.h"
        "${file}C.inl"
        "${file}S.cpp"
        "${file}S.h"
        "${file}TypeSupport.idl"
        "${file}TypeSupportImpl.cpp"
        "${file}TypeSupportImpl.h"
    )
endforeach()

# Process OMG IDLs.
# TODO Need to set appropriate working dir dynamically. Duh!
message("    Processing OMG IDLs via Tao and OpenDDS.")
add_custom_command(
    OUTPUT ${OpenDDS_idlArtifacts}
    WORKING_DIRECTORY /home/crahda/ros2_ws/build/rosidl_generator_py/rosidl_generator_dds_idl
    COMMAND ${OpenDDS_TaoIdlProcessor} ${_generated_msg_files} ${_generated_srv_files}
    COMMAND ${OpenDDS_OpenDdsIdlProcessor} ${_generated_msg_files} ${_generated_srv_files}
)

# Install OMG IDL target.
message("    Adding IDL processor artifacts to CXX target ${rosidl_generate_interfaces_TARGET}${_target_suffix}.")
add_library(${rosidl_generate_interfaces_TARGET}${_target_suffix} SHARED ${OpenDDS_idlArtifacts})
set_target_properties(${rosidl_generate_interfaces_TARGET}${_target_suffix} PROPERTIES LINKER_LANGUAGE CXX)

# Add OMG IDL target dependencies.
add_dependencies(
  ${rosidl_generate_interfaces_TARGET}
  ${rosidl_generate_interfaces_TARGET}${_target_suffix}
)
add_dependencies(
  ${rosidl_generate_interfaces_TARGET}${_target_suffix}
  ${rosidl_generate_interfaces_TARGET}__cpp
)

# Install OMG IDL target if enabled.
if(NOT rosidl_generate_interfaces_SKIP_INSTALL)
  install(
    TARGETS ${rosidl_generate_interfaces_TARGET}${_target_suffix}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
  )

  rosidl_export_typesupport_libraries(
    ${_target_suffix}
    ${rosidl_generate_interfaces_TARGET}${_target_suffix}
  )
endif()