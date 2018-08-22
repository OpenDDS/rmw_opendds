# OpenDDS ROS-2 Middleware
This project contains a work-in-progress implementation of the ROS-2 Middleware Interface (**RMW**) using [OpenDDS](https://github.com/objectcomputing/OpenDDS).

## Packages
This project contains several CMake/ROS-2 packages:

- `opendds_cmake_module`: Responsible for detecting the OpenDDS library and configuring paths to the Tao/OpenDDS IDL processors and headers.
- `rosidl_typesupport_opendds_c`: Responsible for generating OpenDDS C headers based on the OMG IDLs produced by ROS-2 preprocessors.
- `rosidl_typesupport_opendds_cpp`: Responsible for generating OpenDDS C++ headers based on the OMG IDLs produced by ROS-2 preprocessors.
- `rmw_opendds`: Responsible for binding the ROS-2 `rmw.h` interface functions to the equivalent OpenDDS functions.

These packages are built using ROS-2's `Colcon` and `Ament` build tools. `rosidl_typesupport_c` and `rosidl_typesupport_cpp` are almost identical, but must be provided as separate packages in order to satisfy the build-time requirements of the ROS-2 demo projects.

## Usage
All packages must be installed to the `src/ros2/` folder within the ROS-2 workspace that you want to compile with support for OpenDDS.  We have provided an `install.bash` script which will automatically install and/or update your *current* ROS-2 workspace with the packages in this repository. This script should only be run after you have sourced the `local_setup.sh` from your current ROS-2 workspace.

Once the packages are installed, you can test how they are working by invoking `colcon build --packages-up-to demo_nodes_cpp --parallel-workers 1` within the ROS-2 workspace folder. This will build one of the simpler ROS-2 demo projects, causing all of the packages in this RMW to be built. The `--parallel-workers 1` flag is used to make debugging parallel execution of CMake targets easier.

## Project Status
1. `rosidl_typesupport_opendds_cpp` is largely incorrect or unimplemented. It should be re-written based on the `rosidl_typesupport_opendds_c` package once that package is complete.

2. `rmw_opendds_cpp` contains buildable stubs of the implementation of `rmw.h`. They must be filled in with the appropriate calls to the underlying OpenDDS functions.

3. `opendds_cmake_module` contains a Python file with no extension (`opendds_cmake_module/opendds_cmake_module`) which contains a method `openDdsPreprocessIdls`. This method must be tested against different ROS-2-produced IDLs to ensure it correctly:
    - Inserts `#pragma` statements for the fields in those IDLs.
    - Inserts `typedef` statements for the fields which would be anonymous in those IDLs.