/**
 * Templated default configuration
 */
const char* config_template = R"(
static_resources:
  listeners:
  - name: base_api_listener
    address:
      socket_address:
        protocol: TCP
        address: 0.0.0.0
        port_value: 10000
    api_listener:
      api_listener:
        "@type": type.googleapis.com/envoy.config.filter.network.http_connection_manager.v2.HttpConnectionManager
        stat_prefix: hcm
        route_config:
          name: api_router
          virtual_hosts:
            - name: api
              domains:
                - "*"
              routes:
                - match:
                    prefix: "/"
                  route:
                    cluster: dynamic_forward_proxy_cluster
        http_filters:
          - name: envoy.filters.http.dynamic_forward_proxy
            config:
              dns_cache_config:
                name: dynamic_forward_proxy_cache_config
                dns_lookup_family: V4_ONLY
          - name: envoy.router
            typed_config:
              "@type": type.googleapis.com/envoy.config.filter.http.router.v2.Router
  clusters:
  - name: base # Note: the direct API depends on the existence of a cluster with this name.
    connect_timeout: {{ connect_timeout_seconds }}s
    dns_refresh_rate: {{ dns_refresh_rate_seconds }}s
    http2_protocol_options: {}
    lb_policy: ROUND_ROBIN
    load_assignment:
      cluster_name: base
      endpoints: &base_endpoints
        - lb_endpoints:
            - endpoint:
                address:
                  socket_address: {address: {{ domain }}, port_value: 443}
    tls_context: &base_tls_context
      common_tls_context: &base_common_tls_context
        validation_context:
          trusted_ca:
            inline_string: |
)"
#include "certificates.inc"
                              R"(
          verify_subject_alt_name:
            - {{ domain }}
      sni: {{ domain }}
    type: LOGICAL_DNS
  - name: base_wlan # Note: the direct API depends on the existence of a cluster with this name.
    connect_timeout: {{ connect_timeout_seconds }}s
    dns_refresh_rate: {{ dns_refresh_rate_seconds }}s
    http2_protocol_options: {}
    lb_policy: ROUND_ROBIN
    load_assignment:
      cluster_name: base_wlan
      endpoints: *base_endpoints
    tls_context: *base_tls_context
    type: LOGICAL_DNS
  - name: base_wwan # Note: the direct API depends on the existence of a cluster with this name.
    connect_timeout: {{ connect_timeout_seconds }}s
    dns_refresh_rate: {{ dns_refresh_rate_seconds }}s
    http2_protocol_options: {}
    lb_policy: ROUND_ROBIN
    load_assignment:
      cluster_name: base_wwan
      endpoints: *base_endpoints
    tls_context: *base_tls_context
    type: LOGICAL_DNS
  - name: dynamic_forward_proxy_cluster
    connect_timeout: 1s
    lb_policy: CLUSTER_PROVIDED
    cluster_type:
      name: envoy.clusters.dynamic_forward_proxy
      typed_config:
        "@type": type.googleapis.com/envoy.config.cluster.dynamic_forward_proxy.v2alpha.ClusterConfig
        dns_cache_config:
          name: dynamic_forward_proxy_cache_config
          dns_lookup_family: V4_ONLY
    transport_socket:
      name: envoy.transport_sockets.tls
      typed_config:
        "@type": type.googleapis.com/envoy.api.v2.auth.UpstreamTlsContext
        common_tls_context: *base_common_tls_context
  - name: stats
    connect_timeout: {{ connect_timeout_seconds }}s
    dns_refresh_rate: {{ dns_refresh_rate_seconds }}s
    http2_protocol_options: {}
    lb_policy: ROUND_ROBIN
    load_assignment:
      cluster_name: stats
      endpoints:
        - lb_endpoints:
            - endpoint:
                address:
                  socket_address: {address: {{ stats_domain }}, port_value: 443}
    tls_context: *base_tls_context
    type: LOGICAL_DNS
stats_flush_interval: {{ stats_flush_interval_seconds }}s
stats_sinks:
  - name: envoy.metrics_service
    config:
      grpc_service:
        envoy_grpc:
          cluster_name: stats
stats_config:
  stats_matcher:
    inclusion_list:
      patterns:
        - safe_regex:
            google_re2: {}
            regex: 'cluster\.[\w]+?\.upstream_rq_total'
        - safe_regex:
            google_re2: {}
            regex: 'cluster\.[\w]+?\.upstream_cx_active'
        - safe_regex:
            google_re2: {}
            regex: 'cluster\.[\w]+?\.upstream_rq_time'
watchdog:
  megamiss_timeout: 60s
  miss_timeout: 60s
node:
  metadata:
    os: {{ device_os }}
)";
