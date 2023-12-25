/*	$NetBSD: smtp.c,v 1.12.2.1 2023/12/25 12:43:35 martin Exp $	*/

/*++
/* NAME
/*	smtp 8
/* SUMMARY
/*	Postfix SMTP+LMTP client
/* SYNOPSIS
/*	\fBsmtp\fR [generic Postfix daemon options] [flags=DORX]
/* DESCRIPTION
/*	The Postfix SMTP+LMTP client implements the SMTP and LMTP mail
/*	delivery protocols. It processes message delivery requests from
/*	the queue manager. Each request specifies a queue file, a sender
/*	address, a domain or host to deliver to, and recipient information.
/*	This program expects to be run from the \fBmaster\fR(8) process
/*	manager.
/*
/*	The SMTP+LMTP client updates the queue file and marks recipients
/*	as finished, or it informs the queue manager that delivery should
/*	be tried again at a later time. Delivery status reports are sent
/*	to the \fBbounce\fR(8), \fBdefer\fR(8) or \fBtrace\fR(8) daemon as
/*	appropriate.
/*
/*	The SMTP+LMTP client looks up a list of mail exchanger addresses for
/*	the destination host, sorts the list by preference, and connects
/*	to each listed address until it finds a server that responds.
/*
/*	When a server is not reachable, or when mail delivery fails due
/*	to a recoverable error condition, the SMTP+LMTP client will try to
/*	deliver the mail to an alternate host.
/*
/*	After a successful mail transaction, a connection may be saved
/*	to the \fBscache\fR(8) connection cache server, so that it
/*	may be used by any SMTP+LMTP client for a subsequent transaction.
/*
/*	By default, connection caching is enabled temporarily for
/*	destinations that have a high volume of mail in the active
/*	queue. Connection caching can be enabled permanently for
/*	specific destinations.
/* SMTP DESTINATION SYNTAX
/* .ad
/* .fi
/*	The Postfix SMTP+LMTP client supports multiple destinations
/*	separated by comma or whitespace (Postfix 3.5 and later).
/*	SMTP destinations have the following form:
/* .IP \fIdomainname\fR
/* .IP \fIdomainname\fR:\fIport\fR
/*	Look up the mail exchangers for the specified domain, and
/*	connect to the specified port (default: \fBsmtp\fR).
/* .IP [\fIhostname\fR]
/* .IP [\fIhostname\fR]:\fIport\fR
/*	Look up the address(es) of the specified host, and connect to
/*	the specified port (default: \fBsmtp\fR).
/* .IP [\fIaddress\fR]
/* .IP [\fIaddress\fR]:\fIport\fR
/*	Connect to the host at the specified address, and connect
/*	to the specified port (default: \fBsmtp\fR). An IPv6 address
/*	must be formatted as [\fBipv6\fR:\fIaddress\fR].
/* LMTP DESTINATION SYNTAX
/* .ad
/* .fi
/*	The Postfix SMTP+LMTP client supports multiple destinations
/*	separated by comma or whitespace (Postfix 3.5 and later).
/*	LMTP destinations have the following form:
/* .IP \fBunix\fR:\fIpathname\fR
/*	Connect to the local UNIX-domain server that is bound to the specified
/*	\fIpathname\fR. If the process runs chrooted, an absolute pathname
/*	is interpreted relative to the Postfix queue directory.
/* .IP \fBinet\fR:\fIhostname\fR
/* .IP \fBinet\fR:\fIhostname\fR:\fIport\fR
/* .IP \fBinet\fR:[\fIaddress\fR]
/* .IP \fBinet\fR:[\fIaddress\fR]:\fIport\fR
/*	Connect to the specified TCP port on the specified local or
/*	remote host. If no port is specified, connect to the port defined as
/*	\fBlmtp\fR in \fBservices\fR(4).
/*	If no such service is found, the \fBlmtp_tcp_port\fR configuration
/*	parameter (default value of 24) will be used.
/*	An IPv6 address must be formatted as [\fBipv6\fR:\fIaddress\fR].
/* SINGLE-RECIPIENT DELIVERY
/* .ad
/* .fi
/*	By default, the Postfix SMTP+LMTP client delivers mail to
/*	multiple recipients per delivery request. This is undesirable
/*	when prepending a \fBDelivered-to:\fR or \fBX-Original-To:\fR
/*	message header. To prevent Postfix from sending multiple
/*	recipients per delivery request, specify
/* .sp
/* .nf
/*	    \fItransport\fB_destination_recipient_limit = 1\fR
/* .fi
/*
/*	in the Postfix \fBmain.cf\fR file, where \fItransport\fR
/*	is the name in the first column of the Postfix \fBmaster.cf\fR
/*	entry for this mail delivery service.
/* COMMAND ATTRIBUTE SYNTAX
/* .ad
/* .fi
/* .IP "\fBflags=DORX\fR (optional)"
/*	Optional message processing flags.
/* .RS
/* .IP \fBD\fR
/*	Prepend a "\fBDelivered-To: \fIrecipient\fR" message header
/*	with the envelope recipient address. Note: for this to work,
/*	the \fItransport\fB_destination_recipient_limit\fR must be
/*	1 (see SINGLE-RECIPIENT DELIVERY above for details).
/* .sp
/*	The \fBD\fR flag also enforces loop detection: if a message
/*	already contains a \fBDelivered-To:\fR header with the same
/*	recipient address, then the message is returned as
/*	undeliverable. The address comparison is case insensitive.
/* .sp
/*	This feature is available as of Postfix 3.5.
/* .IP \fBO\fR
/*	Prepend an "\fBX-Original-To: \fIrecipient\fR" message
/*	header with the recipient address as given to Postfix. Note:
/*	for this to work, the
/*	\fItransport\fB_destination_recipient_limit\fR must be 1
/*	(see SINGLE-RECIPIENT DELIVERY above for details).
/* .sp
/*	This feature is available as of Postfix 3.5.
/* .IP \fBR\fR
/*	Prepend a "\fBReturn-Path: <\fIsender\fB>\fR" message header
/*	with the envelope sender address.
/* .sp
/*	This feature is available as of Postfix 3.5.
/* .IP \fBX\fR
/*	Indicates that the delivery is final. This flag affects
/*	the status reported in "success" DSN (delivery status
/*	notification) messages, and changes it from "relayed" into
/*	"delivered".
/* .sp
/*	This feature is available as of Postfix 3.5.
/* .RE
/* SECURITY
/*	The SMTP+LMTP client is moderately security-sensitive. It
/*	talks to SMTP or LMTP servers and to DNS servers on the
/*	network. The SMTP+LMTP client can be run chrooted at fixed
/*	low privilege.
/* STANDARDS
/*	RFC 821 (SMTP protocol)
/*	RFC 822 (ARPA Internet Text Messages)
/*	RFC 1651 (SMTP service extensions)
/*	RFC 1652 (8bit-MIME transport)
/*	RFC 1870 (Message Size Declaration)
/*	RFC 2033 (LMTP protocol)
/*	RFC 2034 (SMTP Enhanced Error Codes)
/*	RFC 2045 (MIME: Format of Internet Message Bodies)
/*	RFC 2046 (MIME: Media Types)
/*	RFC 2554 (AUTH command)
/*	RFC 2821 (SMTP protocol)
/*	RFC 2782 (SRV resource records)
/*	RFC 2920 (SMTP Pipelining)
/*	RFC 3207 (STARTTLS command)
/*	RFC 3461 (SMTP DSN Extension)
/*	RFC 3463 (Enhanced Status Codes)
/*	RFC 4954 (AUTH command)
/*	RFC 5321 (SMTP protocol)
/*	RFC 6531 (Internationalized SMTP)
/*	RFC 6533 (Internationalized Delivery Status Notifications)
/*	RFC 7672 (SMTP security via opportunistic DANE TLS)
/* DIAGNOSTICS
/*	Problems and transactions are logged to \fBsyslogd\fR(8)
/*	or \fBpostlogd\fR(8).
/*	Corrupted message files are marked so that the queue manager can
/*	move them to the \fBcorrupt\fR queue for further inspection.
/*
/*	Depending on the setting of the \fBnotify_classes\fR parameter,
/*	the postmaster is notified of bounces, protocol problems, and of
/*	other trouble.
/* BUGS
/*	SMTP and LMTP connection reuse for TLS (without closing the
/*	SMTP or LMTP connection) is not supported before Postfix 3.4.
/*
/*	SMTP and LMTP connection reuse assumes that SASL credentials
/*	are valid for all destinations that map onto the same IP
/*	address and TCP port.
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	Before Postfix version 2.3, the LMTP client is a separate
/*	program that implements only a subset of the functionality
/*	available with SMTP: there is no support for TLS, and
/*	connections are cached in-process, making it ineffective
/*	when the client is used for multiple domains.
/*
/*	Most smtp_\fIxxx\fR configuration parameters have an
/*	lmtp_\fIxxx\fR "mirror" parameter for the equivalent LMTP
/*	feature. This document describes only those LMTP-related
/*	parameters that aren't simply "mirror" parameters.
/*
/*	Changes to \fBmain.cf\fR are picked up automatically, as \fBsmtp\fR(8)
/*	processes run for only a limited amount of time. Use the command
/*	"\fBpostfix reload\fR" to speed up a change.
/*
/*	The text below provides only a parameter summary. See
/*	\fBpostconf\fR(5) for more details including examples.
/* COMPATIBILITY CONTROLS
/* .ad
/* .fi
/* .IP "\fBignore_mx_lookup_error (no)\fR"
/*	Ignore DNS MX lookups that produce no response.
/* .IP "\fBsmtp_always_send_ehlo (yes)\fR"
/*	Always send EHLO at the start of an SMTP session.
/* .IP "\fBsmtp_never_send_ehlo (no)\fR"
/*	Never send EHLO at the start of an SMTP session.
/* .IP "\fBsmtp_defer_if_no_mx_address_found (no)\fR"
/*	Defer mail delivery when no MX record resolves to an IP address.
/* .IP "\fBsmtp_line_length_limit (998)\fR"
/*	The maximal length of message header and body lines that Postfix
/*	will send via SMTP.
/* .IP "\fBsmtp_pix_workaround_delay_time (10s)\fR"
/*	How long the Postfix SMTP client pauses before sending
/*	".<CR><LF>" in order to work around the PIX firewall
/*	"<CR><LF>.<CR><LF>" bug.
/* .IP "\fBsmtp_pix_workaround_threshold_time (500s)\fR"
/*	How long a message must be queued before the Postfix SMTP client
/*	turns on the PIX firewall "<CR><LF>.<CR><LF>"
/*	bug workaround for delivery through firewalls with "smtp fixup"
/*	mode turned on.
/* .IP "\fBsmtp_pix_workarounds (disable_esmtp, delay_dotcrlf)\fR"
/*	A list that specifies zero or more workarounds for CISCO PIX
/*	firewall bugs.
/* .IP "\fBsmtp_pix_workaround_maps (empty)\fR"
/*	Lookup tables, indexed by the remote SMTP server address, with
/*	per-destination workarounds for CISCO PIX firewall bugs.
/* .IP "\fBsmtp_quote_rfc821_envelope (yes)\fR"
/*	Quote addresses in Postfix SMTP client MAIL FROM and RCPT TO commands
/*	as required
/*	by RFC 5321.
/* .IP "\fBsmtp_reply_filter (empty)\fR"
/*	A mechanism to transform replies from remote SMTP servers one
/*	line at a time.
/* .IP "\fBsmtp_skip_5xx_greeting (yes)\fR"
/*	Skip remote SMTP servers that greet with a 5XX status code.
/* .IP "\fBsmtp_skip_quit_response (yes)\fR"
/*	Do not wait for the response to the SMTP QUIT command.
/* .PP
/*	Available in Postfix version 2.0 and earlier:
/* .IP "\fBsmtp_skip_4xx_greeting (yes)\fR"
/*	Skip SMTP servers that greet with a 4XX status code (go away, try
/*	again later).
/* .PP
/*	Available in Postfix version 2.2 and later:
/* .IP "\fBsmtp_discard_ehlo_keyword_address_maps (empty)\fR"
/*	Lookup tables, indexed by the remote SMTP server address, with
/*	case insensitive lists of EHLO keywords (pipelining, starttls, auth,
/*	etc.) that the Postfix SMTP client will ignore in the EHLO response from a
/*	remote SMTP server.
/* .IP "\fBsmtp_discard_ehlo_keywords (empty)\fR"
/*	A case insensitive list of EHLO keywords (pipelining, starttls,
/*	auth, etc.) that the Postfix SMTP client will ignore in the EHLO
/*	response from a remote SMTP server.
/* .IP "\fBsmtp_generic_maps (empty)\fR"
/*	Optional lookup tables that perform address rewriting in the
/*	Postfix SMTP client, typically to transform a locally valid address into
/*	a globally valid address when sending mail across the Internet.
/* .PP
/*	Available in Postfix version 2.2.9 and later:
/* .IP "\fBsmtp_cname_overrides_servername (version dependent)\fR"
/*	When the remote SMTP servername is a DNS CNAME, replace the
/*	servername with the result from CNAME expansion for the purpose of
/*	logging, SASL password lookup, TLS
/*	policy decisions, or TLS certificate verification.
/* .PP
/*	Available in Postfix version 2.3 and later:
/* .IP "\fBlmtp_discard_lhlo_keyword_address_maps (empty)\fR"
/*	Lookup tables, indexed by the remote LMTP server address, with
/*	case insensitive lists of LHLO keywords (pipelining, starttls,
/*	auth, etc.) that the Postfix LMTP client will ignore in the LHLO
/*	response
/*	from a remote LMTP server.
/* .IP "\fBlmtp_discard_lhlo_keywords (empty)\fR"
/*	A case insensitive list of LHLO keywords (pipelining, starttls,
/*	auth, etc.) that the Postfix LMTP client will ignore in the LHLO
/*	response
/*	from a remote LMTP server.
/* .PP
/*	Available in Postfix version 2.4.4 and later:
/* .IP "\fBsend_cyrus_sasl_authzid (no)\fR"
/*	When authenticating to a remote SMTP or LMTP server with the
/*	default setting "no", send no SASL authoriZation ID (authzid); send
/*	only the SASL authentiCation ID (authcid) plus the authcid's password.
/* .PP
/*	Available in Postfix version 2.5 and later:
/* .IP "\fBsmtp_header_checks (empty)\fR"
/*	Restricted \fBheader_checks\fR(5) tables for the Postfix SMTP client.
/* .IP "\fBsmtp_mime_header_checks (empty)\fR"
/*	Restricted \fBmime_header_checks\fR(5) tables for the Postfix SMTP
/*	client.
/* .IP "\fBsmtp_nested_header_checks (empty)\fR"
/*	Restricted \fBnested_header_checks\fR(5) tables for the Postfix SMTP
/*	client.
/* .IP "\fBsmtp_body_checks (empty)\fR"
/*	Restricted \fBbody_checks\fR(5) tables for the Postfix SMTP client.
/* .PP
/*	Available in Postfix version 2.6 and later:
/* .IP "\fBtcp_windowsize (0)\fR"
/*	An optional workaround for routers that break TCP window scaling.
/* .PP
/*	Available in Postfix version 2.8 and later:
/* .IP "\fBsmtp_dns_resolver_options (empty)\fR"
/*	DNS Resolver options for the Postfix SMTP client.
/* .PP
/*	Available in Postfix version 2.9 - 3.6:
/* .IP "\fBsmtp_per_record_deadline (no)\fR"
/*	Change the behavior of the smtp_*_timeout time limits, from a
/*	time limit per read or write system call, to a time limit to send
/*	or receive a complete record (an SMTP command line, SMTP response
/*	line, SMTP message content line, or TLS protocol message).
/* .PP
/*	Available in Postfix version 2.9 and later:
/* .IP "\fBsmtp_send_dummy_mail_auth (no)\fR"
/*	Whether or not to append the "AUTH=<>" option to the MAIL
/*	FROM command in SASL-authenticated SMTP sessions.
/* .PP
/*	Available in Postfix version 2.11 and later:
/* .IP "\fBsmtp_dns_support_level (empty)\fR"
/*	Level of DNS support in the Postfix SMTP client.
/* .PP
/*	Available in Postfix version 3.0 and later:
/* .IP "\fBsmtp_delivery_status_filter ($default_delivery_status_filter)\fR"
/*	Optional filter for the \fBsmtp\fR(8) delivery agent to change the
/*	delivery status code or explanatory text of successful or unsuccessful
/*	deliveries.
/* .IP "\fBsmtp_dns_reply_filter (empty)\fR"
/*	Optional filter for Postfix SMTP client DNS lookup results.
/* .PP
/*	Available in Postfix version 3.3 and later:
/* .IP "\fBsmtp_balance_inet_protocols (yes)\fR"
/*	When a remote destination resolves to a combination of IPv4 and
/*	IPv6 addresses, ensure that the Postfix SMTP client can try both
/*	address types before it runs into the smtp_mx_address_limit.
/* .PP
/*	Available in Postfix 3.5 and later:
/* .IP "\fBinfo_log_address_format (external)\fR"
/*	The email address form that will be used in non-debug logging
/*	(info, warning, etc.).
/* .PP
/*	Available in Postfix 3.6 and later:
/* .IP "\fBdnssec_probe (ns:.)\fR"
/*	The DNS query type (default: "ns") and DNS query name (default:
/*	".") that Postfix may use to determine whether DNSSEC validation
/*	is available.
/* .IP "\fBknown_tcp_ports (lmtp=24, smtp=25, smtps=submissions=465, submission=587)\fR"
/*	Optional setting that avoids lookups in the \fBservices\fR(5) database.
/* .PP
/*	Available in Postfix version 3.7 and later:
/* .IP "\fBsmtp_per_request_deadline (no)\fR"
/*	Change the behavior of the smtp_*_timeout time limits, from a
/*	time limit per plaintext or TLS read or write call, to a combined
/*	time limit for sending a complete SMTP request and for receiving a
/*	complete SMTP response.
/* .IP "\fBsmtp_min_data_rate (500)\fR"
/*	The minimum plaintext data transfer rate in bytes/second for
/*	DATA requests, when deadlines are enabled with smtp_per_request_deadline.
/* .IP "\fBheader_from_format (standard)\fR"
/*	The format of the Postfix-generated \fBFrom:\fR header.
/* .PP
/*	Available in Postfix version 3.8 and later:
/* .IP "\fBuse_srv_lookup (empty)\fR"
/*	Enables discovery for the specified service(s) using DNS SRV
/*	records.
/* .IP "\fBignore_srv_lookup_error (no)\fR"
/*	When SRV record lookup fails, fall back to MX or IP address
/*	lookup as if SRV record lookup was not enabled.
/* .IP "\fBallow_srv_lookup_fallback (no)\fR"
/*	When SRV record lookup fails or no SRV record exists, fall back
/*	to MX or IP address lookup as if SRV record lookup was not enabled.
/* MIME PROCESSING CONTROLS
/* .ad
/* .fi
/*	Available in Postfix version 2.0 and later:
/* .IP "\fBdisable_mime_output_conversion (no)\fR"
/*	Disable the conversion of 8BITMIME format to 7BIT format.
/* .IP "\fBmime_boundary_length_limit (2048)\fR"
/*	The maximal length of MIME multipart boundary strings.
/* .IP "\fBmime_nesting_limit (100)\fR"
/*	The maximal recursion level that the MIME processor will handle.
/* EXTERNAL CONTENT INSPECTION CONTROLS
/* .ad
/* .fi
/*	Available in Postfix version 2.1 and later:
/* .IP "\fBsmtp_send_xforward_command (no)\fR"
/*	Send the non-standard XFORWARD command when the Postfix SMTP server
/*	EHLO response announces XFORWARD support.
/* SASL AUTHENTICATION CONTROLS
/* .ad
/* .fi
/* .IP "\fBsmtp_sasl_auth_enable (no)\fR"
/*	Enable SASL authentication in the Postfix SMTP client.
/* .IP "\fBsmtp_sasl_password_maps (empty)\fR"
/*	Optional Postfix SMTP client lookup tables with one username:password
/*	entry per sender, remote hostname or next-hop domain.
/* .IP "\fBsmtp_sasl_security_options (noplaintext, noanonymous)\fR"
/*	Postfix SMTP client SASL security options; as of Postfix 2.3
/*	the list of available
/*	features depends on the SASL client implementation that is selected
/*	with \fBsmtp_sasl_type\fR.
/* .PP
/*	Available in Postfix version 2.2 and later:
/* .IP "\fBsmtp_sasl_mechanism_filter (empty)\fR"
/*	If non-empty, a Postfix SMTP client filter for the remote SMTP
/*	server's list of offered SASL mechanisms.
/* .PP
/*	Available in Postfix version 2.3 and later:
/* .IP "\fBsmtp_sender_dependent_authentication (no)\fR"
/*	Enable sender-dependent authentication in the Postfix SMTP client; this is
/*	available only with SASL authentication, and disables SMTP connection
/*	caching to ensure that mail from different senders will use the
/*	appropriate credentials.
/* .IP "\fBsmtp_sasl_path (empty)\fR"
/*	Implementation-specific information that the Postfix SMTP client
/*	passes through to
/*	the SASL plug-in implementation that is selected with
/*	\fBsmtp_sasl_type\fR.
/* .IP "\fBsmtp_sasl_type (cyrus)\fR"
/*	The SASL plug-in type that the Postfix SMTP client should use
/*	for authentication.
/* .PP
/*	Available in Postfix version 2.5 and later:
/* .IP "\fBsmtp_sasl_auth_cache_name (empty)\fR"
/*	An optional table to prevent repeated SASL authentication
/*	failures with the same remote SMTP server hostname, username and
/*	password.
/* .IP "\fBsmtp_sasl_auth_cache_time (90d)\fR"
/*	The maximal age of an smtp_sasl_auth_cache_name entry before it
/*	is removed.
/* .IP "\fBsmtp_sasl_auth_soft_bounce (yes)\fR"
/*	When a remote SMTP server rejects a SASL authentication request
/*	with a 535 reply code, defer mail delivery instead of returning
/*	mail as undeliverable.
/* .PP
/*	Available in Postfix version 2.9 and later:
/* .IP "\fBsmtp_send_dummy_mail_auth (no)\fR"
/*	Whether or not to append the "AUTH=<>" option to the MAIL
/*	FROM command in SASL-authenticated SMTP sessions.
/* STARTTLS SUPPORT CONTROLS
/* .ad
/* .fi
/*	Detailed information about STARTTLS configuration may be found
/*	in the TLS_README document.
/* .IP "\fBsmtp_tls_security_level (empty)\fR"
/*	The default SMTP TLS security level for the Postfix SMTP client.
/* .IP "\fBsmtp_sasl_tls_security_options ($smtp_sasl_security_options)\fR"
/*	The SASL authentication security options that the Postfix SMTP
/*	client uses for TLS encrypted SMTP sessions.
/* .IP "\fBsmtp_starttls_timeout (300s)\fR"
/*	Time limit for Postfix SMTP client write and read operations
/*	during TLS startup and shutdown handshake procedures.
/* .IP "\fBsmtp_tls_CAfile (empty)\fR"
/*	A file containing CA certificates of root CAs trusted to sign
/*	either remote SMTP server certificates or intermediate CA certificates.
/* .IP "\fBsmtp_tls_CApath (empty)\fR"
/*	Directory with PEM format Certification Authority certificates
/*	that the Postfix SMTP client uses to verify a remote SMTP server
/*	certificate.
/* .IP "\fBsmtp_tls_cert_file (empty)\fR"
/*	File with the Postfix SMTP client RSA certificate in PEM format.
/* .IP "\fBsmtp_tls_mandatory_ciphers (medium)\fR"
/*	The minimum TLS cipher grade that the Postfix SMTP client will
/*	use with
/*	mandatory TLS encryption.
/* .IP "\fBsmtp_tls_exclude_ciphers (empty)\fR"
/*	List of ciphers or cipher types to exclude from the Postfix
/*	SMTP client cipher
/*	list at all TLS security levels.
/* .IP "\fBsmtp_tls_mandatory_exclude_ciphers (empty)\fR"
/*	Additional list of ciphers or cipher types to exclude from the
/*	Postfix SMTP client cipher list at mandatory TLS security levels.
/* .IP "\fBsmtp_tls_dcert_file (empty)\fR"
/*	File with the Postfix SMTP client DSA certificate in PEM format.
/* .IP "\fBsmtp_tls_dkey_file ($smtp_tls_dcert_file)\fR"
/*	File with the Postfix SMTP client DSA private key in PEM format.
/* .IP "\fBsmtp_tls_key_file ($smtp_tls_cert_file)\fR"
/*	File with the Postfix SMTP client RSA private key in PEM format.
/* .IP "\fBsmtp_tls_loglevel (0)\fR"
/*	Enable additional Postfix SMTP client logging of TLS activity.
/* .IP "\fBsmtp_tls_note_starttls_offer (no)\fR"
/*	Log the hostname of a remote SMTP server that offers STARTTLS,
/*	when TLS is not already enabled for that server.
/* .IP "\fBsmtp_tls_policy_maps (empty)\fR"
/*	Optional lookup tables with the Postfix SMTP client TLS security
/*	policy by next-hop destination; when a non-empty value is specified,
/*	this overrides the obsolete smtp_tls_per_site parameter.
/* .IP "\fBsmtp_tls_mandatory_protocols (see 'postconf -d' output)\fR"
/*	TLS protocols that the Postfix SMTP client will use with mandatory
/*	TLS encryption.
/* .IP "\fBsmtp_tls_scert_verifydepth (9)\fR"
/*	The verification depth for remote SMTP server certificates.
/* .IP "\fBsmtp_tls_secure_cert_match (nexthop, dot-nexthop)\fR"
/*	How the Postfix SMTP client verifies the server certificate
/*	peername for the "secure" TLS security level.
/* .IP "\fBsmtp_tls_session_cache_database (empty)\fR"
/*	Name of the file containing the optional Postfix SMTP client
/*	TLS session cache.
/* .IP "\fBsmtp_tls_session_cache_timeout (3600s)\fR"
/*	The expiration time of Postfix SMTP client TLS session cache
/*	information.
/* .IP "\fBsmtp_tls_verify_cert_match (hostname)\fR"
/*	How the Postfix SMTP client verifies the server certificate
/*	peername for the
/*	"verify" TLS security level.
/* .IP "\fBtls_daemon_random_bytes (32)\fR"
/*	The number of pseudo-random bytes that an \fBsmtp\fR(8) or \fBsmtpd\fR(8)
/*	process requests from the \fBtlsmgr\fR(8) server in order to seed its
/*	internal pseudo random number generator (PRNG).
/* .IP "\fBtls_high_cipherlist (see 'postconf -d' output)\fR"
/*	The OpenSSL cipherlist for "high" grade ciphers.
/* .IP "\fBtls_medium_cipherlist (see 'postconf -d' output)\fR"
/*	The OpenSSL cipherlist for "medium" or higher grade ciphers.
/* .IP "\fBtls_null_cipherlist (eNULL:!aNULL)\fR"
/*	The OpenSSL cipherlist for "NULL" grade ciphers that provide
/*	authentication without encryption.
/* .PP
/*	Available in in Postfix version 2.3..3.7:
/* .IP "\fBtls_low_cipherlist (see 'postconf -d' output)\fR"
/*	The OpenSSL cipherlist for "low" or higher grade ciphers.
/* .IP "\fBtls_export_cipherlist (see 'postconf -d' output)\fR"
/*	The OpenSSL cipherlist for "export" or higher grade ciphers.
/* .PP
/*	Available in Postfix version 2.4 and later:
/* .IP "\fBsmtp_sasl_tls_verified_security_options ($smtp_sasl_tls_security_options)\fR"
/*	The SASL authentication security options that the Postfix SMTP
/*	client uses for TLS encrypted SMTP sessions with a verified server
/*	certificate.
/* .PP
/*	Available in Postfix version 2.5 and later:
/* .IP "\fBsmtp_tls_fingerprint_cert_match (empty)\fR"
/*	List of acceptable remote SMTP server certificate fingerprints for
/*	the "fingerprint" TLS security level (\fBsmtp_tls_security_level\fR =
/*	fingerprint).
/* .IP "\fBsmtp_tls_fingerprint_digest (see 'postconf -d' output)\fR"
/*	The message digest algorithm used to construct remote SMTP server
/*	certificate fingerprints.
/* .PP
/*	Available in Postfix version 2.6 and later:
/* .IP "\fBsmtp_tls_protocols (see postconf -d output)\fR"
/*	TLS protocols that the Postfix SMTP client will use with
/*	opportunistic TLS encryption.
/* .IP "\fBsmtp_tls_ciphers (medium)\fR"
/*	The minimum TLS cipher grade that the Postfix SMTP client
/*	will use with opportunistic TLS encryption.
/* .IP "\fBsmtp_tls_eccert_file (empty)\fR"
/*	File with the Postfix SMTP client ECDSA certificate in PEM format.
/* .IP "\fBsmtp_tls_eckey_file ($smtp_tls_eccert_file)\fR"
/*	File with the Postfix SMTP client ECDSA private key in PEM format.
/* .PP
/*	Available in Postfix version 2.7 and later:
/* .IP "\fBsmtp_tls_block_early_mail_reply (no)\fR"
/*	Try to detect a mail hijacking attack based on a TLS protocol
/*	vulnerability (CVE-2009-3555), where an attacker prepends malicious
/*	HELO, MAIL, RCPT, DATA commands to a Postfix SMTP client TLS session.
/* .PP
/*	Available in Postfix version 2.8 and later:
/* .IP "\fBtls_disable_workarounds (see 'postconf -d' output)\fR"
/*	List or bit-mask of OpenSSL bug work-arounds to disable.
/* .PP
/*	Available in Postfix version 2.11-3.1:
/* .IP "\fBtls_dane_digest_agility (on)\fR"
/*	Configure RFC7671 DANE TLSA digest algorithm agility.
/* .IP "\fBtls_dane_trust_anchor_digest_enable (yes)\fR"
/*	Enable support for RFC 6698 (DANE TLSA) DNS records that contain
/*	digests of trust-anchors with certificate usage "2".
/* .PP
/*	Available in Postfix version 2.11 and later:
/* .IP "\fBsmtp_tls_trust_anchor_file (empty)\fR"
/*	Zero or more PEM-format files with trust-anchor certificates
/*	and/or public keys.
/* .IP "\fBsmtp_tls_force_insecure_host_tlsa_lookup (no)\fR"
/*	Lookup the associated DANE TLSA RRset even when a hostname is
/*	not an alias and its address records lie in an unsigned zone.
/* .IP "\fBtlsmgr_service_name (tlsmgr)\fR"
/*	The name of the \fBtlsmgr\fR(8) service entry in master.cf.
/* .PP
/*	Available in Postfix version 3.0 and later:
/* .IP "\fBsmtp_tls_wrappermode (no)\fR"
/*	Request that the Postfix SMTP client connects using the
/*	SUBMISSIONS/SMTPS protocol instead of using the STARTTLS command.
/* .PP
/*	Available in Postfix version 3.1 and later:
/* .IP "\fBsmtp_tls_dane_insecure_mx_policy (see 'postconf -d' output)\fR"
/*	The TLS policy for MX hosts with "secure" TLSA records when the
/*	nexthop destination security level is \fBdane\fR, but the MX
/*	record was found via an "insecure" MX lookup.
/* .PP
/*	Available in Postfix version 3.2 and later:
/* .IP "\fBtls_eecdh_auto_curves (see 'postconf -d' output)\fR"
/*	The prioritized list of elliptic curves supported by the Postfix
/*	SMTP client and server.
/* .PP
/*	Available in Postfix version 3.4 and later:
/* .IP "\fBsmtp_tls_connection_reuse (no)\fR"
/*	Try to make multiple deliveries per TLS-encrypted connection.
/* .IP "\fBsmtp_tls_chain_files (empty)\fR"
/*	List of one or more PEM files, each holding one or more private keys
/*	directly followed by a corresponding certificate chain.
/* .IP "\fBsmtp_tls_servername (empty)\fR"
/*	Optional name to send to the remote SMTP server in the TLS Server
/*	Name Indication (SNI) extension.
/* .PP
/*	Available in Postfix 3.5, 3.4.6, 3.3.5, 3.2.10, 3.1.13 and later:
/* .IP "\fBtls_fast_shutdown_enable (yes)\fR"
/*	A workaround for implementations that hang Postfix while shutting
/*	down a TLS session, until Postfix times out.
/* .PP
/*	Available in Postfix version 3.8 and later:
/* .IP "\fBtls_ffdhe_auto_groups (see 'postconf -d' output)\fR"
/*	The prioritized list of finite-field Diffie-Hellman ephemeral
/*	(FFDHE) key exchange groups supported by the Postfix SMTP client and
/*	server.
/* .PP
/*	Available in Postfix 3.9, 3.8.1, 3.7.6, 3.6.10, 3.5.20 and later:
/* .IP "\fBtls_config_file (default)\fR"
/*	Optional configuration file with baseline OpenSSL settings.
/* .IP "\fBtls_config_name (empty)\fR"
/*	The application name passed by Postfix to OpenSSL library
/*	initialization functions.
/* OBSOLETE STARTTLS CONTROLS
/* .ad
/* .fi
/*	The following configuration parameters exist for compatibility
/*	with Postfix versions before 2.3. Support for these will
/*	be removed in a future release.
/* .IP "\fBsmtp_use_tls (no)\fR"
/*	Opportunistic mode: use TLS when a remote SMTP server announces
/*	STARTTLS support, otherwise send the mail in the clear.
/* .IP "\fBsmtp_enforce_tls (no)\fR"
/*	Enforcement mode: require that remote SMTP servers use TLS
/*	encryption, and never send mail in the clear.
/* .IP "\fBsmtp_tls_enforce_peername (yes)\fR"
/*	With mandatory TLS encryption, require that the remote SMTP
/*	server hostname matches the information in the remote SMTP server
/*	certificate.
/* .IP "\fBsmtp_tls_per_site (empty)\fR"
/*	Optional lookup tables with the Postfix SMTP client TLS usage
/*	policy by next-hop destination and by remote SMTP server hostname.
/* .IP "\fBsmtp_tls_cipherlist (empty)\fR"
/*	Obsolete Postfix < 2.3 control for the Postfix SMTP client TLS
/*	cipher list.
/* RESOURCE AND RATE CONTROLS
/* .ad
/* .fi
/* .IP "\fBsmtp_connect_timeout (30s)\fR"
/*	The Postfix SMTP client time limit for completing a TCP connection, or
/*	zero (use the operating system built-in time limit).
/* .IP "\fBsmtp_helo_timeout (300s)\fR"
/*	The Postfix SMTP client time limit for sending the HELO or EHLO command,
/*	and for receiving the initial remote SMTP server response.
/* .IP "\fBlmtp_lhlo_timeout (300s)\fR"
/*	The Postfix LMTP client time limit for sending the LHLO command,
/*	and for receiving the initial remote LMTP server response.
/* .IP "\fBsmtp_xforward_timeout (300s)\fR"
/*	The Postfix SMTP client time limit for sending the XFORWARD command,
/*	and for receiving the remote SMTP server response.
/* .IP "\fBsmtp_mail_timeout (300s)\fR"
/*	The Postfix SMTP client time limit for sending the MAIL FROM command,
/*	and for receiving the remote SMTP server response.
/* .IP "\fBsmtp_rcpt_timeout (300s)\fR"
/*	The Postfix SMTP client time limit for sending the SMTP RCPT TO
/*	command, and for receiving the remote SMTP server response.
/* .IP "\fBsmtp_data_init_timeout (120s)\fR"
/*	The Postfix SMTP client time limit for sending the SMTP DATA command,
/*	and for receiving the remote SMTP server response.
/* .IP "\fBsmtp_data_xfer_timeout (180s)\fR"
/*	The Postfix SMTP client time limit for sending the SMTP message content.
/* .IP "\fBsmtp_data_done_timeout (600s)\fR"
/*	The Postfix SMTP client time limit for sending the SMTP ".", and
/*	for receiving the remote SMTP server response.
/* .IP "\fBsmtp_quit_timeout (300s)\fR"
/*	The Postfix SMTP client time limit for sending the QUIT command,
/*	and for receiving the remote SMTP server response.
/* .PP
/*	Available in Postfix version 2.1 and later:
/* .IP "\fBsmtp_mx_address_limit (5)\fR"
/*	The maximal number of MX (mail exchanger) IP addresses that can
/*	result from Postfix SMTP client mail exchanger lookups, or zero (no
/*	limit).
/* .IP "\fBsmtp_mx_session_limit (2)\fR"
/*	The maximal number of SMTP sessions per delivery request before
/*	the Postfix SMTP client
/*	gives up or delivers to a fall-back relay host, or zero (no
/*	limit).
/* .IP "\fBsmtp_rset_timeout (20s)\fR"
/*	The Postfix SMTP client time limit for sending the RSET command,
/*	and for receiving the remote SMTP server response.
/* .PP
/*	Available in Postfix version 2.2 and earlier:
/* .IP "\fBlmtp_cache_connection (yes)\fR"
/*	Keep Postfix LMTP client connections open for up to $max_idle
/*	seconds.
/* .PP
/*	Available in Postfix version 2.2 and later:
/* .IP "\fBsmtp_connection_cache_destinations (empty)\fR"
/*	Permanently enable SMTP connection caching for the specified
/*	destinations.
/* .IP "\fBsmtp_connection_cache_on_demand (yes)\fR"
/*	Temporarily enable SMTP connection caching while a destination
/*	has a high volume of mail in the active queue.
/* .IP "\fBsmtp_connection_reuse_time_limit (300s)\fR"
/*	The amount of time during which Postfix will use an SMTP
/*	connection repeatedly.
/* .IP "\fBsmtp_connection_cache_time_limit (2s)\fR"
/*	When SMTP connection caching is enabled, the amount of time that
/*	an unused SMTP client socket is kept open before it is closed.
/* .PP
/*	Available in Postfix version 2.3 and later:
/* .IP "\fBconnection_cache_protocol_timeout (5s)\fR"
/*	Time limit for connection cache connect, send or receive
/*	operations.
/* .PP
/*	Available in Postfix version 2.9 - 3.6:
/* .IP "\fBsmtp_per_record_deadline (no)\fR"
/*	Change the behavior of the smtp_*_timeout time limits, from a
/*	time limit per read or write system call, to a time limit to send
/*	or receive a complete record (an SMTP command line, SMTP response
/*	line, SMTP message content line, or TLS protocol message).
/* .PP
/*	Available in Postfix version 2.11 and later:
/* .IP "\fBsmtp_connection_reuse_count_limit (0)\fR"
/*	When SMTP connection caching is enabled, the number of times
/*	that an SMTP session may be reused before it is closed, or zero (no
/*	limit).
/* .PP
/*	Available in Postfix version 3.4 and later:
/* .IP "\fBsmtp_tls_connection_reuse (no)\fR"
/*	Try to make multiple deliveries per TLS-encrypted connection.
/* .PP
/*	Available in Postfix version 3.7 and later:
/* .IP "\fBsmtp_per_request_deadline (no)\fR"
/*	Change the behavior of the smtp_*_timeout time limits, from a
/*	time limit per plaintext or TLS read or write call, to a combined
/*	time limit for sending a complete SMTP request and for receiving a
/*	complete SMTP response.
/* .IP "\fBsmtp_min_data_rate (500)\fR"
/*	The minimum plaintext data transfer rate in bytes/second for
/*	DATA requests, when deadlines are enabled with smtp_per_request_deadline.
/* .PP
/*	Implemented in the qmgr(8) daemon:
/* .IP "\fBtransport_destination_concurrency_limit ($default_destination_concurrency_limit)\fR"
/*	A transport-specific override for the
/*	default_destination_concurrency_limit parameter value, where
/*	\fItransport\fR is the master.cf name of the message delivery
/*	transport.
/* .IP "\fBtransport_destination_recipient_limit ($default_destination_recipient_limit)\fR"
/*	A transport-specific override for the
/*	default_destination_recipient_limit parameter value, where
/*	\fItransport\fR is the master.cf name of the message delivery
/*	transport.
/* SMTPUTF8 CONTROLS
/* .ad
/* .fi
/*	Preliminary SMTPUTF8 support is introduced with Postfix 3.0.
/* .IP "\fBsmtputf8_enable (yes)\fR"
/*	Enable preliminary SMTPUTF8 support for the protocols described
/*	in RFC 6531, RFC 6532, and RFC 6533.
/* .IP "\fBsmtputf8_autodetect_classes (sendmail, verify)\fR"
/*	Detect that a message requires SMTPUTF8 support for the specified
/*	mail origin classes.
/* .PP
/*	Available in Postfix version 3.2 and later:
/* .IP "\fBenable_idna2003_compatibility (no)\fR"
/*	Enable 'transitional' compatibility between IDNA2003 and IDNA2008,
/*	when converting UTF-8 domain names to/from the ASCII form that is
/*	used for DNS lookups.
/* TROUBLE SHOOTING CONTROLS
/* .ad
/* .fi
/* .IP "\fBdebug_peer_level (2)\fR"
/*	The increment in verbose logging level when a nexthop destination,
/*	remote client or server name or network address matches a pattern
/*	given with the debug_peer_list parameter.
/* .IP "\fBdebug_peer_list (empty)\fR"
/*	Optional list of nexthop destination, remote client or server
/*	name or network address patterns that, if matched, cause the verbose
/*	logging level to increase by the amount specified in $debug_peer_level.
/* .IP "\fBerror_notice_recipient (postmaster)\fR"
/*	The recipient of postmaster notifications about mail delivery
/*	problems that are caused by policy, resource, software or protocol
/*	errors.
/* .IP "\fBinternal_mail_filter_classes (empty)\fR"
/*	What categories of Postfix-generated mail are subject to
/*	before-queue content inspection by non_smtpd_milters, header_checks
/*	and body_checks.
/* .IP "\fBnotify_classes (resource, software)\fR"
/*	The list of error classes that are reported to the postmaster.
/* MISCELLANEOUS CONTROLS
/* .ad
/* .fi
/* .IP "\fBbest_mx_transport (empty)\fR"
/*	Where the Postfix SMTP client should deliver mail when it detects
/*	a "mail loops back to myself" error condition.
/* .IP "\fBconfig_directory (see 'postconf -d' output)\fR"
/*	The default location of the Postfix main.cf and master.cf
/*	configuration files.
/* .IP "\fBdaemon_timeout (18000s)\fR"
/*	How much time a Postfix daemon process may take to handle a
/*	request before it is terminated by a built-in watchdog timer.
/* .IP "\fBdelay_logging_resolution_limit (2)\fR"
/*	The maximal number of digits after the decimal point when logging
/*	sub-second delay values.
/* .IP "\fBdisable_dns_lookups (no)\fR"
/*	Disable DNS lookups in the Postfix SMTP and LMTP clients.
/* .IP "\fBinet_interfaces (all)\fR"
/*	The local network interface addresses that this mail system receives
/*	mail on.
/* .IP "\fBinet_protocols (see 'postconf -d output')\fR"
/*	The Internet protocols Postfix will attempt to use when making
/*	or accepting connections.
/* .IP "\fBipc_timeout (3600s)\fR"
/*	The time limit for sending or receiving information over an internal
/*	communication channel.
/* .IP "\fBlmtp_assume_final (no)\fR"
/*	When a remote LMTP server announces no DSN support, assume that
/*	the
/*	server performs final delivery, and send "delivered" delivery status
/*	notifications instead of "relayed".
/* .IP "\fBlmtp_tcp_port (24)\fR"
/*	The default TCP port that the Postfix LMTP client connects to.
/* .IP "\fBmax_idle (100s)\fR"
/*	The maximum amount of time that an idle Postfix daemon process waits
/*	for an incoming connection before terminating voluntarily.
/* .IP "\fBmax_use (100)\fR"
/*	The maximal number of incoming connections that a Postfix daemon
/*	process will service before terminating voluntarily.
/* .IP "\fBprocess_id (read-only)\fR"
/*	The process ID of a Postfix command or daemon process.
/* .IP "\fBprocess_name (read-only)\fR"
/*	The process name of a Postfix command or daemon process.
/* .IP "\fBproxy_interfaces (empty)\fR"
/*	The remote network interface addresses that this mail system receives mail
/*	on by way of a proxy or network address translation unit.
/* .IP "\fBsmtp_address_preference (any)\fR"
/*	The address type ("ipv6", "ipv4" or "any") that the Postfix
/*	SMTP client will try first, when a destination has IPv6 and IPv4
/*	addresses with equal MX preference.
/* .IP "\fBsmtp_bind_address (empty)\fR"
/*	An optional numerical network address that the Postfix SMTP client
/*	should bind to when making an IPv4 connection.
/* .IP "\fBsmtp_bind_address6 (empty)\fR"
/*	An optional numerical network address that the Postfix SMTP client
/*	should bind to when making an IPv6 connection.
/* .IP "\fBsmtp_helo_name ($myhostname)\fR"
/*	The hostname to send in the SMTP HELO or EHLO command.
/* .IP "\fBlmtp_lhlo_name ($myhostname)\fR"
/*	The hostname to send in the LMTP LHLO command.
/* .IP "\fBsmtp_host_lookup (dns)\fR"
/*	What mechanisms the Postfix SMTP client uses to look up a host's
/*	IP address.
/* .IP "\fBsmtp_randomize_addresses (yes)\fR"
/*	Randomize the order of equal-preference MX host addresses.
/* .IP "\fBsyslog_facility (mail)\fR"
/*	The syslog facility of Postfix logging.
/* .IP "\fBsyslog_name (see 'postconf -d' output)\fR"
/*	A prefix that is prepended to the process name in syslog
/*	records, so that, for example, "smtpd" becomes "prefix/smtpd".
/* .PP
/*	Available with Postfix 2.2 and earlier:
/* .IP "\fBfallback_relay (empty)\fR"
/*	Optional list of relay hosts for SMTP destinations that can't be
/*	found or that are unreachable.
/* .PP
/*	Available with Postfix 2.3 and later:
/* .IP "\fBsmtp_fallback_relay ($fallback_relay)\fR"
/*	Optional list of relay destinations that will be used when an
/*	SMTP destination is not found, or when delivery fails due to a
/*	non-permanent error.
/* .PP
/*	Available with Postfix 3.0 and later:
/* .IP "\fBsmtp_address_verify_target (rcpt)\fR"
/*	In the context of email address verification, the SMTP protocol
/*	stage that determines whether an email address is deliverable.
/* .PP
/*	Available with Postfix 3.1 and later:
/* .IP "\fBlmtp_fallback_relay (empty)\fR"
/*	Optional list of relay hosts for LMTP destinations that can't be
/*	found or that are unreachable.
/* .PP
/*	Available with Postfix 3.2 and later:
/* .IP "\fBsmtp_tcp_port (smtp)\fR"
/*	The default TCP port that the Postfix SMTP client connects to.
/* .PP
/*	Available in Postfix 3.3 and later:
/* .IP "\fBservice_name (read-only)\fR"
/*	The master.cf service name of a Postfix daemon process.
/* .PP
/*	Available in Postfix 3.7 and later:
/* .IP "\fBsmtp_bind_address_enforce (no)\fR"
/*	Defer delivery when the Postfix SMTP client cannot apply the
/*	smtp_bind_address or smtp_bind_address6 setting.
/* SEE ALSO
/*	generic(5), output address rewriting
/*	header_checks(5), message header content inspection
/*	body_checks(5), body parts content inspection
/*	qmgr(8), queue manager
/*	bounce(8), delivery status reports
/*	scache(8), connection cache server
/*	postconf(5), configuration parameters
/*	master(5), generic daemon options
/*	master(8), process manager
/*	tlsmgr(8), TLS session and PRNG management
/*	postlogd(8), Postfix logging
/*	syslogd(8), system logging
/* README FILES
/* .ad
/* .fi
/*	Use "\fBpostconf readme_directory\fR" or
/*	"\fBpostconf html_directory\fR" to locate this information.
/* .na
/* .nf
/*	SASL_README, Postfix SASL howto
/*	TLS_README, Postfix STARTTLS howto
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*
/*	Command pipelining in cooperation with:
/*	Jon Ribbens
/*	Oaktree Internet Solutions Ltd.,
/*	Internet House,
/*	Canal Basin,
/*	Coventry,
/*	CV1 4LY, United Kingdom.
/*
/*	SASL support originally by:
/*	Till Franke
/*	SuSE Rhein/Main AG
/*	65760 Eschborn, Germany
/*
/*	TLS support originally by:
/*	Lutz Jaenicke
/*	BTU Cottbus
/*	Allgemeine Elektrotechnik
/*	Universitaetsplatz 3-4
/*	D-03044 Cottbus, Germany
/*
/*	Revised TLS and SMTP connection cache support by:
/*	Victor Duchovni
/*	Morgan Stanley
/*--*/

