#!/bin/bash

# Ensure COLCON_PREFIX_PATH exists.
if [[ -z "$COLCON_PREFIX_PATH" ]]; then
	echo "COLCON_PREFIX_PATH must be set to the /install directory of a ROS-2 installation or overlay."1>&2
	exit 1
fi

# Prompt for confirmation and perform installation.
read -p "Installing the OpenDDS RMW will remove any existing OpenDDS files in $COLCON_PREFIX_PATH/../src/. Continue? [Y/N] " -n 1 -r
if [[ ! $REPLY =~ ^[Yy]$ ]]; then

	# Remove existing installation.
	echo "Removing OpenDDS RMW files from $COLCON_PREFIX_PATH/../src/"
	rm -rf $COLCON_PREFIX_PATH/../src/opendds_cmake_module
	rm -rf $COLCON_PREFIX_PATH/../src/rmw_opendds_cpp
	rm -rf $COLCON_PREFIX_PATH/../src/rosidl_typesupport_opendds_cpp

	# Copy new installation.
	echo "Installing OpenDDS RMW files to $COLCON_PREFIX_PATH/../src/"
	mv -r opendds_cmake_module $COLCON_PREFIX_PATH/../src/
	mv -r rmw_opendds_cpp $COLCON_PREFIX_PATH/../src/
	mv -r rosidl_typesupport_opendds_cpp $COLCON_PREFIX_PATH/../src/
fi
