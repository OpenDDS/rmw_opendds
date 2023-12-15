
#include <rcutils/allocator.h>
#include <rmw/ret_types.h>
#include <rmw/types.h>
#include <rmw/network_flow_endpoint_array.h>
#include <rmw/get_network_flow_endpoints.h>
#include <rmw/error_handling.h>

rmw_ret_t
rmw_publisher_get_network_flow_endpoints (
    const rmw_publisher_t * publisher,
    rcutils_allocator_t * allocator,
    rmw_network_flow_endpoint_array_t * network_flow_endpoints)
{
    (void) publisher;
    (void) allocator;
    (void) network_flow_endpoints;
    // Unused in current implementation.
    RMW_SET_ERROR_MSG("unimplemented");
    return RMW_RET_ERROR;
}

rmw_ret_t
rmw_subscription_get_network_flow_endpoints (
    const rmw_subscription_t * publisher,
    rcutils_allocator_t * allocator,
    rmw_network_flow_endpoint_array_t * network_flow_endpoints)
{
    (void) publisher;
    (void) allocator;
    (void) network_flow_endpoints;
    // Unused in current implementation.
    RMW_SET_ERROR_MSG("unimplemented");
    return RMW_RET_ERROR;
}
