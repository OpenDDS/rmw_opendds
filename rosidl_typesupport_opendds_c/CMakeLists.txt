cmake_minimum_required(VERSION 3.5)

project(rosidl_typesupport_opendds_c)

# Load required CMake packages.
find_package(ament_cmake REQUIRED)
find_package(ament_cmake_python REQUIRED)
find_package(opendds_cmake_module REQUIRED)
find_package(OpenDDS MODULE)

# Declare RMW and ROS-IDL typesupport dependencies.
ament_export_dependencies(
    rmw
    rosidl_cmake
    rosidl_generator_c
    rosidl_generator_dds_idl
    rosidl_typesupport_interface
)

ament_index_register_resource("rosidl_typesupport_c")

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

# Install CMake utilities.
install(DIRECTORY cmake DESTINATION share/${PROJECT_NAME})
install(PROGRAMS ${PROJECT_NAME}/${PROJECT_NAME} DESTINATION lib/${PROJECT_NAME})

# Make this package available to other ROS-2 packages.
ament_python_install_package(${PROJECT_NAME})
ament_package(CONFIG_EXTRAS "${PROJECT_NAME}-extras.cmake")