/* System library. */

#include <sys_defs.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <dict.h>
#include <stringops.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <name_mask.h>
#include <name_code.h>
#include <byte_mask.h>

/* Global library. */

#include <deliver_request.h>
#include <delivered_hdr.h>
#include <mail_proto.h>
#include <mail_params.h>
#include <mail_version.h>
#include <mail_conf.h>
#include <debug_peer.h>
#include <flush_clnt.h>
#include <scache.h>
#include <string_list.h>
#include <maps.h>
#include <ext_prop.h>
#include <hfrom_format.h>

/* DNS library. */

#include <dns.h>

/* Single server skeleton. */

#include <mail_server.h>

/* Application-specific. */

#include "smtp.h"
#include "smtp_sasl.h"

 /*
  * Tunable parameters. These have compiled-in defaults that can be overruled
  * by settings in the global Postfix configuration file.
  */
int     var_smtp_conn_tmout;
int     var_smtp_helo_tmout;
int     var_smtp_xfwd_tmout;
int     var_smtp_mail_tmout;
int     var_smtp_rcpt_tmout;
int     var_smtp_data0_tmout;
int     var_smtp_data1_tmout;
int     var_smtp_data2_tmout;
int     var_smtp_rset_tmout;
int     var_smtp_quit_tmout;
char   *var_notify_classes;
int     var_smtp_skip_5xx_greeting;
int     var_ign_mx_lookup_err;
int     var_skip_quit_resp;
char   *var_fallback_relay;
char   *var_bestmx_transp;
char   *var_error_rcpt;
int     var_smtp_always_ehlo;
int     var_smtp_never_ehlo;
char   *var_smtp_sasl_opts;
char   *var_smtp_sasl_path;
char   *var_smtp_sasl_passwd;
bool    var_smtp_sasl_enable;
char   *var_smtp_sasl_mechs;
char   *var_smtp_sasl_type;
char   *var_smtp_bind_addr;
char   *var_smtp_bind_addr6;
char   *var_smtp_vrfy_tgt;
bool    var_smtp_rand_addr;
int     var_smtp_pix_thresh;
int     var_queue_run_delay;
int     var_min_backoff_time;
int     var_smtp_pix_delay;
int     var_smtp_line_limit;
char   *var_smtp_helo_name;
char   *var_smtp_host_lookup;
bool    var_smtp_quote_821_env;
bool    var_smtp_defer_mxaddr;
bool    var_smtp_send_xforward;
int     var_smtp_mxaddr_limit;
int     var_smtp_mxsess_limit;
int     var_smtp_cache_conn;
int     var_smtp_reuse_time;
int     var_smtp_reuse_count;
char   *var_smtp_cache_dest;
char   *var_scache_service;		/* You can now leave this here. */
bool    var_smtp_cache_demand;
char   *var_smtp_ehlo_dis_words;
char   *var_smtp_ehlo_dis_maps;
char   *var_smtp_addr_pref;

