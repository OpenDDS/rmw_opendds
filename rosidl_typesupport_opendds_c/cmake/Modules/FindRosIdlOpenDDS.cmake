# Load OpenDDS CMake module.
find_package(OpenDDS MODULE)

# Generate OMG-IDLs from ROS-2 IDLs.
rosidl_generate_dds_interfaces(
    ${rosidl_generate_interfaces_TARGET}__dds_opendds_idl
    IDL_FILES ${rosidl_generate_interfaces_IDL_FILES}
    DEPENDENCY_PACKAGE_NAMES ${rosidl_generate_interfaces_DEPENDENCY_PACKAGE_NAMES}
    OUTPUT_SUBFOLDERS "opendds_intermediate_omg_idls"
)

# Iterate over generated IDLs and build a list of them.
set(rosidl_idl_files "")
set(rosidl_idl_files_base_path "${CMAKE_CURRENT_BINARY_DIR}/rosidl_generator_dds_idl/${PROJECT_NAME}")
foreach(rosidl_idl_file ${rosidl_generate_interfaces_IDL_FILES})

    # Get filename and extension.
    get_filename_component(_extension "${_idl_file}" EXT)
    get_filename_component(_name "${_idl_file}" NAME_WE)

    # ROS .MSGs create 1 IDL.
    if (_extension STREQUAL ".msg")
        get_filename_component(_parent_folder "${_idl_file}" DIRECTORY)
        get_filename_component(_parent_folder "${_parent_folder}" NAME)
        list(APPEND rosidl_idl_files
             "${rosidl_idl_files_base_path}/${_parent_folder}/opendds_intermediate_omg_idls/${_name}_.idl")

    # ROS .SRVs create 2 IDLs.
    elseif(_extension STREQUAL ".srv")
        list(APPEND _dds_idl_files
          "${rosidl_idl_files_base_path}/srv/opendds_intermediate_omg_idls/Sample_${_name}_Request_.idl")
        list(APPEND _dds_idl_files
          "${rosidl_idl_files_base_path}/srv/opendds_intermediate_omg_idls/Sample_${_name}_Response_.idl")
    endif()
endforeach()

# Print info!
message(WARNING rosidl_idl_files)