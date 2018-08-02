find_package(ament_cmake REQUIRED)
find_package(opendds_cmake_module REQUIRED)
find_package(OpenDDS MODULE)
list(INSERT CMAKE_MODULE_PATH 0 "${rosidl_typesupport_opendds_c_DIR}/Modules")

ament_register_extension(
    "rosidl_generate_interfaces"
    "rosidl_typesupport_opendds_c"
    "rosidl_typesupport_opendds_c_generate_interfaces.cmake")