char   *var_smtp_tls_level;
bool    var_smtp_use_tls;
bool    var_smtp_enforce_tls;
char   *var_smtp_tls_per_site;
char   *var_smtp_tls_policy;
bool    var_smtp_tls_wrappermode;
bool    var_smtp_tls_conn_reuse;
char   *var_tlsproxy_service;

#ifdef USE_TLS
char   *var_smtp_sasl_tls_opts;
char   *var_smtp_sasl_tlsv_opts;
int     var_smtp_starttls_tmout;
char   *var_smtp_tls_CAfile;
char   *var_smtp_tls_CApath;
char   *var_smtp_tls_chain_files;
char   *var_smtp_tls_cert_file;
char   *var_smtp_tls_mand_ciph;
char   *var_smtp_tls_excl_ciph;
char   *var_smtp_tls_mand_excl;
char   *var_smtp_tls_dcert_file;
char   *var_smtp_tls_dkey_file;
bool    var_smtp_tls_enforce_peername;
char   *var_smtp_tls_key_file;
char   *var_smtp_tls_loglevel;
bool    var_smtp_tls_note_starttls_offer;
char   *var_smtp_tls_mand_proto;
char   *var_smtp_tls_sec_cmatch;
int     var_smtp_tls_scert_vd;
char   *var_smtp_tls_vfy_cmatch;
char   *var_smtp_tls_fpt_cmatch;
char   *var_smtp_tls_fpt_dgst;
char   *var_smtp_tls_tafile;
char   *var_smtp_tls_proto;
char   *var_smtp_tls_ciph;
char   *var_smtp_tls_eccert_file;
char   *var_smtp_tls_eckey_file;
char   *var_smtp_tls_sni;
bool    var_smtp_tls_blk_early_mail_reply;
bool    var_smtp_tls_force_tlsa;
char   *var_smtp_tls_insecure_mx_policy;

