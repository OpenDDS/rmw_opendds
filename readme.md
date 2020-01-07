# OpenDDS ROS2 Middleware
This project contains a work-in-progress implementation of the ROS2 Middleware Interface (**RMW**) using [OpenDDS](https://github.com/objectcomputing/OpenDDS).

## Packages
This project contains several ROS2 packages:

- `rmw_opendds`: composed of (this repo)
  - `rmw_opendds_cpp`: Responsible for binding the ROS2 `rmw.h` interface functions to the equivalent OpenDDS functions.
  - `rmw_opendds_shared_cpp`: Shared code used in rmw_opendds.
- `rosidl_typesupport_opendds`: composed of (https://github.com/oci-labs/rosidl_typesupport_opendds)
  - `opendds_cmake_module`: Responsible for configuring paths to the TAO/OpenDDS IDL processors and headers.
  - `rosidl_typesupport_opendds_c`: Responsible for generating OpenDDS C headers based on the OMG IDLs produced by ROS2 preprocessors.
  - `rosidl_typesupport_opendds_cpp`: Responsible for generating OpenDDS C++ headers based on the OMG IDLs produced by ROS2 preprocessors.

These packages are built using ROS2's `Colcon` and `Ament` build tools. `rosidl_typesupport_c` and `rosidl_typesupport_cpp` are almost identical, but must be provided as separate packages in order to satisfy the build-time requirements of the ROS2 demo projects.

## Usage
Add the two repos above to the `src/` folder within a ROS2 workspace that you want to compile with support for OpenDDS. You can pull the `objectcomputing/opendds_ros2` docker image as a build environment. A [volume mapped docker container](https://github.com/adamsj-oci/workspace_tools/blob/master/start_docker.sh) to the ROS2 workspace, can be used to build the sources from the following steps:

- `source /opt/OpenDDS/setenv.sh`
- `source /opt/ros/eloquent/setup.bash`
- Verify the environment.
  - `printenv|grep DDS`
  - `printenv|grep ROS`
- Build the code.
  - `colcon build --symlink-install` 

## Project Status
1. `rosidl_typesupport_opendds_c` is partially implemented. Currently, it correctly converts any MSG/SRV files to OMG IDLs, detects the OpenDDS and TAO libraries/preprocessors, and attemps to process the IDLs using those preprocessors. The next step is to integrate the Python script from the `opendds_cmake_module` (which is not written/implemented) into this module's root CMake file so that it runs *before* the OpenDDS and TAO processors. Once that is done, all of the expected C/C++ output files from OpenDDS/TAO must be added to a library that ROS2 can detect (which should happen automatically).

2. `rosidl_typesupport_opendds_cpp` is largely incorrect or unimplemented. It should be re-written based on the `rosidl_typesupport_opendds_c` package once that package is complete.

3. `rmw_opendds_cpp` contains buildable stubs of the implementation of `rmw.h`. They must be filled in with the appropriate calls to the underlying OpenDDS functions.

4. `opendds_cmake_module` contains a Python file with no extension (`opendds_cmake_module/opendds_cmake_module`) which contains a method `openDdsPreprocessIdls`. This method must be implemented and tested against different ROS2-produced IDLs to ensure it correctly:
    - Inserts `#pragma` statements for the fields in those IDLs.
    - Inserts `typedef` statements for the fields which would be anonymous in those IDLs.
