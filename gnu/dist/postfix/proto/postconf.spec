%CLASS install
%PARAM command_directory
%PARAM config_directory
%PARAM daemon_directory
%PARAM default_database_type
%PARAM mail_owner
%PARAM mail_spool_directory
%PARAM mailq_path
%PARAM manpage_directory
%PARAM newaliases_path
%PARAM process_id_directory
%PARAM queue_directory
%PARAM readme_directory
%PARAM sample_directory
%PARAM sendmail_path
%PARAM setgid_group

%CLASS postfix
%PARAM mail_release_date
%PARAM mail_version

%CLASS plumbing
%PARAM address_verify_service_name
%PARAM bounce_service_name
%PARAM cleanup_service_name
%PARAM defer_service_name
%PARAM error_service_name
%PARAM flush_service_name
%PARAM pickup_service_name
%PARAM queue_service_name
%PARAM rewrite_service_name
%PARAM showq_service_name
%PARAM trace_service_name

%CLASS security
%PARAM allow_min_user
%PARAM alternate_config_directories
%PARAM default_privs
%PARAM import_environment
%PARAM proxy_read_maps

%CLASS local-security
%PARAM allow_mail_to_commands
%PARAM allow_mail_to_files
%PARAM command_expansion_filter
%PARAM local_command_shell

%CLASS address-verification
%PARAM address_verify_sender
%PARAM address_verify_service_name

%CLASS address-verification-caching
%PARAM address_verify_map
%PARAM address_verify_negative_cache
%PARAM address_verify_negative_expire_time
%PARAM address_verify_negative_refresh_time
%PARAM address_verify_positive_expire_time
%PARAM address_verify_positive_refresh_time

%CLASS address-verification-routing
%PARAM address_verify_default_transport
%PARAM address_verify_local_transport
%PARAM address_verify_relay_transport
%PARAM address_verify_relayhost
%PARAM address_verify_transport_maps
%PARAM address_verify_virtual_transport

%CLASS smtpd-address-verification
%PARAM address_verify_poll_count
%PARAM address_verify_poll_delay
%PARAM unverified_recipient_reject_code
%PARAM unverified_sender_reject_code

%class compatibility
%PARAM undisclosed_recipients_header
%PARAM allow_min_user
%PARAM backwards_bounce_logfile_compatibility

%CLASS local-compatibility
%PARAM sun_mailtool_compatibility
%PARAM allow_mail_to_commands
%PARAM allow_mail_to_files
%PARAM biff

%CLASS smtpd-compatibility
%PARAM broken_sasl_auth_clients
%PARAM disable_vrfy_command
%PARAM smtpd_helo_required
%PARAM smtpd_noop_commands
%PARAM smtpd_sasl_exceptions_networks
%PARAM strict_rfc821_envelopes

%CLASS smtp-compatibility
%PARAM ignore_mx_lookup_error
%PARAM smtp_always_send_ehlo
%PARAM smtp_defer_if_no_mx_address_found
%PARAM smtp_host_lookup
%PARAM smtp_line_length_limit
%PARAM smtp_never_send_ehlo
%PARAM smtp_pix_workaround_delay_time
%PARAM smtp_pix_workaround_threshold_time
%PARAM smtp_quote_rfc821_envelope
%PARAM smtp_skip_4xx_greeting
%PARAM smtp_skip_5xx_greeting
%PARAM smtp_skip_quit_response

%CLASS lmtp-compatibility
%PARAM lmtp_skip_quit_response

%CLASS mime-compatibility
%PARAM strict_8bitmime
%PARAM strict_8bitmime_body
%PARAM strict_mime_encoding_domain
%PARAM strict_7bit_headers

%CLASS resource-control
%PARAM application_event_drain_time
%PARAM berkeley_db_create_buffer_size
%PARAM berkeley_db_read_buffer_size
%PARAM bounce_size_limit
%PARAM command_time_limit
%PARAM daemon_timeout
%PARAM default_process_limit
%PARAM delay_warning_time
%PARAM deliver_lock_attempts
%PARAM deliver_lock_delay
%PARAM duplicate_filter_limit
%PARAM fork_attempts
%PARAM fork_delay
%PARAM header_address_token_limit
%PARAM header_size_limit
%PARAM hopcount_limit
%PARAM in_flow_delay
%PARAM ipc_idle
%PARAM ipc_timeout
%PARAM ipc_ttl
%PARAM line_length_limit
%PARAM max_idle
%PARAM max_use
%PARAM message_size_limit
%PARAM queue_file_attribute_count_limit
%PARAM service_throttle_time
%PARAM stale_lock_time
%PARAM transport_retry_time
%PARAM trigger_timeout

%CLASS smtpd-resource-control
%PARAM client_event_status_update_time
%PARAM client_rate_time_unit
%PARAM queue_minfree
%PARAM smtpd_client_connection_count_limit
%PARAM smtpd_client_connection_limit_exceptions
%PARAM smtpd_client_connection_rate_limit
%PARAM smtpd_history_flush_threshold
%PARAM smtpd_junk_command_limit
%PARAM smtpd_recipient_limit
%PARAM smtpd_timeout

