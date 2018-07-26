# OpenDDS ROS-2 Middleware
This project contains a work-in-progress implementation of the ROS-2 Middleware Interface (**RMW**) using [OpenDDS](https://github.com/objectcomputing/OpenDDS).

## Status
This project contains three ROS-2 packages (and subsequently 3 CMake packages):

- `opendds_cmake_module`: First draft complete; OpenDDS library loading/detection still needs to be performed.
- `rosidl_typesupport_opendds_cpp`: Draft in progress; ROS-2 IDL to `tao_idl` and `opendds_idl` formats must be done.
- `rmw_opendds`: Basic package.xml present. No implementation.

## Usage
All three packages must be installed to the `src/ros2/` folder within the ROS-2 workspace that you want to compile with support for OpenDDS. 

We have provided an `install.bash` script which will automatically install and/or update your *current* ROS-2 workspace with the packages in this repository. This script should only be run after you have sourced the `local_setup.sh` from your current ROS-2 workspace.