#include "rmw/rmw.h"
#include "rmw/macros.h"
#include "rmw/qos_profiles.h"
#include "rmw/types.h"
#include "rmw/visibility_control.h"

#ifndef DEFAULT_RMW_RETURN
#define DEFAULT_RMW_RETURN -1
#endif

// The extern "C" here enforces that overloading is not used.
extern "C"
{

const char * rmw_get_implementation_identifier(void) {
    return "OpenDDS";
}

rmw_ret_t rmw_init(void) {
    // TODO:
    return DEFAULT_RMW_RETURN;
};

rmw_node_t * rmw_create_node(const char * name,
                             const char * namespace_,
                             size_t domain_id,
                             const rmw_node_security_options_t * security_options) {
    // TODO:
    return nullptr;
}

rmw_ret_t rmw_destroy_node(rmw_node_t * node) {
    // TODO:
    return DEFAULT_RMW_RETURN;
}

const rmw_guard_condition_t * rmw_node_get_graph_guard_condition(const rmw_node_t * node) {
    // TODO:
    return nullptr;
}

rmw_publisher_t * rmw_create_publisher(const rmw_node_t * node,
                                       const rosidl_message_type_support_t * type_support,
                                       const char * topic_name,
                                       const rmw_qos_profile_t * qos_policies) {
    // TODO:
    return nullptr;
}

rmw_ret_t rmw_destroy_publisher(rmw_node_t * node, rmw_publisher_t * publisher) {
    // TODO:
    return DEFAULT_RMW_RETURN;
}

rmw_ret_t rmw_publish(const rmw_publisher_t * publisher, const void * ros_message) {
    // TODO:
    return DEFAULT_RMW_RETURN;
}

rmw_ret_t rmw_publish_serialized_message(const rmw_publisher_t * publisher,
                                         const rmw_serialized_message_t * serialized_message) {
    // TODO:
    return DEFAULT_RMW_RETURN;
}

rmw_ret_t rmw_serialize(const void * ros_message,
                        const rosidl_message_type_support_t * type_support,
                        rmw_serialized_message_t * serialized_message) {
    // TODO:
    return DEFAULT_RMW_RETURN;
}

rmw_ret_t rmw_deserialize(const rmw_serialized_message_t * serialized_message,
                          const rosidl_message_type_support_t * type_support,
                          void * ros_message) {
    // TODO:
    return DEFAULT_RMW_RETURN;
}

rmw_subscription_t * rmw_create_subscription(const rmw_node_t * node,
                                             const rosidl_message_type_support_t * type_support,
                                             const char * topic_name,
                                             const rmw_qos_profile_t * qos_policies,
                                             bool ignore_local_publications) {
    // TODO:
    return nullptr;
}

rmw_ret_t rmw_destroy_subscription(rmw_node_t * node, rmw_subscription_t * subscription) {
    // TODO:
    return DEFAULT_RMW_RETURN;
}

rmw_ret_t rmw_take(const rmw_subscription_t * subscription, void * ros_message, bool * taken) {
    // TODO:
    return DEFAULT_RMW_RETURN;
}

rmw_ret_t rmw_take_with_info(const rmw_subscription_t * subscription,
                             void * ros_message,
                             bool * taken,
                             rmw_message_info_t * message_info) {
     // TODO:
     return DEFAULT_RMW_RETURN;
}

rmw_ret_t rmw_take_serialized_message(const rmw_subscription_t * subscription,
                                      rmw_serialized_message_t * serialized_message,
                                      bool * taken) {
    // TODO:
    return DEFAULT_RMW_RETURN;
}

rmw_ret_t rmw_take_serialized_message_with_info(const rmw_subscription_t * subscription,
                                                rmw_serialized_message_t * serialized_message,
                                                bool * taken,
                                                rmw_message_info_t * message_info) {
    // TODO:
    return DEFAULT_RMW_RETURN;
}

rmw_client_t * rmw_create_client(const rmw_node_t * node,
                                 const rosidl_service_type_support_t * type_support,
                                 const char * service_name,
                                 const rmw_qos_profile_t * qos_policies) {
    // TODO:
    return nullptr;
}

rmw_ret_t rmw_destroy_client(rmw_node_t * node, rmw_client_t * client) {
    // TODO:
    return DEFAULT_RMW_RETURN;
}

rmw_ret_t rmw_send_request(const rmw_client_t * client,
                           const void * ros_request,
                           int64_t * sequence_id) {
    // TODO:
    return DEFAULT_RMW_RETURN;
}

rmw_ret_t rmw_take_response(const rmw_client_t * client,
                            rmw_request_id_t * request_header,
                            void * ros_response,
                            bool * taken) {
    // TODO:
    return DEFAULT_RMW_RETURN;
}

rmw_service_t * rmw_create_service(const rmw_node_t * node,
                                   const rosidl_service_type_support_t * type_support,
                                   const char * service_name,
                                   const rmw_qos_profile_t * qos_policies) {
    // TODO:
    return nullptr;
}

rmw_ret_t rmw_destroy_service(rmw_node_t * node, rmw_service_t * service) {
    // TODO:
    return DEFAULT_RMW_RETURN;
}

rmw_ret_t rmw_take_request(const rmw_service_t * service,
                           rmw_request_id_t * request_header,
                           void * ros_request,
                           bool * taken) {
    // TODO:
    return DEFAULT_RMW_RETURN;
}

rmw_ret_t rmw_send_response(const rmw_service_t * service,
                            rmw_request_id_t * request_header,
                            void * ros_response) {
    // TODO:
    return DEFAULT_RMW_RETURN;
}

rmw_guard_condition_t * rmw_create_guard_condition(void) {
    // TODO:
    return nullptr;
}

rmw_ret_t rmw_destroy_guard_condition(rmw_guard_condition_t * guard_condition) {
    // TODO:
    return DEFAULT_RMW_RETURN;
}

rmw_ret_t rmw_trigger_guard_condition(const rmw_guard_condition_t * guard_condition) {
    // TODO:
    return DEFAULT_RMW_RETURN;
}

rmw_wait_set_t * rmw_create_wait_set(size_t max_conditions) {
    // TODO:
    return nullptr;
}

rmw_ret_t rmw_destroy_wait_set(rmw_wait_set_t * wait_set) {
    // TODO:
    return DEFAULT_RMW_RETURN;
}

rmw_ret_t rmw_wait(rmw_subscriptions_t * subscriptions,
                   rmw_guard_conditions_t * guard_conditions,
                   rmw_services_t * services,
                   rmw_clients_t * clients,
                   rmw_wait_set_t * wait_set,
                   const rmw_time_t * wait_timeout) {
    // TODO:
    return DEFAULT_RMW_RETURN;
}

rmw_ret_t rmw_get_node_names(const rmw_node_t * node,
                             rcutils_string_array_t * node_names) {
    // TODO:
    return DEFAULT_RMW_RETURN;
}

rmw_ret_t rmw_count_publishers(const rmw_node_t * node,
                               const char * topic_name,
                               size_t * count) {
    // TODO:
    return DEFAULT_RMW_RETURN;
}

rmw_ret_t rmw_count_subscribers(const rmw_node_t * node,
                                const char * topic_name,
                                size_t * count) {
    // TODO:
    return DEFAULT_RMW_RETURN;
}

rmw_ret_t rmw_get_gid_for_publisher(const rmw_publisher_t * publisher, rmw_gid_t * gid) {
    // TODO:
    return DEFAULT_RMW_RETURN;
}

rmw_ret_t rmw_compare_gids_equal(const rmw_gid_t * gid1, const rmw_gid_t * gid2, bool * result) {
    // TODO:
    return DEFAULT_RMW_RETURN;
}

rmw_ret_t rmw_service_server_is_available(const rmw_node_t * node,
                                          const rmw_client_t * client,
                                          bool * is_available) {
    // TODO:
    return DEFAULT_RMW_RETURN;
}

rmw_ret_t rmw_set_log_severity(rmw_log_severity_t severity) {
    // TODO:
    return DEFAULT_RMW_RETURN;
}

}