%CLASS smtp-resource-control
%PARAM smtp_connect_timeout
%PARAM smtp_data_done_timeout
%PARAM smtp_data_init_timeout
%PARAM smtp_data_xfer_timeout
%PARAM smtp_destination_concurrency_limit
%PARAM smtp_destination_recipient_limit
%PARAM smtp_helo_timeout
%PARAM smtp_mail_timeout
%PARAM smtp_mx_address_limit
%PARAM smtp_mx_session_limit
%PARAM smtp_quit_timeout
%PARAM smtp_rcpt_timeout
%PARAM smtp_rset_timeout
%PARAM smtp_xforward_timeout

%CLASS lmtp-resource-control
%PARAM lmtp_cache_connection
%PARAM lmtp_connect_timeout
%PARAM lmtp_data_done_timeout
%PARAM lmtp_data_init_timeout
%PARAM lmtp_data_xfer_timeout
%PARAM lmtp_lhlo_timeout
%PARAM lmtp_mail_timeout
%PARAM lmtp_quit_timeout
%PARAM lmtp_rcpt_timeout
%PARAM lmtp_rset_timeout
%PARAM lmtp_xforward_timeout

%CLASS mime-resource-control
%PARAM mime_boundary_length_limit
%PARAM mime_nesting_limit

%CLASS local-resource-control
%PARAM local_destination_concurrency_limit
%PARAM local_destination_recipient_limit

%CLASS smtpd-tarpit
%PARAM smtpd_error_sleep_time
%PARAM smtpd_hard_error_limit
%PARAM smtpd_soft_error_limit

%CLASS content-filter
%PARAM lmtp_send_xforward_command
%PARAM receive_override_options
%PARAM smtp_send_xforward_command
%PARAM smtpd_authorized_xforward_hosts

%CLASS built-in-filter
%PARAM body_checks
%PARAM body_checks_size_limit
%PARAM header_checks
%PARAM mime_header_checks
%PARAM nested_header_checks

%CLASS after-queue-filter
%PARAM content_filter

%CLASS smtpd-proxy-filter
%PARAM smtpd_proxy_ehlo
%PARAM smtpd_proxy_filter
%PARAM smtpd_proxy_timeout

%CLASS smtp
%PARAM best_mx_transport
%PARAM disable_dns_lookups
%PARAM fallback_relay
%PARAM smtp_bind_address
%PARAM smtp_helo_name
%PARAM smtp_randomize_addresses

%CLASS basic-config
%PARAM alias_maps
%PARAM inet_interfaces
%PARAM mydestination
%PARAM mydomain
%PARAM myhostname
%PARAM mynetworks
%PARAM mynetworks_style
%PARAM myorigin
%PARAM proxy_interfaces

%CLASS smtpd-policy
%PARAM smtpd_policy_service_max_idle
%PARAM smtpd_policy_service_max_ttl
%PARAM smtpd_policy_service_timeout

%CLASS smtpd-access
%PARAM allow_untrusted_routing
%PARAM maps_rbl_domains
%PARAM parent_domain_matches_subdomains
%PARAM permit_mx_backup_networks
%PARAM smtpd_client_restrictions
%PARAM smtpd_data_restrictions
%PARAM smtpd_delay_reject
%PARAM smtpd_etrn_restrictions
%PARAM smtpd_expansion_filter
%PARAM smtpd_helo_restrictions
%PARAM smtpd_null_access_lookup_key
%PARAM smtpd_recipient_restrictions
%PARAM smtpd_reject_unlisted_recipient
%PARAM smtpd_reject_unlisted_sender
%PARAM smtpd_restriction_classes
%PARAM smtpd_sender_restrictions

%CLASS smtpd-reply-code
%PARAM access_map_reject_code
%PARAM default_rbl_reply
%PARAM defer_code
%PARAM invalid_hostname_reject_code
%PARAM maps_rbl_reject_code
%PARAM multi_recipient_bounce_reject_code
%PARAM non_fqdn_reject_code
%PARAM rbl_reply_maps
%PARAM reject_code
%PARAM relay_domains_reject_code
%PARAM unknown_address_reject_code
%PARAM unknown_client_reject_code
%PARAM unknown_hostname_reject_code
%PARAM unknown_local_recipient_reject_code
%PARAM unknown_relay_recipient_reject_code
%PARAM unknown_virtual_alias_reject_code
%PARAM unknown_virtual_mailbox_reject_code
%PARAM unverified_recipient_reject_code
%PARAM unverified_sender_reject_code

%CLASS smtpd-sasl
%PARAM smtpd_sasl_application_name
%PARAM smtpd_sasl_auth_enable
%PARAM smtpd_sasl_local_domain
%PARAM smtpd_sasl_security_options
%PARAM smtpd_sender_login_maps

%CLASS smtp-sasl
%PARAM smtp_sasl_auth_enable
%PARAM smtp_sasl_password_maps
%PARAM smtp_sasl_security_options

%CLASS lmtp-sasl
%PARAM lmtp_sasl_auth_enable
%PARAM lmtp_sasl_password_maps
%PARAM lmtp_sasl_security_options

