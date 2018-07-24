# OpenDDS ROS-2 Middleware
This project contains a work-in-progress implementation of the ROS-2 Middleware Interface (**RMW**) using [OpenDDS](https://github.com/objectcomputing/OpenDDS).

## Usage
This project contains three ROS-2 packages (and subsequently 3 CMake packages):

- `opendds_cmake_module`
- `rosidl_typesupport_opendds_cpp`
- `rmw_opendds`

All three of these must be installed to the `src/ros2/` folder within the ROS-2 workspace that you want to compile with support for OpenDDS. 

We have provided an `install.bash` script which will automatically install and/or update your *current* ROS-2 workspace with the packages in this repository. This script should only be run after you have sourced the `local_setup.sh` from your current ROS-2 workspace.