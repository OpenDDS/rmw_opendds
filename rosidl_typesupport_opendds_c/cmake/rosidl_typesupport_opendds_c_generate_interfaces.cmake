
# Default _target_suffix is the target that invoked this one;
# e.g., __rosidl_generator_c. We must change this to our package
# name to be compliant with the expectations of generators.
set(_target_suffix "__rosidl_typesupport_opendds_c")

message("Generating OpenDDS C Interfaces... [${PROJECT_NAME}]")
message("For: Target ${rosidl_generate_interfaces_TARGET} with suffix ${_target_suffix}")

foreach(_idl_file ${rosidl_generate_interfaces_IDL_FILES})
    message("    Generating IDL file: ${_idl_file}")
endforeach()

message("Generated OpenDDS C Interfaces!")