find_package(ament_cmake REQUIRED)
find_package(opendds_cmake_module REQUIRED)
find_package(OpenDDS MODULE)
list(INSERT CMAKE_MODULE_PATH 0 "${rosidl_typesupport_opendds_cpp_DIR}/Modules")