#endif

char   *var_smtp_generic_maps;
char   *var_prop_extension;
bool    var_smtp_sender_auth;
char   *var_smtp_tcp_port;
int     var_scache_proto_tmout;
bool    var_smtp_cname_overr;
char   *var_smtp_pix_bug_words;
char   *var_smtp_pix_bug_maps;
char   *var_cyrus_conf_path;
char   *var_smtp_head_chks;
char   *var_smtp_mime_chks;
char   *var_smtp_nest_chks;
char   *var_smtp_body_chks;
char   *var_smtp_resp_filter;
bool    var_lmtp_assume_final;
char   *var_smtp_dns_res_opt;
char   *var_smtp_dns_support;
bool    var_smtp_dummy_mail_auth;
char   *var_smtp_dsn_filter;
char   *var_smtp_dns_re_filter;
bool    var_smtp_balance_inet_proto;
bool    var_smtp_req_deadline;
int     var_smtp_min_data_rate;
char   *var_use_srv_lookup;
bool	var_ign_srv_lookup_err;
bool	var_allow_srv_fallback;

 /* Special handling of 535 AUTH errors. */
char   *var_smtp_sasl_auth_cache_name;
int     var_smtp_sasl_auth_cache_time;
bool    var_smtp_sasl_auth_soft_bounce;

