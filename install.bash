#!/bin/bash

# Ensure COLCON_PREFIX_PATH exists.
if [[ -z "$COLCON_PREFIX_PATH" ]]; then
	echo "COLCON_PREFIX_PATH must be set to the /install directory of a ROS-2 installation or overlay." 1>&2
	echo "Have you sourced /path/to/ros2_ws/install/local_setup.sh?" 1>&2
	exit
fi

# Prompt for confirmation and perform installation.
read -p "This will remove any OpenDDS files in $COLCON_PREFIX_PATH/../src/ros2/. Continue? [Y/N] " -n 1 -r
if [[ $REPLY =~ ^[Yy]$ ]]; then
	echo

	# Remove existing installation.
	echo "Removing OpenDDS RMW files from $COLCON_PREFIX_PATH/../src/ros2/"
	rm -rf $COLCON_PREFIX_PATH/../src/ros2/opendds_cmake_module
	rm -rf $COLCON_PREFIX_PATH/../src/ros2/rmw_opendds_cpp
	rm -rf $COLCON_PREFIX_PATH/../src/ros2/rosidl_typesupport_opendds_c
	rm -rf $COLCON_PREFIX_PATH/../src/ros2/rosidl_typesupport_opendds_cpp

	# Copy new installation.
	echo "Installing OpenDDS RMW files to $COLCON_PREFIX_PATH/../src/ros2/"
	cp -r opendds_cmake_module $COLCON_PREFIX_PATH/../src/ros2/
	cp -r rmw_opendds_cpp $COLCON_PREFIX_PATH/../src/ros2/
	cp -r rosidl_typesupport_opendds_c $COLCON_PREFIX_PATH/../src/ros2/
	cp -r rosidl_typesupport_opendds_cpp $COLCON_PREFIX_PATH/../src/ros2/
fi
