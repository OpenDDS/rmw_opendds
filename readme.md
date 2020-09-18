
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

These packages are built using ROS2's `Colcon` build tool. `rosidl_typesupport_c` and `rosidl_typesupport_cpp` are almost identical, but must be provided as separate packages in order to satisfy the build-time requirements of the ROS2 demo projects.

## Usage
See https://github.com/oci-labs/rmw_build/blob/master/README.md for details on how to build and run this repo.