char   *var_hfrom_format;
bool var_smtp_bind_addr_enforce;

 /*
  * Global variables.
  */
int     smtp_mode;
int     smtp_host_lookup_mask;
int     smtp_dns_support;
STRING_LIST *smtp_cache_dest;
SCACHE *smtp_scache;
MAPS   *smtp_ehlo_dis_maps;
MAPS   *smtp_generic_maps;
int     smtp_ext_prop_mask;
unsigned smtp_dns_res_opt;
MAPS   *smtp_pix_bug_maps;
HBC_CHECKS *smtp_header_checks;		/* limited header checks */
HBC_CHECKS *smtp_body_checks;		/* limited body checks */
SMTP_CLI_ATTR smtp_cli_attr;		/* parsed command-line */
int     smtp_hfrom_format;		/* postmaster notifications */
STRING_LIST *smtp_use_srv_lookup;

#ifdef USE_TLS

 /*
  * OpenSSL client state (opaque handle)
  */
TLS_APPL_STATE *smtp_tls_ctx;
int     smtp_tls_insecure_mx_policy;

#endif

 /*
  * IPv6 preference.
  */
static int smtp_addr_pref;

/* get_cli_attr - get command-line attributes */

static void get_cli_attr(SMTP_CLI_ATTR *attr, char **argv)
{
    const char myname[] = "get_cli_attr";
    const char *last_flags = "flags=";	/* i.e. empty */
    static const BYTE_MASK flags_map[] = {
	'D', SMTP_CLI_FLAG_DELIVERED_TO,
	'O', SMTP_CLI_FLAG_ORIG_RCPT,
	'R', SMTP_CLI_FLAG_RETURN_PATH,
	'X', SMTP_CLI_FLAG_FINAL_DELIVERY,
	0,
    };

    /*
     * Initialize.
     */
    attr->flags = 0;

    /*
     * Iterate over the command-line attribute list. Errors are fatal.
     */
    for ( /* void */ ; *argv != 0; argv++) {

	/*
	 * flags=stuff. Errors are fatal.
	 */
	if (strncasecmp("flags=", *argv, sizeof("flags=") - 1) == 0) {
	    last_flags = *argv;
	    if (msg_verbose)
		msg_info("%s: %s", myname, last_flags);
	    attr->flags = byte_mask(*argv, flags_map,
				    *argv + sizeof("flags=") - 1);
	}

	/*
	 * Bad.
	 */
	else
	    msg_fatal("unknown attribute name: %s", *argv);
    }

    /*
     * Backwards compatibility, redundancy, and obsolescence.
     */
    if (!smtp_mode && var_lmtp_assume_final
	&& (attr->flags & SMTP_CLI_FLAG_FINAL_DELIVERY) == 0) {
	attr->flags |= SMTP_CLI_FLAG_FINAL_DELIVERY;
	msg_warn("%s is obsolete; instead, specify \"%sX\" in %s",
		 VAR_LMTP_ASSUME_FINAL, last_flags, MASTER_CONF_FILE);
    }
}

