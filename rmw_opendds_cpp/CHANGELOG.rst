^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Changelog for package rmw_opendds_cpp
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

0.1.0 (2021-03-15)
------------------
* non-xytpes interop
* support new option in dds_marshal
* reduce payload size to match other ROS2 DDS
* Adding support for rmw_get_subscriptions_info_by_topic and rmw_get_publishers_info_by_topic
* Added code to support rmw_take_event
* Added support for rmw_get_node_names_with_enclaves
* Added support for 'event' functions
* Fix for type name in messages
* Added deallocator parameter to create_requester / replier functions
* rmw_subscription_count_matched_publishers
* used default allocator
* rmw_destroy_subscription and take memcpy
* implemented rmw_client
* rmw service
* qos changes
* rmw_subscription_get_actual_qos
* Making use of OpenDDS cmake support
* pulled other missing runtime dependencies from rmw_connext
* use correct _narrow method
* IDL file generation
* Starter implementation by reusing rmw_connext_cpp
* Add a C type support as well; it is required for full RMW build.
* Contributors: Adam Mitz, Brandon Sanders, Jeremy Adams, Jiang Li, Johnny Willemsen, Mike Kuznetsov