%CLASS smtpd-unknown-recipients
%PARAM local_recipient_maps
%PARAM relay_recipient_maps
%PARAM virtual_alias_maps
%PARAM virtual_mailbox_maps

%CLASS trouble-shooting
%PARAM 2bounce_notice_recipient
%PARAM bounce_notice_recipient
%PARAM debug_peer_level
%PARAM debug_peer_list
%PARAM debugger_command
%PARAM delay_notice_recipient
%PARAM dont_remove
%PARAM double_bounce_sender
%PARAM error_notice_recipient
%PARAM fault_injection_code
%PARAM helpful_warnings
%PARAM notify_classes
%PARAM show_user_unknown_table_name
%PARAM smtpd_authorized_xclient_hosts
%PARAM soft_bounce

%CLASS mime
%PARAM disable_mime_input_processing
%PARAM disable_mime_output_conversion

%CLASS verp
%PARAM default_verp_delimiters
%PARAM disable_verp_bounces
%PARAM smtpd_authorized_verp_clients
%PARAM verp_delimiter_filter

%CLASS lmtp
%PARAM lmtp_tcp_port

%CLASS other
%PARAM command_directory
%PARAM process_name
%PARAM process_id
%PARAM smtpd_banner
%PARAM mail_name

%CLASS scheduler
%PARAM bounce_queue_lifetime
%PARAM default_delivery_slot_cost
%PARAM default_delivery_slot_discount
%PARAM default_delivery_slot_loan
%PARAM default_destination_concurrency_limit
%PARAM default_destination_recipient_limit
%PARAM default_extra_recipient_limit
%PARAM default_minimum_delivery_slots
%PARAM default_recipient_limit
%PARAM defer_transports
%PARAM initial_destination_concurrency
%PARAM maximal_backoff_time
%PARAM maximal_queue_lifetime
%PARAM minimal_backoff_time
%PARAM qmgr_clog_warn_time
%PARAM qmgr_fudge_factor
%PARAM qmgr_message_active_limit
%PARAM qmgr_message_recipient_limit
%PARAM qmgr_message_recipient_minimum
%PARAM queue_run_delay
%PARAM queue_service_name

%CLASS qmqpd
%PARAM qmqpd_authorized_clients
%PARAM qmqpd_error_delay
%PARAM qmqpd_timeout

%CLASS logging
%PARAM syslog_facility
%PARAM syslog_name
%PARAM debug_peer_list
%PARAM debug_peer_level

%CLASS etrn
%PARAM fast_flush_domains
%PARAM fast_flush_purge_time
%PARAM fast_flush_refresh_time
%PARAM flush_service_name

%CLASS local
%PARAM alias_database
%PARAM alias_maps
%PARAM export_environment
%PARAM fallback_transport
%PARAM forward_expansion_filter
%PARAM forward_path
%PARAM home_mailbox
%PARAM local_transport
%PARAM luser_relay
%PARAM mailbox_command
%PARAM mailbox_command_maps
%PARAM mailbox_delivery_lock
%PARAM mailbox_size_limit
%PARAM mailbox_transport
%PARAM prepend_delivered_header
%PARAM require_home_directory

%CLASS address-manipulation
%PARAM allow_percent_hack
%PARAM always_bcc
%PARAM append_at_myorigin
%PARAM append_dot_mydomain
%PARAM canonical_maps
%PARAM cleanup_service_name
%PARAM default_transport
%PARAM default_transport
%PARAM empty_address_recipient
%PARAM enable_original_recipient
%PARAM expand_owner_alias
%PARAM masquerade_classes
%PARAM masquerade_domains
%PARAM masquerade_exceptions
%PARAM owner_request_special
%PARAM propagate_unmatched_extensions
%PARAM recipient_bcc_maps
%PARAM recipient_canonical_maps
%PARAM recipient_delimiter
%PARAM relay_domains
%PARAM relay_transport
%PARAM relayhost
%PARAM relocated_maps
%PARAM resolve_dequoted_address
%PARAM rewrite_service_name
%PARAM sender_based_routing
%PARAM sender_bcc_maps
%PARAM sender_canonical_maps
%PARAM swap_bangpath
%PARAM transport_maps
%PARAM virtual_alias_expansion_limit
%PARAM virtual_alias_maps
%PARAM virtual_alias_recursion_limit

%CLASS queue-hashing
%PARAM hash_queue_depth
%PARAM hash_queue_names

%CLASS virtual-mailbox
%PARAM virtual_gid_maps
%PARAM virtual_mailbox_base
%PARAM virtual_mailbox_domains
%PARAM virtual_mailbox_limit
%PARAM virtual_mailbox_lock
%PARAM virtual_mailbox_maps
%PARAM virtual_minimum_uid
%PARAM virtual_transport
%PARAM virtual_uid_maps

%CLASS virtual-alias-domain
%PARAM virtual_alias_domains
%PARAM virtual_alias_expansion_limit
%PARAM virtual_alias_maps
%PARAM virtual_alias_recursion_limit