/* deliver_message - deliver message with extreme prejudice */

static int deliver_message(const char *service, DELIVER_REQUEST *request)
{
    SMTP_STATE *state;
    int     result;

    if (msg_verbose)
	msg_info("deliver_message: from %s", request->sender);

    /*
     * Sanity checks. The smtp server is unprivileged and chrooted, so we can
     * afford to distribute the data censoring code, instead of having it all
     * in one place.
     */
    if (request->nexthop[0] == 0)
	msg_fatal("empty nexthop hostname");
    if (request->rcpt_list.len <= 0)
	msg_fatal("recipient count: %d", request->rcpt_list.len);

    /*
     * D flag checks.
     */
    if (smtp_cli_attr.flags & SMTP_CLI_FLAG_DELIVERED_TO) {

	/*
	 * The D flag cannot be specified for multi-recipient deliveries.
	 */
	if (request->rcpt_list.len > 1) {
	    msg_warn("flag `D' requires %s_destination_recipient_limit = 1",
		     service);
	    return (reject_deliver_request(service, request, "4.3.5",
					"mail system configuration error"));
	}

	/*
	 * The recipient cannot appear in a Delivered-To: header.
	 */
	else {
	    DELIVERED_HDR_INFO *delivered_info = delivered_hdr_init(
			  request->fp, request->data_offset, FOLD_ADDR_ALL);
	    VSTRING *generic_rcpt = vstring_alloc(100);
	    int     have_delivered_loop;

	    smtp_rewrite_generic_internal(generic_rcpt,
					  request->rcpt_list.info->address);
	    have_delivered_loop = delivered_hdr_find(
					 delivered_info, STR(generic_rcpt));
	    vstring_free(generic_rcpt);
	    delivered_hdr_free(delivered_info);
	    if (have_delivered_loop) {
		return (reject_deliver_request(service, request, "5.4.6",
					       "mail forwarding loop for %s",
					 request->rcpt_list.info->address));
	    }
	}
    }

    /*
     * The O flag cannot be specified for multi-recipient deliveries.
     */
    if ((smtp_cli_attr.flags & SMTP_CLI_FLAG_ORIG_RCPT)
	&& request->rcpt_list.len > 1) {
	msg_warn("flag `O' requires %s_destination_recipient_limit = 1",
		 service);
	return (reject_deliver_request(service, request, "4.3.5",
				       "mail system configuration error"));
    }

    /*
     * Initialize. Bundle all information about the delivery request, so that
     * we can produce understandable diagnostics when something goes wrong
     * many levels below. The alternative would be to make everything global.
     */
    state = smtp_state_alloc();
    state->request = request;
    state->src = request->fp;
    state->service = service;
    state->misc_flags |= smtp_addr_pref;
    state->debug_peer_per_nexthop =
	debug_peer_check(request->nexthop, "noaddr");
    SMTP_RCPT_INIT(state);

    /*
     * Establish an SMTP session and deliver this message to all requested
     * recipients. At the end, notify the postmaster of any protocol errors.
     * Optionally deliver mail locally when this machine is the best mail
     * exchanger.
     */
    result = smtp_connect(state);

    /*
     * Clean up.
     */
    smtp_state_free(state);

    return (result);
}

/* smtp_service - perform service for client */

static void smtp_service(VSTREAM *client_stream, char *service,
			         char **unused_argv)
{
    DELIVER_REQUEST *request;
    int     status;

    /*
     * This routine runs whenever a client connects to the UNIX-domain socket
     * dedicated to remote SMTP delivery service. What we see below is a
     * little protocol to (1) tell the queue manager that we are ready, (2)
     * read a request from the queue manager, and (3) report the completion
     * status of that request. All connection-management stuff is handled by
     * the common code in single_server.c.
     */
    if ((request = deliver_request_read(client_stream)) != 0) {
	status = deliver_message(service, request);
	deliver_request_done(client_stream, request, status);
    }
}

/* post_init - post-jail initialization */

static void post_init(char *unused_name, char **argv)
{
    static const NAME_MASK lookup_masks[] = {
	SMTP_HOST_LOOKUP_DNS, SMTP_HOST_FLAG_DNS,
	SMTP_HOST_LOOKUP_NATIVE, SMTP_HOST_FLAG_NATIVE,
	0,
    };
    static const NAME_MASK dns_res_opt_masks[] = {
	SMTP_DNS_RES_OPT_DEFNAMES, RES_DEFNAMES,
	SMTP_DNS_RES_OPT_DNSRCH, RES_DNSRCH,
	0,
    };
    static const NAME_CODE dns_support[] = {
	SMTP_DNS_SUPPORT_DISABLED, SMTP_DNS_DISABLED,
	SMTP_DNS_SUPPORT_ENABLED, SMTP_DNS_ENABLED,
#if (RES_USE_DNSSEC != 0) && (RES_USE_EDNS0 != 0)
	SMTP_DNS_SUPPORT_DNSSEC, SMTP_DNS_DNSSEC,
#endif
	0, SMTP_DNS_INVALID,
    };

    if (*var_smtp_dns_support == 0) {
	/* Backwards compatible empty setting */
	smtp_dns_support =
	    var_disable_dns ? SMTP_DNS_DISABLED : SMTP_DNS_ENABLED;
    } else {
	smtp_dns_support =
	    name_code(dns_support, NAME_CODE_FLAG_NONE, var_smtp_dns_support);
	if (smtp_dns_support == SMTP_DNS_INVALID)
	    msg_fatal("invalid %s: \"%s\"", VAR_LMTP_SMTP(DNS_SUPPORT),
		      var_smtp_dns_support);
	var_disable_dns = (smtp_dns_support == SMTP_DNS_DISABLED);
    }

#ifdef USE_TLS
    if (smtp_mode) {
	smtp_tls_insecure_mx_policy =
	    tls_level_lookup(var_smtp_tls_insecure_mx_policy);
	switch (smtp_tls_insecure_mx_policy) {
	case TLS_LEV_MAY:
	case TLS_LEV_ENCRYPT:
	case TLS_LEV_DANE:
	    break;
	default:
	    msg_fatal("invalid %s: \"%s\"", VAR_SMTP_TLS_INSECURE_MX_POLICY,
		      var_smtp_tls_insecure_mx_policy);
	}
    }
#endif

    /*
     * Select hostname lookup mechanisms.
     */
    if (smtp_dns_support == SMTP_DNS_DISABLED)
	smtp_host_lookup_mask = SMTP_HOST_FLAG_NATIVE;
    else
	smtp_host_lookup_mask =
	    name_mask(VAR_LMTP_SMTP(HOST_LOOKUP), lookup_masks,
		      var_smtp_host_lookup);
    if (msg_verbose)
	msg_info("host name lookup methods: %s",
		 str_name_mask(VAR_LMTP_SMTP(HOST_LOOKUP), lookup_masks,
			       smtp_host_lookup_mask));

    /*
     * Session cache instance.
     */
    if (*var_smtp_cache_dest || var_smtp_cache_demand)
#if 0
	smtp_scache = scache_multi_create();
#else
	smtp_scache = scache_clnt_create(var_scache_service,
					 var_scache_proto_tmout,
					 var_ipc_idle_limit,
					 var_ipc_ttl_limit);
#endif

    /*
     * Select DNS query flags.
     */
    smtp_dns_res_opt = name_mask(VAR_LMTP_SMTP(DNS_RES_OPT), dns_res_opt_masks,
				 var_smtp_dns_res_opt);

    /*
     * Address verification.
     */
    smtp_vrfy_init();

    /*
     * Look up service command-line attributes; these do not change during
     * the process lifetime.
     */
    get_cli_attr(&smtp_cli_attr, argv);

    /*
     * header_from format, for postmaster notifications.
     */
    smtp_hfrom_format = hfrom_format_parse(VAR_HFROM_FORMAT, var_hfrom_format);

    /*
     * Service discovery with SRV record lookup.
     */
    if (*var_use_srv_lookup)
	smtp_use_srv_lookup = string_list_init(VAR_USE_SRV_LOOKUP,
					       MATCH_FLAG_RETURN,
					       var_use_srv_lookup);
}

/* pre_init - pre-jail initialization */

static void pre_init(char *unused_name, char **unused_argv)
{
    int     use_tls;
    static const NAME_CODE addr_pref_map[] = {
	INET_PROTO_NAME_IPV6, SMTP_MISC_FLAG_PREF_IPV6,
	INET_PROTO_NAME_IPV4, SMTP_MISC_FLAG_PREF_IPV4,
	INET_PROTO_NAME_ANY, 0,
	0, -1,
    };

    /*
     * Turn on per-peer debugging.
     */
    debug_peer_init();

    /*
     * SASL initialization.
     */
    if (var_smtp_sasl_enable)
#ifdef USE_SASL_AUTH
	smtp_sasl_initialize();
#else
	msg_warn("%s is true, but SASL support is not compiled in",
		 VAR_LMTP_SMTP(SASL_ENABLE));
#endif

    if (*var_smtp_tls_level != 0)
#ifdef USE_TLS
	switch (tls_level_lookup(var_smtp_tls_level)) {
	case TLS_LEV_SECURE:
	case TLS_LEV_VERIFY:
	case TLS_LEV_DANE_ONLY:
	case TLS_LEV_FPRINT:
	case TLS_LEV_ENCRYPT:
	    var_smtp_use_tls = var_smtp_enforce_tls = 1;
	    break;
	case TLS_LEV_DANE:
	case TLS_LEV_MAY:
	    var_smtp_use_tls = 1;
	    var_smtp_enforce_tls = 0;
	    break;
	case TLS_LEV_NONE:
	    var_smtp_use_tls = var_smtp_enforce_tls = 0;
	    break;
	default:
	    /* tls_level_lookup() logs no warning. */
	    /* session_tls_init() assumes that var_smtp_tls_level is sane. */
	    msg_fatal("Invalid TLS level \"%s\"", var_smtp_tls_level);
	}
#endif
    use_tls = (var_smtp_use_tls || var_smtp_enforce_tls);

    /*
     * Initialize the TLS data before entering the chroot jail
     */
    if (use_tls || var_smtp_tls_per_site[0] || var_smtp_tls_policy[0]) {
#ifdef USE_TLS
	TLS_CLIENT_INIT_PROPS props;

	tls_pre_jail_init(TLS_ROLE_CLIENT);

	/*
	 * We get stronger type safety and a cleaner interface by combining
	 * the various parameters into a single tls_client_props structure.
	 * 
	 * Large parameter lists are error-prone, so we emulate a language
	 * feature that C does not have natively: named parameter lists.
	 * 
	 * With tlsproxy(8) turned on, this is still needed for DANE-related
	 * initializations.
	 */
	smtp_tls_ctx =
	    TLS_CLIENT_INIT(&props,
			    log_param = VAR_LMTP_SMTP(TLS_LOGLEVEL),
			    log_level = var_smtp_tls_loglevel,
			    verifydepth = var_smtp_tls_scert_vd,
			    cache_type = LMTP_SMTP_SUFFIX(TLS_MGR_SCACHE),
			    chain_files = var_smtp_tls_chain_files,
			    cert_file = var_smtp_tls_cert_file,
			    key_file = var_smtp_tls_key_file,
			    dcert_file = var_smtp_tls_dcert_file,
			    dkey_file = var_smtp_tls_dkey_file,
			    eccert_file = var_smtp_tls_eccert_file,
			    eckey_file = var_smtp_tls_eckey_file,
			    CAfile = var_smtp_tls_CAfile,
			    CApath = var_smtp_tls_CApath,
			    mdalg = var_smtp_tls_fpt_dgst);
	smtp_tls_list_init();
	tls_dane_loglevel(VAR_LMTP_SMTP(TLS_LOGLEVEL), var_smtp_tls_loglevel);
#else
	msg_warn("TLS has been selected, but TLS support is not compiled in");
#endif
    }

    /*
     * Flush client.
     */
    flush_init();

    /*
     * Session cache domain list.
     */
    if (*var_smtp_cache_dest)
	smtp_cache_dest = string_list_init(VAR_SMTP_CACHE_DEST,
					   MATCH_FLAG_RETURN,
					   var_smtp_cache_dest);

    /*
     * EHLO keyword filter.
     */
    if (*var_smtp_ehlo_dis_maps)
	smtp_ehlo_dis_maps = maps_create(VAR_LMTP_SMTP(EHLO_DIS_MAPS),
					 var_smtp_ehlo_dis_maps,
					 DICT_FLAG_LOCK);

    /*
     * PIX bug workarounds.
     */
    if (*var_smtp_pix_bug_maps)
	smtp_pix_bug_maps = maps_create(VAR_LMTP_SMTP(PIX_BUG_MAPS),
					var_smtp_pix_bug_maps,
					DICT_FLAG_LOCK);

    /*
     * Generic maps.
     */
    if (*var_prop_extension)
	smtp_ext_prop_mask =
	    ext_prop_mask(VAR_PROP_EXTENSION, var_prop_extension);
    if (*var_smtp_generic_maps)
	smtp_generic_maps =
	    maps_create(VAR_LMTP_SMTP(GENERIC_MAPS), var_smtp_generic_maps,
			DICT_FLAG_LOCK | DICT_FLAG_FOLD_FIX
			| DICT_FLAG_UTF8_REQUEST);

    /*
     * Header/body checks.
     */
    smtp_header_checks = hbc_header_checks_create(
			       VAR_LMTP_SMTP(HEAD_CHKS), var_smtp_head_chks,
			       VAR_LMTP_SMTP(MIME_CHKS), var_smtp_mime_chks,
			       VAR_LMTP_SMTP(NEST_CHKS), var_smtp_nest_chks,
						  smtp_hbc_callbacks);
    smtp_body_checks = hbc_body_checks_create(
			       VAR_LMTP_SMTP(BODY_CHKS), var_smtp_body_chks,
					      smtp_hbc_callbacks);

    /*
     * Server reply filter.
     */
    if (*var_smtp_resp_filter)
	smtp_chat_resp_filter =
	    dict_open(var_smtp_resp_filter, O_RDONLY,
		      DICT_FLAG_LOCK | DICT_FLAG_FOLD_FIX);

    /*
     * Address family preference.
     */
    if (*var_smtp_addr_pref) {
	smtp_addr_pref = name_code(addr_pref_map, NAME_CODE_FLAG_NONE,
				   var_smtp_addr_pref);
	if (smtp_addr_pref < 0)
	    msg_fatal("bad %s value: %s", VAR_LMTP_SMTP(ADDR_PREF),
		      var_smtp_addr_pref);
    }

    /*
     * DNS reply filter.
     */
    if (*var_smtp_dns_re_filter)
	dns_rr_filter_compile(VAR_LMTP_SMTP(DNS_RE_FILTER),
			      var_smtp_dns_re_filter);
}

/* pre_accept - see if tables have changed */

static void pre_accept(char *unused_name, char **unused_argv)
{
    const char *table;

    if ((table = dict_changed_name()) != 0) {
	msg_info("table %s has changed -- restarting", table);
	exit(0);
    }
}

MAIL_VERSION_STAMP_DECLARE;

/* main - pass control to the single-threaded skeleton */

int     main(int argc, char **argv)
{
    char   *sane_procname;

#include "smtp_params.c"
#include "lmtp_params.c"

    /*
     * Fingerprint executables and core dumps.
     */
    MAIL_VERSION_STAMP_ALLOCATE;

    /*
     * XXX At this point, var_procname etc. are not initialized.
     * 
     * The process name, "smtp" or "lmtp", determines the protocol, the DSN
     * server reply type, SASL service information lookup, and more. Prepare
     * for the possibility there may be another personality.
     */
    sane_procname = sane_basename((VSTRING *) 0, argv[0]);
    if (strcmp(sane_procname, "smtp") == 0)
	smtp_mode = 1;
    else if (strcmp(sane_procname, "lmtp") == 0)
	smtp_mode = 0;
    else
	/* TODO: logging is not initialized. */
	msg_fatal("unexpected process name \"%s\" - "
		  "specify \"smtp\" or \"lmtp\"", var_procname);

    /*
     * Initialize with the LMTP or SMTP parameter name space.
     */
    single_server_main(argc, argv, smtp_service,
		       CA_MAIL_SERVER_TIME_TABLE(smtp_mode ?
					 smtp_time_table : lmtp_time_table),
		       CA_MAIL_SERVER_INT_TABLE(smtp_mode ?
					   smtp_int_table : lmtp_int_table),
		       CA_MAIL_SERVER_STR_TABLE(smtp_mode ?
					   smtp_str_table : lmtp_str_table),
		       CA_MAIL_SERVER_BOOL_TABLE(smtp_mode ?
					 smtp_bool_table : lmtp_bool_table),
		       CA_MAIL_SERVER_NBOOL_TABLE(smtp_mode ?
				       smtp_nbool_table : lmtp_nbool_table),
		       CA_MAIL_SERVER_PRE_INIT(pre_init),
		       CA_MAIL_SERVER_POST_INIT(post_init),
		       CA_MAIL_SERVER_PRE_ACCEPT(pre_accept),
		       CA_MAIL_SERVER_BOUNCE_INIT(VAR_SMTP_DSN_FILTER,
						  &var_smtp_dsn_filter),
		       0);
}
