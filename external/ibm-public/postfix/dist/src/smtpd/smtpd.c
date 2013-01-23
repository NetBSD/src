/*	$NetBSD: smtpd.c,v 1.7.2.2 2013/01/23 00:05:12 yamt Exp $	*/

/*++
/* NAME
/*	smtpd 8
/* SUMMARY
/*	Postfix SMTP server
/* SYNOPSIS
/*	\fBsmtpd\fR [generic Postfix daemon options]
/*
/*	\fBsendmail -bs\fR
/* DESCRIPTION
/*	The SMTP server accepts network connection requests
/*	and performs zero or more SMTP transactions per connection.
/*	Each received message is piped through the \fBcleanup\fR(8)
/*	daemon, and is placed into the \fBincoming\fR queue as one
/*	single queue file.  For this mode of operation, the program
/*	expects to be run from the \fBmaster\fR(8) process manager.
/*
/*	Alternatively, the SMTP server be can run in stand-alone
/*	mode; this is traditionally obtained with "\fBsendmail
/*	-bs\fR".  When the SMTP server runs stand-alone with non
/*	$\fBmail_owner\fR privileges, it receives mail even while
/*	the mail system is not running, deposits messages directly
/*	into the \fBmaildrop\fR queue, and disables the SMTP server's
/*	access policies. As of Postfix version 2.3, the SMTP server
/*	refuses to receive mail from the network when it runs with
/*	non $\fBmail_owner\fR privileges.
/*
/*	The SMTP server implements a variety of policies for connection
/*	requests, and for parameters given to \fBHELO, ETRN, MAIL FROM, VRFY\fR
/*	and \fBRCPT TO\fR commands. They are detailed below and in the
/*	\fBmain.cf\fR configuration file.
/* SECURITY
/* .ad
/* .fi
/*	The SMTP server is moderately security-sensitive. It talks to SMTP
/*	clients and to DNS servers on the network. The SMTP server can be
/*	run chrooted at fixed low privilege.
/* STANDARDS
/*	RFC 821 (SMTP protocol)
/*	RFC 1123 (Host requirements)
/*	RFC 1652 (8bit-MIME transport)
/*	RFC 1869 (SMTP service extensions)
/*	RFC 1870 (Message size declaration)
/*	RFC 1985 (ETRN command)
/*	RFC 2034 (SMTP enhanced status codes)
/*	RFC 2554 (AUTH command)
/*	RFC 2821 (SMTP protocol)
/*	RFC 2920 (SMTP pipelining)
/*	RFC 3207 (STARTTLS command)
/*	RFC 3461 (SMTP DSN extension)
/*	RFC 3463 (Enhanced status codes)
/*	RFC 3848 (ESMTP transmission types)
/*	RFC 4409 (Message submission)
/*	RFC 4954 (AUTH command)
/* DIAGNOSTICS
/*	Problems and transactions are logged to \fBsyslogd\fR(8).
/*
/*	Depending on the setting of the \fBnotify_classes\fR parameter,
/*	the postmaster is notified of bounces, protocol problems,
/*	policy violations, and of other trouble.
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	Changes to \fBmain.cf\fR are picked up automatically, as \fBsmtpd\fR(8)
/*	processes run for only a limited amount of time. Use the command
/*	"\fBpostfix reload\fR" to speed up a change.
/*
/*	The text below provides only a parameter summary. See
/*	\fBpostconf\fR(5) for more details including examples.
/* COMPATIBILITY CONTROLS
/* .ad
/* .fi
/*	The following parameters work around implementation errors in other
/*	software, and/or allow you to override standards in order to prevent
/*	undesirable use.
/* .ad
/* .fi
/* .IP "\fBbroken_sasl_auth_clients (no)\fR"
/*	Enable inter-operability with remote SMTP clients that implement an obsolete
/*	version of the AUTH command (RFC 4954).
/* .IP "\fBdisable_vrfy_command (no)\fR"
/*	Disable the SMTP VRFY command.
/* .IP "\fBsmtpd_noop_commands (empty)\fR"
/*	List of commands that the Postfix SMTP server replies to with "250
/*	Ok", without doing any syntax checks and without changing state.
/* .IP "\fBstrict_rfc821_envelopes (no)\fR"
/*	Require that addresses received in SMTP MAIL FROM and RCPT TO
/*	commands are enclosed with <>, and that those addresses do
/*	not contain RFC 822 style comments or phrases.
/* .PP
/*	Available in Postfix version 2.1 and later:
/* .IP "\fBresolve_null_domain (no)\fR"
/*	Resolve an address that ends in the "@" null domain as if the
/*	local hostname were specified, instead of rejecting the address as
/*	invalid.
/* .IP "\fBsmtpd_reject_unlisted_sender (no)\fR"
/*	Request that the Postfix SMTP server rejects mail from unknown
/*	sender addresses, even when no explicit reject_unlisted_sender
/*	access restriction is specified.
/* .IP "\fBsmtpd_sasl_exceptions_networks (empty)\fR"
/*	What remote SMTP clients the Postfix SMTP server will not offer
/*	AUTH support to.
/* .PP
/*	Available in Postfix version 2.2 and later:
/* .IP "\fBsmtpd_discard_ehlo_keyword_address_maps (empty)\fR"
/*	Lookup tables, indexed by the remote SMTP client address, with
/*	case insensitive lists of EHLO keywords (pipelining, starttls, auth,
/*	etc.) that the Postfix SMTP server will not send in the EHLO response
/*	to a
/*	remote SMTP client.
/* .IP "\fBsmtpd_discard_ehlo_keywords (empty)\fR"
/*	A case insensitive list of EHLO keywords (pipelining, starttls,
/*	auth, etc.) that the Postfix SMTP server will not send in the EHLO
/*	response
/*	to a remote SMTP client.
/* .IP "\fBsmtpd_delay_open_until_valid_rcpt (yes)\fR"
/*	Postpone the start of an SMTP mail transaction until a valid
/*	RCPT TO command is received.
/* .PP
/*	Available in Postfix version 2.3 and later:
/* .IP "\fBsmtpd_tls_always_issue_session_ids (yes)\fR"
/*	Force the Postfix SMTP server to issue a TLS session id, even
/*	when TLS session caching is turned off (smtpd_tls_session_cache_database
/*	is empty).
/* .PP
/*	Available in Postfix version 2.6 and later:
/* .IP "\fBtcp_windowsize (0)\fR"
/*	An optional workaround for routers that break TCP window scaling.
/* .PP
/*	Available in Postfix version 2.7 and later:
/* .IP "\fBsmtpd_command_filter (empty)\fR"
/*	A mechanism to transform commands from remote SMTP clients.
/* .PP
/*	Available in Postfix version 2.9 and later:
/* .IP "\fBsmtpd_per_record_deadline (normal: no, overload: yes)\fR"
/*	Change the behavior of the smtpd_timeout time limit, from a
/*	time limit per read or write system call, to a time limit to send
/*	or receive a complete record (an SMTP command line, SMTP response
/*	line, SMTP message content line, or TLS protocol message).
/* ADDRESS REWRITING CONTROLS
/* .ad
/* .fi
/*	See the ADDRESS_REWRITING_README document for a detailed
/*	discussion of Postfix address rewriting.
/* .IP "\fBreceive_override_options (empty)\fR"
/*	Enable or disable recipient validation, built-in content
/*	filtering, or address mapping.
/* .PP
/*	Available in Postfix version 2.2 and later:
/* .IP "\fBlocal_header_rewrite_clients (permit_inet_interfaces)\fR"
/*	Rewrite message header addresses in mail from these clients and
/*	update incomplete addresses with the domain name in $myorigin or
/*	$mydomain; either don't rewrite message headers from other clients
/*	at all, or rewrite message headers and update incomplete addresses
/*	with the domain specified in the remote_header_rewrite_domain
/*	parameter.
/* AFTER QUEUE EXTERNAL CONTENT INSPECTION CONTROLS
/* .ad
/* .fi
/*	As of version 1.0, Postfix can be configured to send new mail to
/*	an external content filter AFTER the mail is queued. This content
/*	filter is expected to inject mail back into a (Postfix or other)
/*	MTA for further delivery. See the FILTER_README document for details.
/* .IP "\fBcontent_filter (empty)\fR"
/*	After the message is queued, send the entire message to the
/*	specified \fItransport:destination\fR.
/* BEFORE QUEUE EXTERNAL CONTENT INSPECTION CONTROLS
/* .ad
/* .fi
/*	As of version 2.1, the Postfix SMTP server can be configured
/*	to send incoming mail to a real-time SMTP-based content filter
/*	BEFORE mail is queued.  This content filter is expected to inject
/*	mail back into Postfix.  See the SMTPD_PROXY_README document for
/*	details on how to configure and operate this feature.
/* .IP "\fBsmtpd_proxy_filter (empty)\fR"
/*	The hostname and TCP port of the mail filtering proxy server.
/* .IP "\fBsmtpd_proxy_ehlo ($myhostname)\fR"
/*	How the Postfix SMTP server announces itself to the proxy filter.
/* .IP "\fBsmtpd_proxy_options (empty)\fR"
/*	List of options that control how the Postfix SMTP server
/*	communicates with a before-queue content filter.
/* .IP "\fBsmtpd_proxy_timeout (100s)\fR"
/*	The time limit for connecting to a proxy filter and for sending or
/*	receiving information.
/* BEFORE QUEUE MILTER CONTROLS
/* .ad
/* .fi
/*	As of version 2.3, Postfix supports the Sendmail version 8
/*	Milter (mail filter) protocol. These content filters run
/*	outside Postfix. They can inspect the SMTP command stream
/*	and the message content, and can request modifications before
/*	mail is queued. For details see the MILTER_README document.
/* .IP "\fBsmtpd_milters (empty)\fR"
/*	A list of Milter (mail filter) applications for new mail that
/*	arrives via the Postfix \fBsmtpd\fR(8) server.
/* .IP "\fBmilter_protocol (6)\fR"
/*	The mail filter protocol version and optional protocol extensions
/*	for communication with a Milter application; prior to Postfix 2.6
/*	the default protocol is 2.
/* .IP "\fBmilter_default_action (tempfail)\fR"
/*	The default action when a Milter (mail filter) application is
/*	unavailable or mis-configured.
/* .IP "\fBmilter_macro_daemon_name ($myhostname)\fR"
/*	The {daemon_name} macro value for Milter (mail filter) applications.
/* .IP "\fBmilter_macro_v ($mail_name $mail_version)\fR"
/*	The {v} macro value for Milter (mail filter) applications.
/* .IP "\fBmilter_connect_timeout (30s)\fR"
/*	The time limit for connecting to a Milter (mail filter)
/*	application, and for negotiating protocol options.
/* .IP "\fBmilter_command_timeout (30s)\fR"
/*	The time limit for sending an SMTP command to a Milter (mail
/*	filter) application, and for receiving the response.
/* .IP "\fBmilter_content_timeout (300s)\fR"
/*	The time limit for sending message content to a Milter (mail
/*	filter) application, and for receiving the response.
/* .IP "\fBmilter_connect_macros (see 'postconf -d' output)\fR"
/*	The macros that are sent to Milter (mail filter) applications
/*	after completion of an SMTP connection.
/* .IP "\fBmilter_helo_macros (see 'postconf -d' output)\fR"
/*	The macros that are sent to Milter (mail filter) applications
/*	after the SMTP HELO or EHLO command.
/* .IP "\fBmilter_mail_macros (see 'postconf -d' output)\fR"
/*	The macros that are sent to Milter (mail filter) applications
/*	after the SMTP MAIL FROM command.
/* .IP "\fBmilter_rcpt_macros (see 'postconf -d' output)\fR"
/*	The macros that are sent to Milter (mail filter) applications
/*	after the SMTP RCPT TO command.
/* .IP "\fBmilter_data_macros (see 'postconf -d' output)\fR"
/*	The macros that are sent to version 4 or higher Milter (mail
/*	filter) applications after the SMTP DATA command.
/* .IP "\fBmilter_unknown_command_macros (see 'postconf -d' output)\fR"
/*	The macros that are sent to version 3 or higher Milter (mail
/*	filter) applications after an unknown SMTP command.
/* .IP "\fBmilter_end_of_header_macros (see 'postconf -d' output)\fR"
/*	The macros that are sent to Milter (mail filter) applications
/*	after the end of the message header.
/* .IP "\fBmilter_end_of_data_macros (see 'postconf -d' output)\fR"
/*	The macros that are sent to Milter (mail filter) applications
/*	after the message end-of-data.
/* GENERAL CONTENT INSPECTION CONTROLS
/* .ad
/* .fi
/*	The following parameters are applicable for both built-in
/*	and external content filters.
/* .PP
/*	Available in Postfix version 2.1 and later:
/* .IP "\fBreceive_override_options (empty)\fR"
/*	Enable or disable recipient validation, built-in content
/*	filtering, or address mapping.
/* EXTERNAL CONTENT INSPECTION CONTROLS
/* .ad
/* .fi
/*	The following parameters are applicable for both before-queue
/*	and after-queue content filtering.
/* .PP
/*	Available in Postfix version 2.1 and later:
/* .IP "\fBsmtpd_authorized_xforward_hosts (empty)\fR"
/*	What remote SMTP clients are allowed to use the XFORWARD feature.
/* SASL AUTHENTICATION CONTROLS
/* .ad
/* .fi
/*	Postfix SASL support (RFC 4954) can be used to authenticate remote
/*	SMTP clients to the Postfix SMTP server, and to authenticate the
/*	Postfix SMTP client to a remote SMTP server.
/*	See the SASL_README document for details.
/* .IP "\fBbroken_sasl_auth_clients (no)\fR"
/*	Enable inter-operability with remote SMTP clients that implement an obsolete
/*	version of the AUTH command (RFC 4954).
/* .IP "\fBsmtpd_sasl_auth_enable (no)\fR"
/*	Enable SASL authentication in the Postfix SMTP server.
/* .IP "\fBsmtpd_sasl_local_domain (empty)\fR"
/*	The name of the Postfix SMTP server's local SASL authentication
/*	realm.
/* .IP "\fBsmtpd_sasl_security_options (noanonymous)\fR"
/*	Postfix SMTP server SASL security options; as of Postfix 2.3
/*	the list of available
/*	features depends on the SASL server implementation that is selected
/*	with \fBsmtpd_sasl_type\fR.
/* .IP "\fBsmtpd_sender_login_maps (empty)\fR"
/*	Optional lookup table with the SASL login names that own sender
/*	(MAIL FROM) addresses.
/* .PP
/*	Available in Postfix version 2.1 and later:
/* .IP "\fBsmtpd_sasl_exceptions_networks (empty)\fR"
/*	What remote SMTP clients the Postfix SMTP server will not offer
/*	AUTH support to.
/* .PP
/*	Available in Postfix version 2.1 and 2.2:
/* .IP "\fBsmtpd_sasl_application_name (smtpd)\fR"
/*	The application name that the Postfix SMTP server uses for SASL
/*	server initialization.
/* .PP
/*	Available in Postfix version 2.3 and later:
/* .IP "\fBsmtpd_sasl_authenticated_header (no)\fR"
/*	Report the SASL authenticated user name in the \fBsmtpd\fR(8) Received
/*	message header.
/* .IP "\fBsmtpd_sasl_path (smtpd)\fR"
/*	Implementation-specific information that the Postfix SMTP server
/*	passes through to
/*	the SASL plug-in implementation that is selected with
/*	\fBsmtpd_sasl_type\fR.
/* .IP "\fBsmtpd_sasl_type (cyrus)\fR"
/*	The SASL plug-in type that the Postfix SMTP server should use
/*	for authentication.
/* .PP
/*	Available in Postfix version 2.5 and later:
/* .IP "\fBcyrus_sasl_config_path (empty)\fR"
/*	Search path for Cyrus SASL application configuration files,
/*	currently used only to locate the $smtpd_sasl_path.conf file.
/* STARTTLS SUPPORT CONTROLS
/* .ad
/* .fi
/*	Detailed information about STARTTLS configuration may be
/*	found in the TLS_README document.
/* .IP "\fBsmtpd_tls_security_level (empty)\fR"
/*	The SMTP TLS security level for the Postfix SMTP server; when
/*	a non-empty value is specified, this overrides the obsolete parameters
/*	smtpd_use_tls and smtpd_enforce_tls.
/* .IP "\fBsmtpd_sasl_tls_security_options ($smtpd_sasl_security_options)\fR"
/*	The SASL authentication security options that the Postfix SMTP
/*	server uses for TLS encrypted SMTP sessions.
/* .IP "\fBsmtpd_starttls_timeout (see 'postconf -d' output)\fR"
/*	The time limit for Postfix SMTP server write and read operations
/*	during TLS startup and shutdown handshake procedures.
/* .IP "\fBsmtpd_tls_CAfile (empty)\fR"
/*	A file containing (PEM format) CA certificates of root CAs trusted
/*	to sign either remote SMTP client certificates or intermediate CA
/*	certificates.
/* .IP "\fBsmtpd_tls_CApath (empty)\fR"
/*	A directory containing (PEM format) CA certificates of root CAs
/*	trusted to sign either remote SMTP client certificates or intermediate CA
/*	certificates.
/* .IP "\fBsmtpd_tls_always_issue_session_ids (yes)\fR"
/*	Force the Postfix SMTP server to issue a TLS session id, even
/*	when TLS session caching is turned off (smtpd_tls_session_cache_database
/*	is empty).
/* .IP "\fBsmtpd_tls_ask_ccert (no)\fR"
/*	Ask a remote SMTP client for a client certificate.
/* .IP "\fBsmtpd_tls_auth_only (no)\fR"
/*	When TLS encryption is optional in the Postfix SMTP server, do
/*	not announce or accept SASL authentication over unencrypted
/*	connections.
/* .IP "\fBsmtpd_tls_ccert_verifydepth (9)\fR"
/*	The verification depth for remote SMTP client certificates.
/* .IP "\fBsmtpd_tls_cert_file (empty)\fR"
/*	File with the Postfix SMTP server RSA certificate in PEM format.
/* .IP "\fBsmtpd_tls_exclude_ciphers (empty)\fR"
/*	List of ciphers or cipher types to exclude from the SMTP server
/*	cipher list at all TLS security levels.
/* .IP "\fBsmtpd_tls_dcert_file (empty)\fR"
/*	File with the Postfix SMTP server DSA certificate in PEM format.
/* .IP "\fBsmtpd_tls_dh1024_param_file (empty)\fR"
/*	File with DH parameters that the Postfix SMTP server should
/*	use with EDH ciphers.
/* .IP "\fBsmtpd_tls_dh512_param_file (empty)\fR"
/*	File with DH parameters that the Postfix SMTP server should
/*	use with EDH ciphers.
/* .IP "\fBsmtpd_tls_dkey_file ($smtpd_tls_dcert_file)\fR"
/*	File with the Postfix SMTP server DSA private key in PEM format.
/* .IP "\fBsmtpd_tls_key_file ($smtpd_tls_cert_file)\fR"
/*	File with the Postfix SMTP server RSA private key in PEM format.
/* .IP "\fBsmtpd_tls_loglevel (0)\fR"
/*	Enable additional Postfix SMTP server logging of TLS activity.
/* .IP "\fBsmtpd_tls_mandatory_ciphers (medium)\fR"
/*	The minimum TLS cipher grade that the Postfix SMTP server will
/*	use with mandatory TLS encryption.
/* .IP "\fBsmtpd_tls_mandatory_exclude_ciphers (empty)\fR"
/*	Additional list of ciphers or cipher types to exclude from the
/*	Postfix SMTP server cipher list at mandatory TLS security levels.
/* .IP "\fBsmtpd_tls_mandatory_protocols (!SSLv2)\fR"
/*	The SSL/TLS protocols accepted by the Postfix SMTP server with
/*	mandatory TLS encryption.
/* .IP "\fBsmtpd_tls_received_header (no)\fR"
/*	Request that the Postfix SMTP server produces Received:  message
/*	headers that include information about the protocol and cipher used,
/*	as well as the remote SMTP client CommonName and client certificate issuer
/*	CommonName.
/* .IP "\fBsmtpd_tls_req_ccert (no)\fR"
/*	With mandatory TLS encryption, require a trusted remote SMTP client
/*	certificate in order to allow TLS connections to proceed.
/* .IP "\fBsmtpd_tls_session_cache_database (empty)\fR"
/*	Name of the file containing the optional Postfix SMTP server
/*	TLS session cache.
/* .IP "\fBsmtpd_tls_session_cache_timeout (3600s)\fR"
/*	The expiration time of Postfix SMTP server TLS session cache
/*	information.
/* .IP "\fBsmtpd_tls_wrappermode (no)\fR"
/*	Run the Postfix SMTP server in the non-standard "wrapper" mode,
/*	instead of using the STARTTLS command.
/* .IP "\fBtls_daemon_random_bytes (32)\fR"
/*	The number of pseudo-random bytes that an \fBsmtp\fR(8) or \fBsmtpd\fR(8)
/*	process requests from the \fBtlsmgr\fR(8) server in order to seed its
/*	internal pseudo random number generator (PRNG).
/* .IP "\fBtls_high_cipherlist (ALL:!EXPORT:!LOW:!MEDIUM:+RC4:@STRENGTH)\fR"
/*	The OpenSSL cipherlist for "HIGH" grade ciphers.
/* .IP "\fBtls_medium_cipherlist (ALL:!EXPORT:!LOW:+RC4:@STRENGTH)\fR"
/*	The OpenSSL cipherlist for "MEDIUM" or higher grade ciphers.
/* .IP "\fBtls_low_cipherlist (ALL:!EXPORT:+RC4:@STRENGTH)\fR"
/*	The OpenSSL cipherlist for "LOW" or higher grade ciphers.
/* .IP "\fBtls_export_cipherlist (ALL:+RC4:@STRENGTH)\fR"
/*	The OpenSSL cipherlist for "EXPORT" or higher grade ciphers.
/* .IP "\fBtls_null_cipherlist (eNULL:!aNULL)\fR"
/*	The OpenSSL cipherlist for "NULL" grade ciphers that provide
/*	authentication without encryption.
/* .PP
/*	Available in Postfix version 2.5 and later:
/* .IP "\fBsmtpd_tls_fingerprint_digest (md5)\fR"
/*	The message digest algorithm to construct remote SMTP
/*	client-certificate
/*	fingerprints or public key fingerprints (Postfix 2.9 and later)
/*	for \fBcheck_ccert_access\fR and \fBpermit_tls_clientcerts\fR.
/* .PP
/*	Available in Postfix version 2.6 and later:
/* .IP "\fBsmtpd_tls_protocols (empty)\fR"
/*	List of TLS protocols that the Postfix SMTP server will exclude
/*	or include with opportunistic TLS encryption.
/* .IP "\fBsmtpd_tls_ciphers (export)\fR"
/*	The minimum TLS cipher grade that the Postfix SMTP server
/*	will use with opportunistic TLS encryption.
/* .IP "\fBsmtpd_tls_eccert_file (empty)\fR"
/*	File with the Postfix SMTP server ECDSA certificate in PEM format.
/* .IP "\fBsmtpd_tls_eckey_file ($smtpd_tls_eccert_file)\fR"
/*	File with the Postfix SMTP server ECDSA private key in PEM format.
/* .IP "\fBsmtpd_tls_eecdh_grade (see 'postconf -d' output)\fR"
/*	The Postfix SMTP server security grade for ephemeral elliptic-curve
/*	Diffie-Hellman (EECDH) key exchange.
/* .IP "\fBtls_eecdh_strong_curve (prime256v1)\fR"
/*	The elliptic curve used by the Postfix SMTP server for sensibly
/*	strong
/*	ephemeral ECDH key exchange.
/* .IP "\fBtls_eecdh_ultra_curve (secp384r1)\fR"
/*	The elliptic curve used by the Postfix SMTP server for maximally
/*	strong
/*	ephemeral ECDH key exchange.
/* .PP
/*	Available in Postfix version 2.8 and later:
/* .IP "\fBtls_preempt_cipherlist (no)\fR"
/*	With SSLv3 and later, use the Postfix SMTP server's cipher
/*	preference order instead of the remote client's cipher preference
/*	order.
/* .IP "\fBtls_disable_workarounds (see 'postconf -d' output)\fR"
/*	List or bit-mask of OpenSSL bug work-arounds to disable.
/* OBSOLETE STARTTLS CONTROLS
/* .ad
/* .fi
/*	The following configuration parameters exist for compatibility
/*	with Postfix versions before 2.3. Support for these will
/*	be removed in a future release.
/* .IP "\fBsmtpd_use_tls (no)\fR"
/*	Opportunistic TLS: announce STARTTLS support to remote SMTP clients,
/*	but do not require that clients use TLS encryption.
/* .IP "\fBsmtpd_enforce_tls (no)\fR"
/*	Mandatory TLS: announce STARTTLS support to remote SMTP clients,
/*	and require that clients use TLS encryption.
/* .IP "\fBsmtpd_tls_cipherlist (empty)\fR"
/*	Obsolete Postfix < 2.3 control for the Postfix SMTP server TLS
/*	cipher list.
/* VERP SUPPORT CONTROLS
/* .ad
/* .fi
/*	With VERP style delivery, each recipient of a message receives a
/*	customized copy of the message with his/her own recipient address
/*	encoded in the envelope sender address.  The VERP_README file
/*	describes configuration and operation details of Postfix support
/*	for variable envelope return path addresses.  VERP style delivery
/*	is requested with the SMTP XVERP command or with the "sendmail
/*	-V" command-line option and is available in Postfix version 1.1
/*	and later.
/* .IP "\fBdefault_verp_delimiters (+=)\fR"
/*	The two default VERP delimiter characters.
/* .IP "\fBverp_delimiter_filter (-=+)\fR"
/*	The characters Postfix accepts as VERP delimiter characters on the
/*	Postfix \fBsendmail\fR(1) command line and in SMTP commands.
/* .PP
/*	Available in Postfix version 1.1 and 2.0:
/* .IP "\fBauthorized_verp_clients ($mynetworks)\fR"
/*	What remote SMTP clients are allowed to specify the XVERP command.
/* .PP
/*	Available in Postfix version 2.1 and later:
/* .IP "\fBsmtpd_authorized_verp_clients ($authorized_verp_clients)\fR"
/*	What remote SMTP clients are allowed to specify the XVERP command.
/* TROUBLE SHOOTING CONTROLS
/* .ad
/* .fi
/*	The DEBUG_README document describes how to debug parts of the
/*	Postfix mail system. The methods vary from making the software log
/*	a lot of detail, to running some daemon processes under control of
/*	a call tracer or debugger.
/* .IP "\fBdebug_peer_level (2)\fR"
/*	The increment in verbose logging level when a remote client or
/*	server matches a pattern in the debug_peer_list parameter.
/* .IP "\fBdebug_peer_list (empty)\fR"
/*	Optional list of remote client or server hostname or network
/*	address patterns that cause the verbose logging level to increase
/*	by the amount specified in $debug_peer_level.
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
/* .IP "\fBsmtpd_reject_footer (empty)\fR"
/*	Optional information that is appended after each Postfix SMTP
/*	server
/*	4XX or 5XX response.
/* .IP "\fBsoft_bounce (no)\fR"
/*	Safety net to keep mail queued that would otherwise be returned to
/*	the sender.
/* .PP
/*	Available in Postfix version 2.1 and later:
/* .IP "\fBsmtpd_authorized_xclient_hosts (empty)\fR"
/*	What remote SMTP clients are allowed to use the XCLIENT feature.
/* KNOWN VERSUS UNKNOWN RECIPIENT CONTROLS
/* .ad
/* .fi
/*	As of Postfix version 2.0, the SMTP server rejects mail for
/*	unknown recipients. This prevents the mail queue from clogging up
/*	with undeliverable MAILER-DAEMON messages. Additional information
/*	on this topic is in the LOCAL_RECIPIENT_README and ADDRESS_CLASS_README
/*	documents.
/* .IP "\fBshow_user_unknown_table_name (yes)\fR"
/*	Display the name of the recipient table in the "User unknown"
/*	responses.
/* .IP "\fBcanonical_maps (empty)\fR"
/*	Optional address mapping lookup tables for message headers and
/*	envelopes.
/* .IP "\fBrecipient_canonical_maps (empty)\fR"
/*	Optional address mapping lookup tables for envelope and header
/*	recipient addresses.
/* .PP
/*	Parameters concerning known/unknown local recipients:
/* .IP "\fBmydestination ($myhostname, localhost.$mydomain, localhost)\fR"
/*	The list of domains that are delivered via the $local_transport
/*	mail delivery transport.
/* .IP "\fBinet_interfaces (all)\fR"
/*	The network interface addresses that this mail system receives
/*	mail on.
/* .IP "\fBproxy_interfaces (empty)\fR"
/*	The network interface addresses that this mail system receives mail
/*	on by way of a proxy or network address translation unit.
/* .IP "\fBinet_protocols (all)\fR"
/*	The Internet protocols Postfix will attempt to use when making
/*	or accepting connections.
/* .IP "\fBlocal_recipient_maps (proxy:unix:passwd.byname $alias_maps)\fR"
/*	Lookup tables with all names or addresses of local recipients:
/*	a recipient address is local when its domain matches $mydestination,
/*	$inet_interfaces or $proxy_interfaces.
/* .IP "\fBunknown_local_recipient_reject_code (550)\fR"
/*	The numerical Postfix SMTP server response code when a recipient
/*	address is local, and $local_recipient_maps specifies a list of
/*	lookup tables that does not match the recipient.
/* .PP
/*	Parameters concerning known/unknown recipients of relay destinations:
/* .IP "\fBrelay_domains ($mydestination)\fR"
/*	What destination domains (and subdomains thereof) this system
/*	will relay mail to.
/* .IP "\fBrelay_recipient_maps (empty)\fR"
/*	Optional lookup tables with all valid addresses in the domains
/*	that match $relay_domains.
/* .IP "\fBunknown_relay_recipient_reject_code (550)\fR"
/*	The numerical Postfix SMTP server reply code when a recipient
/*	address matches $relay_domains, and relay_recipient_maps specifies
/*	a list of lookup tables that does not match the recipient address.
/* .PP
/*	Parameters concerning known/unknown recipients in virtual alias
/*	domains:
/* .IP "\fBvirtual_alias_domains ($virtual_alias_maps)\fR"
/*	Postfix is final destination for the specified list of virtual
/*	alias domains, that is, domains for which all addresses are aliased
/*	to addresses in other local or remote domains.
/* .IP "\fBvirtual_alias_maps ($virtual_maps)\fR"
/*	Optional lookup tables that alias specific mail addresses or domains
/*	to other local or remote address.
/* .IP "\fBunknown_virtual_alias_reject_code (550)\fR"
/*	The Postfix SMTP server reply code when a recipient address matches
/*	$virtual_alias_domains, and $virtual_alias_maps specifies a list
/*	of lookup tables that does not match the recipient address.
/* .PP
/*	Parameters concerning known/unknown recipients in virtual mailbox
/*	domains:
/* .IP "\fBvirtual_mailbox_domains ($virtual_mailbox_maps)\fR"
/*	Postfix is final destination for the specified list of domains;
/*	mail is delivered via the $virtual_transport mail delivery transport.
/* .IP "\fBvirtual_mailbox_maps (empty)\fR"
/*	Optional lookup tables with all valid addresses in the domains that
/*	match $virtual_mailbox_domains.
/* .IP "\fBunknown_virtual_mailbox_reject_code (550)\fR"
/*	The Postfix SMTP server reply code when a recipient address matches
/*	$virtual_mailbox_domains, and $virtual_mailbox_maps specifies a list
/*	of lookup tables that does not match the recipient address.
/* RESOURCE AND RATE CONTROLS
/* .ad
/* .fi
/*	The following parameters limit resource usage by the SMTP
/*	server and/or control client request rates.
/* .IP "\fBline_length_limit (2048)\fR"
/*	Upon input, long lines are chopped up into pieces of at most
/*	this length; upon delivery, long lines are reconstructed.
/* .IP "\fBqueue_minfree (0)\fR"
/*	The minimal amount of free space in bytes in the queue file system
/*	that is needed to receive mail.
/* .IP "\fBmessage_size_limit (10240000)\fR"
/*	The maximal size in bytes of a message, including envelope information.
/* .IP "\fBsmtpd_recipient_limit (1000)\fR"
/*	The maximal number of recipients that the Postfix SMTP server
/*	accepts per message delivery request.
/* .IP "\fBsmtpd_timeout (normal: 300s, overload: 10s)\fR"
/*	The time limit for sending a Postfix SMTP server response and for
/*	receiving a remote SMTP client request.
/* .IP "\fBsmtpd_history_flush_threshold (100)\fR"
/*	The maximal number of lines in the Postfix SMTP server command history
/*	before it is flushed upon receipt of EHLO, RSET, or end of DATA.
/* .PP
/*	Available in Postfix version 2.3 and later:
/* .IP "\fBsmtpd_peername_lookup (yes)\fR"
/*	Attempt to look up the remote SMTP client hostname, and verify that
/*	the name matches the client IP address.
/* .PP
/*	The per SMTP client connection count and request rate limits are
/*	implemented in co-operation with the \fBanvil\fR(8) service, and
/*	are available in Postfix version 2.2 and later.
/* .IP "\fBsmtpd_client_connection_count_limit (50)\fR"
/*	How many simultaneous connections any client is allowed to
/*	make to this service.
/* .IP "\fBsmtpd_client_connection_rate_limit (0)\fR"
/*	The maximal number of connection attempts any client is allowed to
/*	make to this service per time unit.
/* .IP "\fBsmtpd_client_message_rate_limit (0)\fR"
/*	The maximal number of message delivery requests that any client is
/*	allowed to make to this service per time unit, regardless of whether
/*	or not Postfix actually accepts those messages.
/* .IP "\fBsmtpd_client_recipient_rate_limit (0)\fR"
/*	The maximal number of recipient addresses that any client is allowed
/*	to send to this service per time unit, regardless of whether or not
/*	Postfix actually accepts those recipients.
/* .IP "\fBsmtpd_client_event_limit_exceptions ($mynetworks)\fR"
/*	Clients that are excluded from smtpd_client_*_count/rate_limit
/*	restrictions.
/* .PP
/*	Available in Postfix version 2.3 and later:
/* .IP "\fBsmtpd_client_new_tls_session_rate_limit (0)\fR"
/*	The maximal number of new (i.e., uncached) TLS sessions that a
/*	remote SMTP client is allowed to negotiate with this service per
/*	time unit.
/* .PP
/*	Available in Postfix version 2.9 and later:
/* .IP "\fBsmtpd_per_record_deadline (normal: no, overload: yes)\fR"
/*	Change the behavior of the smtpd_timeout time limit, from a
/*	time limit per read or write system call, to a time limit to send
/*	or receive a complete record (an SMTP command line, SMTP response
/*	line, SMTP message content line, or TLS protocol message).
/* TARPIT CONTROLS
/* .ad
/* .fi
/*	When a remote SMTP client makes errors, the Postfix SMTP server
/*	can insert delays before responding. This can help to slow down
/*	run-away software.  The behavior is controlled by an error counter
/*	that counts the number of errors within an SMTP session that a
/*	client makes without delivering mail.
/* .IP "\fBsmtpd_error_sleep_time (1s)\fR"
/*	With Postfix version 2.1 and later: the SMTP server response delay after
/*	a client has made more than $smtpd_soft_error_limit errors, and
/*	fewer than $smtpd_hard_error_limit errors, without delivering mail.
/* .IP "\fBsmtpd_soft_error_limit (10)\fR"
/*	The number of errors a remote SMTP client is allowed to make without
/*	delivering mail before the Postfix SMTP server slows down all its
/*	responses.
/* .IP "\fBsmtpd_hard_error_limit (normal: 20, overload: 1)\fR"
/*	The maximal number of errors a remote SMTP client is allowed to
/*	make without delivering mail.
/* .IP "\fBsmtpd_junk_command_limit (normal: 100, overload: 1)\fR"
/*	The number of junk commands (NOOP, VRFY, ETRN or RSET) that a remote
/*	SMTP client can send before the Postfix SMTP server starts to
/*	increment the error counter with each junk command.
/* .PP
/*	Available in Postfix version 2.1 and later:
/* .IP "\fBsmtpd_recipient_overshoot_limit (1000)\fR"
/*	The number of recipients that a remote SMTP client can send in
/*	excess of the limit specified with $smtpd_recipient_limit, before
/*	the Postfix SMTP server increments the per-session error count
/*	for each excess recipient.
/* ACCESS POLICY DELEGATION CONTROLS
/* .ad
/* .fi
/*	As of version 2.1, Postfix can be configured to delegate access
/*	policy decisions to an external server that runs outside Postfix.
/*	See the file SMTPD_POLICY_README for more information.
/* .IP "\fBsmtpd_policy_service_max_idle (300s)\fR"
/*	The time after which an idle SMTPD policy service connection is
/*	closed.
/* .IP "\fBsmtpd_policy_service_max_ttl (1000s)\fR"
/*	The time after which an active SMTPD policy service connection is
/*	closed.
/* .IP "\fBsmtpd_policy_service_timeout (100s)\fR"
/*	The time limit for connecting to, writing to or receiving from a
/*	delegated SMTPD policy server.
/* ACCESS CONTROLS
/* .ad
/* .fi
/*	The SMTPD_ACCESS_README document gives an introduction to all the
/*	SMTP server access control features.
/* .IP "\fBsmtpd_delay_reject (yes)\fR"
/*	Wait until the RCPT TO command before evaluating
/*	$smtpd_client_restrictions, $smtpd_helo_restrictions and
/*	$smtpd_sender_restrictions, or wait until the ETRN command before
/*	evaluating $smtpd_client_restrictions and $smtpd_helo_restrictions.
/* .IP "\fBparent_domain_matches_subdomains (see 'postconf -d' output)\fR"
/*	What Postfix features match subdomains of "domain.tld" automatically,
/*	instead of requiring an explicit ".domain.tld" pattern.
/* .IP "\fBsmtpd_client_restrictions (empty)\fR"
/*	Optional Postfix SMTP server access restrictions in the context of
/*	a remote SMTP client connection request.
/* .IP "\fBsmtpd_helo_required (no)\fR"
/*	Require that a remote SMTP client introduces itself with the HELO
/*	or EHLO command before sending the MAIL command or other commands
/*	that require EHLO negotiation.
/* .IP "\fBsmtpd_helo_restrictions (empty)\fR"
/*	Optional restrictions that the Postfix SMTP server applies in the
/*	context of the SMTP HELO command.
/* .IP "\fBsmtpd_sender_restrictions (empty)\fR"
/*	Optional restrictions that the Postfix SMTP server applies in the
/*	context of the MAIL FROM command.
/* .IP "\fBsmtpd_recipient_restrictions (permit_mynetworks, reject_unauth_destination)\fR"
/*	The access restrictions that the Postfix SMTP server applies in
/*	the context of the RCPT TO command.
/* .IP "\fBsmtpd_etrn_restrictions (empty)\fR"
/*	Optional SMTP server access restrictions in the context of a client
/*	ETRN request.
/* .IP "\fBallow_untrusted_routing (no)\fR"
/*	Forward mail with sender-specified routing (user[@%!]remote[@%!]site)
/*	from untrusted clients to destinations matching $relay_domains.
/* .IP "\fBsmtpd_restriction_classes (empty)\fR"
/*	User-defined aliases for groups of access restrictions.
/* .IP "\fBsmtpd_null_access_lookup_key (<>)\fR"
/*	The lookup key to be used in SMTP \fBaccess\fR(5) tables instead of the
/*	null sender address.
/* .IP "\fBpermit_mx_backup_networks (empty)\fR"
/*	Restrict the use of the permit_mx_backup SMTP access feature to
/*	only domains whose primary MX hosts match the listed networks.
/* .PP
/*	Available in Postfix version 2.0 and later:
/* .IP "\fBsmtpd_data_restrictions (empty)\fR"
/*	Optional access restrictions that the Postfix SMTP server applies
/*	in the context of the SMTP DATA command.
/* .IP "\fBsmtpd_expansion_filter (see 'postconf -d' output)\fR"
/*	What characters are allowed in $name expansions of RBL reply
/*	templates.
/* .PP
/*	Available in Postfix version 2.1 and later:
/* .IP "\fBsmtpd_reject_unlisted_sender (no)\fR"
/*	Request that the Postfix SMTP server rejects mail from unknown
/*	sender addresses, even when no explicit reject_unlisted_sender
/*	access restriction is specified.
/* .IP "\fBsmtpd_reject_unlisted_recipient (yes)\fR"
/*	Request that the Postfix SMTP server rejects mail for unknown
/*	recipient addresses, even when no explicit reject_unlisted_recipient
/*	access restriction is specified.
/* .PP
/*	Available in Postfix version 2.2 and later:
/* .IP "\fBsmtpd_end_of_data_restrictions (empty)\fR"
/*	Optional access restrictions that the Postfix SMTP server
/*	applies in the context of the SMTP END-OF-DATA command.
/* SENDER AND RECIPIENT ADDRESS VERIFICATION CONTROLS
/* .ad
/* .fi
/*	Postfix version 2.1 introduces sender and recipient address verification.
/*	This feature is implemented by sending probe email messages that
/*	are not actually delivered.
/*	This feature is requested via the reject_unverified_sender and
/*	reject_unverified_recipient access restrictions.  The status of
/*	verification probes is maintained by the \fBverify\fR(8) server.
/*	See the file ADDRESS_VERIFICATION_README for information
/*	about how to configure and operate the Postfix sender/recipient
/*	address verification service.
/* .IP "\fBaddress_verify_poll_count (normal: 3, overload: 1)\fR"
/*	How many times to query the \fBverify\fR(8) service for the completion
/*	of an address verification request in progress.
/* .IP "\fBaddress_verify_poll_delay (3s)\fR"
/*	The delay between queries for the completion of an address
/*	verification request in progress.
/* .IP "\fBaddress_verify_sender ($double_bounce_sender)\fR"
/*	The sender address to use in address verification probes; prior
/*	to Postfix 2.5 the default was "postmaster".
/* .IP "\fBunverified_sender_reject_code (450)\fR"
/*	The numerical Postfix SMTP server response code when a recipient
/*	address is rejected by the reject_unverified_sender restriction.
/* .IP "\fBunverified_recipient_reject_code (450)\fR"
/*	The numerical Postfix SMTP server response when a recipient address
/*	is rejected by the reject_unverified_recipient restriction.
/* .PP
/*	Available in Postfix version 2.6 and later:
/* .IP "\fBunverified_sender_defer_code (450)\fR"
/*	The numerical Postfix SMTP server response code when a sender address
/*	probe fails due to a temporary error condition.
/* .IP "\fBunverified_recipient_defer_code (450)\fR"
/*	The numerical Postfix SMTP server response when a recipient address
/*	probe fails due to a temporary error condition.
/* .IP "\fBunverified_sender_reject_reason (empty)\fR"
/*	The Postfix SMTP server's reply when rejecting mail with
/*	reject_unverified_sender.
/* .IP "\fBunverified_recipient_reject_reason (empty)\fR"
/*	The Postfix SMTP server's reply when rejecting mail with
/*	reject_unverified_recipient.
/* .IP "\fBunverified_sender_tempfail_action ($reject_tempfail_action)\fR"
/*	The Postfix SMTP server's action when reject_unverified_sender
/*	fails due to a temporary error condition.
/* .IP "\fBunverified_recipient_tempfail_action ($reject_tempfail_action)\fR"
/*	The Postfix SMTP server's action when reject_unverified_recipient
/*	fails due to a temporary error condition.
/* .PP
/*	Available with Postfix 2.9 and later:
/* .IP "\fBaddress_verify_sender_ttl (0s)\fR"
/*	The time between changes in the time-dependent portion of address
/*	verification probe sender addresses.
/* ACCESS CONTROL RESPONSES
/* .ad
/* .fi
/*	The following parameters control numerical SMTP reply codes
/*	and/or text responses.
/* .IP "\fBaccess_map_reject_code (554)\fR"
/*	The numerical Postfix SMTP server response code for
/*	an \fBaccess\fR(5) map "reject" action.
/* .IP "\fBdefer_code (450)\fR"
/*	The numerical Postfix SMTP server response code when a remote SMTP
/*	client request is rejected by the "defer" restriction.
/* .IP "\fBinvalid_hostname_reject_code (501)\fR"
/*	The numerical Postfix SMTP server response code when the client
/*	HELO or EHLO command parameter is rejected by the reject_invalid_helo_hostname
/*	restriction.
/* .IP "\fBmaps_rbl_reject_code (554)\fR"
/*	The numerical Postfix SMTP server response code when a remote SMTP
/*	client request is blocked by the reject_rbl_client, reject_rhsbl_client,
/*	reject_rhsbl_reverse_client, reject_rhsbl_sender or
/*	reject_rhsbl_recipient restriction.
/* .IP "\fBnon_fqdn_reject_code (504)\fR"
/*	The numerical Postfix SMTP server reply code when a client request
/*	is rejected by the reject_non_fqdn_helo_hostname, reject_non_fqdn_sender
/*	or reject_non_fqdn_recipient restriction.
/* .IP "\fBplaintext_reject_code (450)\fR"
/*	The numerical Postfix SMTP server response code when a request
/*	is rejected by the \fBreject_plaintext_session\fR restriction.
/* .IP "\fBreject_code (554)\fR"
/*	The numerical Postfix SMTP server response code when a remote SMTP
/*	client request is rejected by the "reject" restriction.
/* .IP "\fBrelay_domains_reject_code (554)\fR"
/*	The numerical Postfix SMTP server response code when a client
/*	request is rejected by the reject_unauth_destination recipient
/*	restriction.
/* .IP "\fBunknown_address_reject_code (450)\fR"
/*	The numerical Postfix SMTP server response code when a sender or
/*	recipient address is rejected by the reject_unknown_sender_domain
/*	or reject_unknown_recipient_domain restriction.
/* .IP "\fBunknown_client_reject_code (450)\fR"
/*	The numerical Postfix SMTP server response code when a client
/*	without valid address <=> name mapping is rejected by the
/*	reject_unknown_client_hostname restriction.
/* .IP "\fBunknown_hostname_reject_code (450)\fR"
/*	The numerical Postfix SMTP server response code when the hostname
/*	specified with the HELO or EHLO command is rejected by the
/*	reject_unknown_helo_hostname restriction.
/* .PP
/*	Available in Postfix version 2.0 and later:
/* .IP "\fBdefault_rbl_reply (see 'postconf -d' output)\fR"
/*	The default Postfix SMTP server response template for a request that is
/*	rejected by an RBL-based restriction.
/* .IP "\fBmulti_recipient_bounce_reject_code (550)\fR"
/*	The numerical Postfix SMTP server response code when a remote SMTP
/*	client request is blocked by the reject_multi_recipient_bounce
/*	restriction.
/* .IP "\fBrbl_reply_maps (empty)\fR"
/*	Optional lookup tables with RBL response templates.
/* .PP
/*	Available in Postfix version 2.6 and later:
/* .IP "\fBaccess_map_defer_code (450)\fR"
/*	The numerical Postfix SMTP server response code for
/*	an \fBaccess\fR(5) map "defer" action, including "defer_if_permit"
/*	or "defer_if_reject".
/* .IP "\fBreject_tempfail_action (defer_if_permit)\fR"
/*	The Postfix SMTP server's action when a reject-type restriction
/*	fails due to a temporary error condition.
/* .IP "\fBunknown_helo_hostname_tempfail_action ($reject_tempfail_action)\fR"
/*	The Postfix SMTP server's action when reject_unknown_helo_hostname
/*	fails due to an temporary error condition.
/* .IP "\fBunknown_address_tempfail_action ($reject_tempfail_action)\fR"
/*	The Postfix SMTP server's action when reject_unknown_sender_domain
/*	or reject_unknown_recipient_domain fail due to a temporary error
/*	condition.
/* MISCELLANEOUS CONTROLS
/* .ad
/* .fi
/* .IP "\fBconfig_directory (see 'postconf -d' output)\fR"
/*	The default location of the Postfix main.cf and master.cf
/*	configuration files.
/* .IP "\fBdaemon_timeout (18000s)\fR"
/*	How much time a Postfix daemon process may take to handle a
/*	request before it is terminated by a built-in watchdog timer.
/* .IP "\fBcommand_directory (see 'postconf -d' output)\fR"
/*	The location of all postfix administrative commands.
/* .IP "\fBdouble_bounce_sender (double-bounce)\fR"
/*	The sender address of postmaster notifications that are generated
/*	by the mail system.
/* .IP "\fBipc_timeout (3600s)\fR"
/*	The time limit for sending or receiving information over an internal
/*	communication channel.
/* .IP "\fBmail_name (Postfix)\fR"
/*	The mail system name that is displayed in Received: headers, in
/*	the SMTP greeting banner, and in bounced mail.
/* .IP "\fBmail_owner (postfix)\fR"
/*	The UNIX system account that owns the Postfix queue and most Postfix
/*	daemon processes.
/* .IP "\fBmax_idle (100s)\fR"
/*	The maximum amount of time that an idle Postfix daemon process waits
/*	for an incoming connection before terminating voluntarily.
/* .IP "\fBmax_use (100)\fR"
/*	The maximal number of incoming connections that a Postfix daemon
/*	process will service before terminating voluntarily.
/* .IP "\fBmyhostname (see 'postconf -d' output)\fR"
/*	The internet hostname of this mail system.
/* .IP "\fBmynetworks (see 'postconf -d' output)\fR"
/*	The list of "trusted" remote SMTP clients that have more privileges than
/*	"strangers".
/* .IP "\fBmyorigin ($myhostname)\fR"
/*	The domain name that locally-posted mail appears to come
/*	from, and that locally posted mail is delivered to.
/* .IP "\fBprocess_id (read-only)\fR"
/*	The process ID of a Postfix command or daemon process.
/* .IP "\fBprocess_name (read-only)\fR"
/*	The process name of a Postfix command or daemon process.
/* .IP "\fBqueue_directory (see 'postconf -d' output)\fR"
/*	The location of the Postfix top-level queue directory.
/* .IP "\fBrecipient_delimiter (empty)\fR"
/*	The separator between user names and address extensions (user+foo).
/* .IP "\fBsmtpd_banner ($myhostname ESMTP $mail_name)\fR"
/*	The text that follows the 220 status code in the SMTP greeting
/*	banner.
/* .IP "\fBsyslog_facility (mail)\fR"
/*	The syslog facility of Postfix logging.
/* .IP "\fBsyslog_name (see 'postconf -d' output)\fR"
/*	The mail system name that is prepended to the process name in syslog
/*	records, so that "smtpd" becomes, for example, "postfix/smtpd".
/* .PP
/*	Available in Postfix version 2.2 and later:
/* .IP "\fBsmtpd_forbidden_commands (CONNECT, GET, POST)\fR"
/*	List of commands that cause the Postfix SMTP server to immediately
/*	terminate the session with a 221 code.
/* .PP
/*	Available in Postfix version 2.5 and later:
/* .IP "\fBsmtpd_client_port_logging (no)\fR"
/*	Enable logging of the remote SMTP client port in addition to
/*	the hostname and IP address.
/* SEE ALSO
/*	anvil(8), connection/rate limiting
/*	cleanup(8), message canonicalization
/*	tlsmgr(8), TLS session and PRNG management
/*	trivial-rewrite(8), address resolver
/*	verify(8), address verification service
/*	postconf(5), configuration parameters
/*	master(5), generic daemon options
/*	master(8), process manager
/*	syslogd(8), system logging
/* README FILES
/* .ad
/* .fi
/*	Use "\fBpostconf readme_directory\fR" or
/*	"\fBpostconf html_directory\fR" to locate this information.
/* .na
/* .nf
/*	ADDRESS_CLASS_README, blocking unknown hosted or relay recipients
/*	ADDRESS_REWRITING_README Postfix address manipulation
/*	FILTER_README, external after-queue content filter
/*	LOCAL_RECIPIENT_README, blocking unknown local recipients
/*	MILTER_README, before-queue mail filter applications
/*	SMTPD_ACCESS_README, built-in access policies
/*	SMTPD_POLICY_README, external policy server
/*	SMTPD_PROXY_README, external before-queue content filter
/*	SASL_README, Postfix SASL howto
/*	TLS_README, Postfix STARTTLS howto
/*	VERP_README, Postfix XVERP extension
/*	XCLIENT_README, Postfix XCLIENT extension
/*	XFORWARD_README, Postfix XFORWARD extension
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
/*	Revised TLS support by:
/*	Victor Duchovni
/*	Morgan Stanley
/*--*/

/* System library. */

#include <sys_defs.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>			/* remove() */
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <stddef.h>			/* offsetof() */

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <vstring.h>
#include <vstream.h>
#include <vstring_vstream.h>
#include <stringops.h>
#include <events.h>
#include <smtp_stream.h>
#include <valid_hostname.h>
#include <dict.h>
#include <watchdog.h>
#include <iostuff.h>
#include <split_at.h>
#include <name_code.h>
#include <inet_proto.h>

/* Global library. */

#include <mail_params.h>
#include <mail_version.h>		/* milter_macro_v */
#include <record.h>
#include <rec_type.h>
#include <mail_proto.h>
#include <cleanup_user.h>
#include <mail_date.h>
#include <mail_conf.h>
#include <off_cvt.h>
#include <debug_peer.h>
#include <mail_error.h>
#include <flush_clnt.h>
#include <mail_stream.h>
#include <mail_queue.h>
#include <tok822.h>
#include <verp_sender.h>
#include <string_list.h>
#include <quote_822_local.h>
#include <lex_822.h>
#include <namadr_list.h>
#include <input_transp.h>
#include <is_header.h>
#include <anvil_clnt.h>
#include <flush_clnt.h>
#include <ehlo_mask.h>			/* ehlo filter */
#include <maps.h>			/* ehlo filter */
#include <valid_mailhost_addr.h>
#include <dsn_mask.h>
#include <xtext.h>
#include <tls_proxy.h>
#include <verify_sender_addr.h>

/* Single-threaded server skeleton. */

#include <mail_server.h>

/* Mail filter library. */

#include <milter.h>

/* Application-specific */

#include <smtpd_token.h>
#include <smtpd.h>
#include <smtpd_check.h>
#include <smtpd_chat.h>
#include <smtpd_sasl_proto.h>
#include <smtpd_sasl_glue.h>
#include <smtpd_proxy.h>
#include <smtpd_milter.h>
#include <smtpd_expand.h>

 /*
  * Tunable parameters. Make sure that there is some bound on the length of
  * an SMTP command, so that the mail system stays in control even when a
  * malicious client sends commands of unreasonable length (qmail-dos-1).
  * Make sure there is some bound on the number of recipients, so that the
  * mail system stays in control even when a malicious client sends an
  * unreasonable number of recipients (qmail-dos-2).
  */
int     var_smtpd_rcpt_limit;
int     var_smtpd_tmout;
int     var_smtpd_soft_erlim;
int     var_smtpd_hard_erlim;
int     var_queue_minfree;		/* XXX use off_t */
char   *var_smtpd_banner;
char   *var_notify_classes;
char   *var_client_checks;
char   *var_helo_checks;
char   *var_mail_checks;
char   *var_rcpt_checks;
char   *var_etrn_checks;
char   *var_data_checks;
char   *var_eod_checks;
int     var_unk_client_code;
int     var_bad_name_code;
int     var_unk_name_code;
int     var_unk_addr_code;
int     var_relay_code;
int     var_maps_rbl_code;
int     var_map_reject_code;
int     var_map_defer_code;
char   *var_maps_rbl_domains;
char   *var_rbl_reply_maps;
int     var_helo_required;
int     var_reject_code;
int     var_defer_code;
int     var_smtpd_err_sleep;
int     var_non_fqdn_code;
char   *var_error_rcpt;
int     var_smtpd_delay_reject;
char   *var_rest_classes;
int     var_strict_rfc821_env;
bool    var_disable_vrfy_cmd;
char   *var_canonical_maps;
char   *var_rcpt_canon_maps;
char   *var_virt_alias_maps;
char   *var_virt_mailbox_maps;
char   *var_alias_maps;
char   *var_local_rcpt_maps;
bool    var_allow_untrust_route;
int     var_smtpd_junk_cmd_limit;
int     var_smtpd_rcpt_overlim;
bool    var_smtpd_sasl_enable;
bool    var_smtpd_sasl_auth_hdr;
char   *var_smtpd_sasl_opts;
char   *var_smtpd_sasl_path;
char   *var_cyrus_conf_path;
char   *var_smtpd_sasl_realm;
char   *var_smtpd_sasl_exceptions_networks;
char   *var_smtpd_sasl_type;
char   *var_filter_xport;
bool    var_broken_auth_clients;
char   *var_perm_mx_networks;
char   *var_smtpd_snd_auth_maps;
char   *var_smtpd_noop_cmds;
char   *var_smtpd_null_key;
int     var_smtpd_hist_thrsh;
char   *var_smtpd_exp_filter;
char   *var_def_rbl_reply;
int     var_unv_from_rcode;
int     var_unv_rcpt_rcode;
int     var_unv_from_dcode;
int     var_unv_rcpt_dcode;
char   *var_unv_from_why;
char   *var_unv_rcpt_why;
int     var_mul_rcpt_code;
char   *var_relay_rcpt_maps;
int     var_local_rcpt_code;
int     var_virt_alias_code;
int     var_virt_mailbox_code;
int     var_relay_rcpt_code;
char   *var_verp_clients;
int     var_show_unk_rcpt_table;
int     var_verify_poll_count;
int     var_verify_poll_delay;
char   *var_smtpd_proxy_filt;
int     var_smtpd_proxy_tmout;
char   *var_smtpd_proxy_ehlo;
char   *var_smtpd_proxy_opts;
char   *var_input_transp;
int     var_smtpd_policy_tmout;
int     var_smtpd_policy_idle;
int     var_smtpd_policy_ttl;
char   *var_xclient_hosts;
char   *var_xforward_hosts;
bool    var_smtpd_rej_unl_from;
bool    var_smtpd_rej_unl_rcpt;
char   *var_smtpd_forbid_cmds;
int     var_smtpd_crate_limit;
int     var_smtpd_cconn_limit;
int     var_smtpd_cmail_limit;
int     var_smtpd_crcpt_limit;
int     var_smtpd_cntls_limit;
char   *var_smtpd_hoggers;
char   *var_local_rwr_clients;
char   *var_smtpd_ehlo_dis_words;
char   *var_smtpd_ehlo_dis_maps;

char   *var_smtpd_tls_level;
bool    var_smtpd_use_tls;
bool    var_smtpd_enforce_tls;
bool    var_smtpd_tls_wrappermode;
bool    var_smtpd_tls_auth_only;
char   *var_smtpd_cmd_filter;
char   *var_smtpd_rej_footer;

#ifdef USE_TLS
char   *var_smtpd_relay_ccerts;
char   *var_smtpd_sasl_tls_opts;
int     var_smtpd_starttls_tmout;
char   *var_smtpd_tls_CAfile;
char   *var_smtpd_tls_CApath;
bool    var_smtpd_tls_ask_ccert;
int     var_smtpd_tls_ccert_vd;
char   *var_smtpd_tls_cert_file;
char   *var_smtpd_tls_mand_ciph;
char   *var_smtpd_tls_excl_ciph;
char   *var_smtpd_tls_mand_excl;
char   *var_smtpd_tls_dcert_file;
char   *var_smtpd_tls_dh1024_param_file;
char   *var_smtpd_tls_dh512_param_file;
char   *var_smtpd_tls_dkey_file;
char   *var_smtpd_tls_key_file;
char   *var_smtpd_tls_loglevel;
char   *var_smtpd_tls_mand_proto;
bool    var_smtpd_tls_received_header;
bool    var_smtpd_tls_req_ccert;
int     var_smtpd_tls_scache_timeout;
bool    var_smtpd_tls_set_sessid;
char   *var_smtpd_tls_fpt_dgst;
char   *var_smtpd_tls_ciph;
char   *var_smtpd_tls_proto;
char   *var_smtpd_tls_eecdh;
char   *var_smtpd_tls_eccert_file;
char   *var_smtpd_tls_eckey_file;

#endif

bool    var_smtpd_peername_lookup;
int     var_plaintext_code;
bool    var_smtpd_delay_open;
char   *var_smtpd_milters;
int     var_milt_conn_time;
int     var_milt_cmd_time;
int     var_milt_msg_time;
char   *var_milt_protocol;
char   *var_milt_def_action;
char   *var_milt_daemon_name;
char   *var_milt_v;
char   *var_milt_conn_macros;
char   *var_milt_helo_macros;
char   *var_milt_mail_macros;
char   *var_milt_rcpt_macros;
char   *var_milt_data_macros;
char   *var_milt_eoh_macros;
char   *var_milt_eod_macros;
char   *var_milt_unk_macros;
bool    var_smtpd_client_port_log;
char   *var_stress;

char   *var_reject_tmpf_act;
char   *var_unk_name_tf_act;
char   *var_unk_addr_tf_act;
char   *var_unv_rcpt_tf_act;
char   *var_unv_from_tf_act;
bool    var_smtpd_rec_deadline;

int     smtpd_proxy_opts;

#ifdef USE_TLSPROXY
char   *var_tlsproxy_service;

#endif

 /*
  * Silly little macros.
  */
#define STR(x)	vstring_str(x)
#define LEN(x)	VSTRING_LEN(x)

 /*
  * EHLO keyword filter
  */
static MAPS *ehlo_discard_maps;

 /*
  * VERP command name.
  */
#define VERP_CMD	"XVERP"
#define VERP_CMD_LEN	5

static NAMADR_LIST *verp_clients;

 /*
  * XCLIENT command. Access control is cached, so that XCLIENT can't override
  * its own access control.
  */
static NAMADR_LIST *xclient_hosts;
static int xclient_allowed;		/* XXX should be SMTPD_STATE member */

 /*
  * XFORWARD command. Access control is cached.
  */
static NAMADR_LIST *xforward_hosts;
static int xforward_allowed;		/* XXX should be SMTPD_STATE member */

 /*
  * Client connection and rate limiting.
  */
ANVIL_CLNT *anvil_clnt;
static NAMADR_LIST *hogger_list;

 /*
  * Other application-specific globals.
  */
int     smtpd_input_transp_mask;

 /*
  * Forward declarations.
  */
static void helo_reset(SMTPD_STATE *);
static void mail_reset(SMTPD_STATE *);
static void rcpt_reset(SMTPD_STATE *);
static void tls_reset(SMTPD_STATE *);
static void chat_reset(SMTPD_STATE *, int);

 /*
  * This filter is applied after printable().
  */
#define NEUTER_CHARACTERS " <>()\\\";@"

 /*
  * Reasons for losing the client.
  */
#define REASON_TIMEOUT		"timeout"
#define REASON_LOST_CONNECTION	"lost connection"
#define REASON_ERROR_LIMIT	"too many errors"

 /*
  * Mail filter initialization status.
  */
MILTERS *smtpd_milters;

#ifdef USE_TLS

 /*
  * TLS initialization status.
  */
static TLS_APPL_STATE *smtpd_tls_ctx;
static int ask_client_cert;

#endif

 /*
  * SMTP command mapping for broken clients.
  */
static DICT *smtpd_cmd_filter;

#ifdef USE_SASL_AUTH

 /*
  * SASL exceptions.
  */
static NAMADR_LIST *sasl_exceptions_networks;

/* sasl_client_exception - can we offer AUTH for this client */

static int sasl_client_exception(SMTPD_STATE *state)
{
    int     match;

    /*
     * This is to work around a Netscape mail client bug where it tries to
     * use AUTH if available, even if user has not configured it. Returns
     * TRUE if AUTH should be offered in the EHLO.
     */
    if (sasl_exceptions_networks == 0)
	return (0);

    if ((match = namadr_list_match(sasl_exceptions_networks,
				   state->name, state->addr)) == 0)
	match = sasl_exceptions_networks->error;

    if (msg_verbose)
	msg_info("sasl_exceptions: %s, match=%d",
		 state->namaddr, match);

    return (match);
}

#endif

/* smtpd_whatsup - gather available evidence for logging */

static const char *smtpd_whatsup(SMTPD_STATE *state)
{
    static VSTRING *buf = 0;

    if (buf == 0)
	buf = vstring_alloc(100);
    else
	VSTRING_RESET(buf);
    if (state->sender)
	vstring_sprintf_append(buf, " from=<%s>", state->sender);
    if (state->recipient)
	vstring_sprintf_append(buf, " to=<%s>", state->recipient);
    if (state->protocol)
	vstring_sprintf_append(buf, " proto=%s", state->protocol);
    if (state->helo_name)
	vstring_sprintf_append(buf, " helo=<%s>", state->helo_name);
    return (STR(buf));
}

/* collapse_args - put arguments together again */

static void collapse_args(int argc, SMTPD_TOKEN *argv)
{
    int     i;

    for (i = 1; i < argc; i++) {
	vstring_strcat(argv[0].vstrval, " ");
	vstring_strcat(argv[0].vstrval, argv[i].strval);
    }
    argv[0].strval = STR(argv[0].vstrval);
}

/* check_milter_reply - process reply from Milter */

static const char *check_milter_reply(SMTPD_STATE *state, const char *reply)
{
    const char *queue_id = state->queue_id ? state->queue_id : "NOQUEUE";
    const char *action;
    const char *text;

    /*
     * The syntax of user-specified SMTP replies is checked by the Milter
     * module, because the replies are also used in the cleanup server.
     * Automatically disconnect after 421 (shutdown) reply. The Sendmail 8
     * Milter quarantine action is not final, so it is not included in
     * MILTER_SKIP_FLAGS.
     */
#define MILTER_SKIP_FLAGS (CLEANUP_FLAG_DISCARD)

    switch (reply[0]) {
    case 'H':
	state->saved_flags |= CLEANUP_FLAG_HOLD;
	action = "milter-hold";
	reply = 0;
	text = "milter triggers HOLD action";
	break;
    case 'D':
	state->saved_flags |= CLEANUP_FLAG_DISCARD;
	action = "milter-discard";
	reply = 0;
	text = "milter triggers DISCARD action";
	break;
    case 'S':
	state->error_mask |= MAIL_ERROR_POLICY;
	action = "milter-reject";
	reply = "421 4.7.0 Server closing connection";
	text = 0;
	break;
    case '4':
    case '5':
	state->error_mask |= MAIL_ERROR_POLICY;
	action = "milter-reject";
	text = 0;
	break;
    default:
	state->error_mask |= MAIL_ERROR_SOFTWARE;
	action = "reject";
	reply = "421 4.3.5 Server configuration error";
	text = 0;
	break;
    }
    msg_info("%s: %s: %s from %s: %s;%s", queue_id, action, state->where,
	     state->namaddr, reply ? reply : text, smtpd_whatsup(state));
    return (reply);
}

/* helo_cmd - process HELO command */

static int helo_cmd(SMTPD_STATE *state, int argc, SMTPD_TOKEN *argv)
{
    const char *err;

    /*
     * RFC 2034: the text part of all 2xx, 4xx, and 5xx SMTP responses other
     * than the initial greeting and any response to HELO or EHLO are
     * prefaced with a status code as defined in RFC 3463.
     */
    if (argc < 2) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "501 Syntax: HELO hostname");
	return (-1);
    }
    if (argc > 2)
	collapse_args(argc - 1, argv + 1);
    if (SMTPD_STAND_ALONE(state) == 0
	&& var_smtpd_delay_reject == 0
	&& (err = smtpd_check_helo(state, argv[1].strval)) != 0) {
	smtpd_chat_reply(state, "%s", err);
	return (-1);
    }

    /*
     * XXX Sendmail compatibility: if a Milter rejects CONNECT, EHLO, or
     * HELO, reply with 250 except in case of 421 (disconnect). The reply
     * persists so it will apply to MAIL FROM and to other commands such as
     * AUTH, STARTTLS, and VRFY.
     */
#define PUSH_STRING(old, curr, new)	{ char *old = (curr); (curr) = (new);
#define POP_STRING(old, curr)		(curr) = old; }

    if (smtpd_milters != 0
	&& SMTPD_STAND_ALONE(state) == 0
	&& (state->saved_flags & MILTER_SKIP_FLAGS) == 0
      && (err = milter_helo_event(smtpd_milters, argv[1].strval, 0)) != 0) {
	/* Log reject etc. with correct HELO information. */
	PUSH_STRING(saved_helo, state->helo_name, argv[1].strval);
	err = check_milter_reply(state, err);
	POP_STRING(saved_helo, state->helo_name);
	if (err != 0 && strncmp(err, "421", 3) == 0) {
	    smtpd_chat_reply(state, "%s", err);
	    return (-1);
	}
    }
    if (state->helo_name != 0)
	helo_reset(state);
    chat_reset(state, var_smtpd_hist_thrsh);
    mail_reset(state);
    rcpt_reset(state);
    state->helo_name = mystrdup(printable(argv[1].strval, '?'));
    neuter(state->helo_name, NEUTER_CHARACTERS, '?');
    /* Downgrading the protocol name breaks the unauthorized pipelining test. */
    if (strcasecmp(state->protocol, MAIL_PROTO_ESMTP) != 0
	&& strcasecmp(state->protocol, MAIL_PROTO_SMTP) != 0) {
	myfree(state->protocol);
	state->protocol = mystrdup(MAIL_PROTO_SMTP);
    }
    smtpd_chat_reply(state, "250 %s", var_myhostname);
    return (0);
}

/* cant_announce_feature - explain and terminate this session */

static NORETURN cant_announce_feature(SMTPD_STATE *state, const char *feature)
{
    msg_warn("don't know if EHLO feature %s should be announced to %s",
	     feature, state->namaddr);
    vstream_longjmp(state->client, SMTP_ERR_DATA);
}

/* cant_permit_command - explain and terminate this session */

static NORETURN cant_permit_command(SMTPD_STATE *state, const char *command)
{
    msg_warn("don't know if command %s should be allowed from %s",
	     command, state->namaddr);
    vstream_longjmp(state->client, SMTP_ERR_DATA);
}

/* ehlo_cmd - process EHLO command */

static int ehlo_cmd(SMTPD_STATE *state, int argc, SMTPD_TOKEN *argv)
{
    const char *err;
    int     discard_mask;
    char  **cpp;

    /*
     * XXX 2821 new feature: Section 4.1.4 specifies that a server must clear
     * all buffers and reset the state exactly as if a RSET command had been
     * issued.
     * 
     * RFC 2034: the text part of all 2xx, 4xx, and 5xx SMTP responses other
     * than the initial greeting and any response to HELO or EHLO are
     * prefaced with a status code as defined in RFC 3463.
     */
    if (argc < 2) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "501 Syntax: EHLO hostname");
	return (-1);
    }
    if (argc > 2)
	collapse_args(argc - 1, argv + 1);
    if (SMTPD_STAND_ALONE(state) == 0
	&& var_smtpd_delay_reject == 0
	&& (err = smtpd_check_helo(state, argv[1].strval)) != 0) {
	smtpd_chat_reply(state, "%s", err);
	return (-1);
    }

    /*
     * XXX Sendmail compatibility: if a Milter 5xx rejects CONNECT, EHLO, or
     * HELO, reply with ENHANCEDSTATUSCODES except in case of immediate
     * disconnect. The reply persists so it will apply to MAIL FROM and to
     * other commands such as AUTH, STARTTLS, and VRFY.
     */
    err = 0;
    if (smtpd_milters != 0
	&& SMTPD_STAND_ALONE(state) == 0
	&& (state->saved_flags & MILTER_SKIP_FLAGS) == 0
      && (err = milter_helo_event(smtpd_milters, argv[1].strval, 1)) != 0) {
	/* Log reject etc. with correct HELO information. */
	PUSH_STRING(saved_helo, state->helo_name, argv[1].strval);
	err = check_milter_reply(state, err);
	POP_STRING(saved_helo, state->helo_name);
	if (err != 0 && strncmp(err, "421", 3) == 0) {
	    smtpd_chat_reply(state, "%s", err);
	    return (-1);
	}
    }
    if (state->helo_name != 0)
	helo_reset(state);
    chat_reset(state, var_smtpd_hist_thrsh);
    mail_reset(state);
    rcpt_reset(state);
    state->helo_name = mystrdup(printable(argv[1].strval, '?'));
    neuter(state->helo_name, NEUTER_CHARACTERS, '?');

    /*
     * XXX reject_unauth_pipelining depends on the following. If the user
     * sends EHLO then we announce PIPELINING and we can't accuse them of
     * using pipelining in places where it is allowed.
     * 
     * XXX The reject_unauth_pipelining test needs to change and also account
     * for mechanisms that disable PIPELINING selectively.
     */
    if (strcasecmp(state->protocol, MAIL_PROTO_ESMTP) != 0) {
	myfree(state->protocol);
	state->protocol = mystrdup(MAIL_PROTO_ESMTP);
    }

    /*
     * Build the EHLO response, producing no output until we know what to
     * send - this simplifies exception handling. The CRLF record boundaries
     * don't exist at this level in the code, so we represent multi-line
     * output as an array of single-line responses.
     */
#define EHLO_APPEND(state, cmd) \
    do { \
	vstring_sprintf((state)->ehlo_buf, (cmd)); \
	argv_add((state)->ehlo_argv, STR((state)->ehlo_buf), (char *) 0); \
    } while (0)

#define EHLO_APPEND1(state, cmd, arg) \
    do { \
	vstring_sprintf((state)->ehlo_buf, (cmd), (arg)); \
	argv_add((state)->ehlo_argv, STR((state)->ehlo_buf), (char *) 0); \
    } while (0)

    /*
     * XXX Sendmail compatibility: if a Milter 5XX rejects CONNECT, EHLO, or
     * HELO, reply with ENHANCEDSTATUSCODES only. The reply persists so it
     * will apply to MAIL FROM, but we currently don't have a proper
     * mechanism to apply Milter rejects to AUTH, STARTTLS, VRFY, and other
     * commands while still allowing HELO/EHLO.
     */
    discard_mask = state->ehlo_discard_mask;
    if (err != 0 && err[0] == '5')
	discard_mask |= ~EHLO_MASK_ENHANCEDSTATUSCODES;
    if ((discard_mask & EHLO_MASK_ENHANCEDSTATUSCODES) == 0)
	if (discard_mask && !(discard_mask & EHLO_MASK_SILENT))
	    msg_info("discarding EHLO keywords: %s", str_ehlo_mask(discard_mask));
    if (ehlo_discard_maps && ehlo_discard_maps->error) {
	msg_warn("don't know what EHLO features to announce to %s",
		 state->namaddr);
	vstream_longjmp(state->client, SMTP_ERR_DATA);
    }

    /*
     * These may still exist after a prior exception.
     */
    if (state->ehlo_argv == 0) {
	state->ehlo_argv = argv_alloc(10);
	state->ehlo_buf = vstring_alloc(10);
    } else
	argv_truncate(state->ehlo_argv, 0);

    EHLO_APPEND1(state, "%s", var_myhostname);
    if ((discard_mask & EHLO_MASK_PIPELINING) == 0)
	EHLO_APPEND(state, "PIPELINING");
    if ((discard_mask & EHLO_MASK_SIZE) == 0) {
	if (var_message_limit)
	    EHLO_APPEND1(state, "SIZE %lu",
			 (unsigned long) var_message_limit);	/* XXX */
	else
	    EHLO_APPEND(state, "SIZE");
    }
    if ((discard_mask & EHLO_MASK_VRFY) == 0)
	if (var_disable_vrfy_cmd == 0)
	    EHLO_APPEND(state, SMTPD_CMD_VRFY);
    if ((discard_mask & EHLO_MASK_ETRN) == 0)
	EHLO_APPEND(state, SMTPD_CMD_ETRN);
#ifdef USE_TLS
    if ((discard_mask & EHLO_MASK_STARTTLS) == 0)
	if (var_smtpd_use_tls && (!state->tls_context))
	    EHLO_APPEND(state, SMTPD_CMD_STARTTLS);
#endif
#ifdef USE_SASL_AUTH
#ifndef AUTH_CMD
#define AUTH_CMD	"AUTH"
#endif
    if ((discard_mask & EHLO_MASK_AUTH) == 0) {
	if (smtpd_sasl_is_active(state) && !sasl_client_exception(state)) {
	    EHLO_APPEND1(state, "AUTH %s", state->sasl_mechanism_list);
	    if (var_broken_auth_clients)
		EHLO_APPEND1(state, "AUTH=%s", state->sasl_mechanism_list);
	} else if (sasl_exceptions_networks && sasl_exceptions_networks->error)
	    cant_announce_feature(state, AUTH_CMD);
    }
#define XCLIENT_LOGIN_KLUDGE	" " XCLIENT_LOGIN
#else
#define XCLIENT_LOGIN_KLUDGE	""
#endif
    if ((discard_mask & EHLO_MASK_VERP) == 0) {
	if (namadr_list_match(verp_clients, state->name, state->addr))
	    EHLO_APPEND(state, VERP_CMD);
	else if (verp_clients && verp_clients->error)
	    cant_announce_feature(state, VERP_CMD);
    }
    /* XCLIENT must not override its own access control. */
    if ((discard_mask & EHLO_MASK_XCLIENT) == 0) {
	if (xclient_allowed)
	    EHLO_APPEND(state, XCLIENT_CMD
			" " XCLIENT_NAME " " XCLIENT_ADDR
			" " XCLIENT_PROTO " " XCLIENT_HELO
			" " XCLIENT_REVERSE_NAME " " XCLIENT_PORT
			XCLIENT_LOGIN_KLUDGE);
	else if (xclient_hosts && xclient_hosts->error)
	    cant_announce_feature(state, XCLIENT_CMD);
    }
    if ((discard_mask & EHLO_MASK_XFORWARD) == 0) {
	if (xforward_allowed)
	    EHLO_APPEND(state, XFORWARD_CMD
			" " XFORWARD_NAME " " XFORWARD_ADDR
			" " XFORWARD_PROTO " " XFORWARD_HELO
			" " XFORWARD_DOMAIN " " XFORWARD_PORT
			" " XFORWARD_IDENT);
	else if (xforward_hosts && xforward_hosts->error)
	    cant_announce_feature(state, XFORWARD_CMD);
    }
    if ((discard_mask & EHLO_MASK_ENHANCEDSTATUSCODES) == 0)
	EHLO_APPEND(state, "ENHANCEDSTATUSCODES");
    if ((discard_mask & EHLO_MASK_8BITMIME) == 0)
	EHLO_APPEND(state, "8BITMIME");
    if ((discard_mask & EHLO_MASK_DSN) == 0)
	EHLO_APPEND(state, "DSN");

    /*
     * Send the reply.
     */
    for (cpp = state->ehlo_argv->argv; *cpp; cpp++)
	smtpd_chat_reply(state, "250%c%s", cpp[1] ? '-' : ' ', *cpp);

    /*
     * Clean up.
     */
    argv_free(state->ehlo_argv);
    state->ehlo_argv = 0;
    vstring_free(state->ehlo_buf);
    state->ehlo_buf = 0;

    return (0);
}

/* helo_reset - reset HELO/EHLO command stuff */

static void helo_reset(SMTPD_STATE *state)
{
    if (state->helo_name) {
	myfree(state->helo_name);
	state->helo_name = 0;
	if (SMTPD_STAND_ALONE(state) == 0 && smtpd_milters != 0)
	    milter_abort(smtpd_milters);
    }
    if (state->ehlo_argv) {
	argv_free(state->ehlo_argv);
	state->ehlo_argv = 0;
    }
    if (state->ehlo_buf) {
	vstring_free(state->ehlo_buf);
	state->ehlo_buf = 0;
    }
}

/* mail_open_stream - open mail queue file or IPC stream */

static int mail_open_stream(SMTPD_STATE *state)
{

    /*
     * Connect to the before-queue filter when one is configured. The MAIL
     * FROM and RCPT TO commands are forwarded as received (including DSN
     * attributes), with the exception that the before-filter smtpd process
     * handles all authentication, encryption, access control and relay
     * control, and that the before-filter smtpd process does not forward
     * blocked commands. If the after-filter smtp server does not support
     * some of Postfix's ESMTP features, then they must be turned off in the
     * before-filter smtpd process with the smtpd_discard_ehlo_keywords
     * feature.
     */
    if (state->proxy_mail) {
	if (smtpd_proxy_create(state, smtpd_proxy_opts, var_smtpd_proxy_filt,
			       var_smtpd_proxy_tmout, var_smtpd_proxy_ehlo,
			       state->proxy_mail) != 0) {
	    smtpd_chat_reply(state, "%s", STR(state->proxy->buffer));
	    smtpd_proxy_free(state);
	    return (-1);
	}
    }

    /*
     * If running from the master or from inetd, connect to the cleanup
     * service.
     * 
     * XXX 2821: An SMTP server is not allowed to "clean up" mail except in the
     * case of original submissions.
     * 
     * We implement this by distinguishing between mail that we are willing to
     * rewrite (the local rewrite context) and mail from elsewhere.
     */
    else if (SMTPD_STAND_ALONE(state) == 0) {
	int     cleanup_flags;

	cleanup_flags = input_transp_cleanup(CLEANUP_FLAG_MASK_EXTERNAL,
					     smtpd_input_transp_mask)
	    | CLEANUP_FLAG_SMTP_REPLY;
	state->dest = mail_stream_service(MAIL_CLASS_PUBLIC,
					  var_cleanup_service);
	if (state->dest == 0
	    || attr_print(state->dest->stream, ATTR_FLAG_NONE,
			  ATTR_TYPE_INT, MAIL_ATTR_FLAGS, cleanup_flags,
			  ATTR_TYPE_END) != 0)
	    msg_fatal("unable to connect to the %s %s service",
		      MAIL_CLASS_PUBLIC, var_cleanup_service);
    }

    /*
     * Otherwise, pipe the message through the privileged postdrop helper.
     * XXX Make postdrop a manifest constant.
     */
    else {
	char   *postdrop_command;

	postdrop_command = concatenate(var_command_dir, "/postdrop",
			      msg_verbose ? " -v" : (char *) 0, (char *) 0);
	state->dest = mail_stream_command(postdrop_command);
	if (state->dest == 0)
	    msg_fatal("unable to execute %s", postdrop_command);
	myfree(postdrop_command);
    }

    /*
     * Record the time of arrival, the SASL-related stuff if applicable, the
     * sender envelope address, some session information, and some additional
     * attributes.
     * 
     * XXX Send Milter information first, because this will hang when cleanup
     * goes into "throw away" mode. Also, cleanup needs to know early on
     * whether or not it has to do its own SMTP event emulation.
     * 
     * XXX At this point we send only dummy information to keep the cleanup
     * server from using its non_smtpd_milters settings. We have to send
     * up-to-date Milter information after DATA so that the cleanup server
     * knows the actual Milter state.
     */
    if (state->dest) {
	state->cleanup = state->dest->stream;
	state->queue_id = mystrdup(state->dest->id);
	if (SMTPD_STAND_ALONE(state) == 0) {
	    if (smtpd_milters != 0
		&& (state->saved_flags & MILTER_SKIP_FLAGS) == 0)
		/* Send place-holder smtpd_milters list. */
		(void) milter_dummy(smtpd_milters, state->cleanup);
	    rec_fprintf(state->cleanup, REC_TYPE_TIME, REC_TYPE_TIME_FORMAT,
			REC_TYPE_TIME_ARG(state->arrival_time));
	    if (*var_filter_xport)
		rec_fprintf(state->cleanup, REC_TYPE_FILT, "%s", var_filter_xport);
	    if (FORWARD_IDENT(state))
		rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%s",
			    MAIL_ATTR_LOG_IDENT, FORWARD_IDENT(state));
	    rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%s",
			MAIL_ATTR_RWR_CONTEXT, FORWARD_DOMAIN(state));
#ifdef USE_SASL_AUTH
	    /* Make external authentication painless (e.g., XCLIENT). */
	    if (state->sasl_method)
		rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%s",
			    MAIL_ATTR_SASL_METHOD, state->sasl_method);
	    if (state->sasl_username)
		rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%s",
			    MAIL_ATTR_SASL_USERNAME, state->sasl_username);
	    if (state->sasl_sender)
		rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%s",
			    MAIL_ATTR_SASL_SENDER, state->sasl_sender);
#endif

	    /*
	     * Record DSN related information that was received with the MAIL
	     * FROM command.
	     * 
	     * RFC 3461 Section 5.2.1. If no ENVID parameter was included in the
	     * MAIL command when the message was received, the ENVID
	     * parameter MUST NOT be supplied when the message is relayed.
	     * Ditto for the RET parameter.
	     * 
	     * In other words, we can't simply make up our default ENVID or RET
	     * values. We have to remember whether the client sent any.
	     * 
	     * We store DSN information as named attribute records so that we
	     * don't have to pollute the queue file with records that are
	     * incompatible with past Postfix versions. Preferably, people
	     * should be able to back out from an upgrade without losing
	     * mail.
	     */
	    if (state->dsn_envid)
		rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%s",
			    MAIL_ATTR_DSN_ENVID, state->dsn_envid);
	    if (state->dsn_ret)
		rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%d",
			    MAIL_ATTR_DSN_RET, state->dsn_ret);
	}
	rec_fputs(state->cleanup, REC_TYPE_FROM, state->sender);
	if (state->encoding != 0)
	    rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%s",
			MAIL_ATTR_ENCODING, state->encoding);

	/*
	 * Store client attributes.
	 */
	if (SMTPD_STAND_ALONE(state) == 0) {

	    /*
	     * Attributes for logging, also used for XFORWARD.
	     * 
	     * We store all client attributes, including ones with unknown
	     * values. Otherwise, an unknown client hostname would be treated
	     * as a non-existent hostname (i.e. local submission).
	     */
	    rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%s",
			MAIL_ATTR_LOG_CLIENT_NAME, FORWARD_NAME(state));
	    /* XXX Note: state->rfc_addr, not state->addr. */
	    rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%s",
			MAIL_ATTR_LOG_CLIENT_ADDR, FORWARD_ADDR(state));
	    rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%s",
			MAIL_ATTR_LOG_CLIENT_PORT, FORWARD_PORT(state));
	    rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%s",
			MAIL_ATTR_LOG_ORIGIN, FORWARD_NAMADDR(state));
	    if (FORWARD_HELO(state))
		rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%s",
			    MAIL_ATTR_LOG_HELO_NAME, FORWARD_HELO(state));
	    rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%s",
			MAIL_ATTR_LOG_PROTO_NAME, FORWARD_PROTO(state));

	    /*
	     * Attributes with actual client information. These are used by
	     * the smtpd Milter client for policy decisions. Mail that is
	     * requeued with "postsuper -r" is not subject to processing by
	     * the cleanup Milter client, because a) it has already been
	     * filtered, and b) we don't have sufficient information to
	     * reproduce the exact same SMTP events and Sendmail macros that
	     * the smtpd Milter client received when the message originally
	     * arrived in Postfix.
	     */
	    rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%s",
			MAIL_ATTR_ACT_CLIENT_NAME, state->name);
	    rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%s",
		    MAIL_ATTR_ACT_REVERSE_CLIENT_NAME, state->reverse_name);
	    /* XXX Note: state->addr, not state->rfc_addr. */
	    rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%s",
			MAIL_ATTR_ACT_CLIENT_ADDR, state->addr);
	    rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%s",
			MAIL_ATTR_ACT_CLIENT_PORT, state->port);
	    if (state->helo_name)
		rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%s",
			    MAIL_ATTR_ACT_HELO_NAME, state->helo_name);
	    rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%s",
			MAIL_ATTR_ACT_PROTO_NAME, state->protocol);
	    rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%u",
			MAIL_ATTR_ACT_CLIENT_AF, state->addr_family);

	    /*
	     * Don't send client certificate down the pipeline unless it is
	     * a) verified or b) just a fingerprint.
	     */
	}
	if (state->verp_delims)
	    rec_fputs(state->cleanup, REC_TYPE_VERP, state->verp_delims);
    }

    /*
     * Log the queue ID with the message origin.
     */
#ifdef USE_SASL_AUTH
    if (state->sasl_username)
	smtpd_sasl_mail_log(state);
    else
#endif

	/*
	 * See also: smtpd_sasl_proto.c, for a longer client= logfile record.
	 */
#define PRINT_OR_NULL(cond, str) \
	    ((cond) ? (str) : "")
#define PRINT2_OR_NULL(cond, name, value) \
	    PRINT_OR_NULL((cond), (name)), PRINT_OR_NULL((cond), (value))

	msg_info("%s: client=%s%s%s%s%s",
		 (state->queue_id ? state->queue_id : "NOQUEUE"),
		 state->namaddr,
		 PRINT2_OR_NULL(HAVE_FORWARDED_IDENT(state),
				", orig_queue_id=", FORWARD_IDENT(state)),
		 PRINT2_OR_NULL(HAVE_FORWARDED_CLIENT_ATTR(state),
				", orig_client=", FORWARD_NAMADDR(state)));
    return (0);
}

/* extract_addr - extract address from rubble */

static int extract_addr(SMTPD_STATE *state, SMTPD_TOKEN *arg,
			        int allow_empty_addr, int strict_rfc821)
{
    const char *myname = "extract_addr";
    TOK822 *tree;
    TOK822 *tp;
    TOK822 *addr = 0;
    int     naddr;
    int     non_addr;
    int     err = 0;
    char   *junk = 0;
    char   *text;
    char   *colon;

    /*
     * Special case.
     */
#define PERMIT_EMPTY_ADDR	1
#define REJECT_EMPTY_ADDR	0

    /*
     * Some mailers send RFC822-style address forms (with comments and such)
     * in SMTP envelopes. We cannot blame users for this: the blame is with
     * programmers violating the RFC, and with sendmail for being permissive.
     * 
     * XXX The SMTP command tokenizer must leave the address in externalized
     * (quoted) form, so that the address parser can correctly extract the
     * address from surrounding junk.
     * 
     * XXX We have only one address parser, written according to the rules of
     * RFC 822. That standard differs subtly from RFC 821.
     */
    if (msg_verbose)
	msg_info("%s: input: %s", myname, STR(arg->vstrval));
    if (STR(arg->vstrval)[0] == '<'
	&& STR(arg->vstrval)[LEN(arg->vstrval) - 1] == '>') {
	junk = text = mystrndup(STR(arg->vstrval) + 1, LEN(arg->vstrval) - 2);
    } else
	text = STR(arg->vstrval);

    /*
     * Truncate deprecated route address form.
     */
    if (*text == '@' && (colon = strchr(text, ':')) != 0)
	text = colon + 1;
    tree = tok822_parse(text);

    if (junk)
	myfree(junk);

    /*
     * Find trouble.
     */
    for (naddr = non_addr = 0, tp = tree; tp != 0; tp = tp->next) {
	if (tp->type == TOK822_ADDR) {
	    addr = tp;
	    naddr += 1;				/* count address forms */
	} else if (tp->type == '<' || tp->type == '>') {
	     /* void */ ;			/* ignore brackets */
	} else {
	    non_addr += 1;			/* count non-address forms */
	}
    }

    /*
     * Report trouble. XXX Should log a warning only if we are going to
     * sleep+reject so that attackers can't flood our logfiles.
     * 
     * XXX Unfortunately, the sleep-before-reject feature had to be abandoned
     * (at least for small error counts) because servers were DOS-ing
     * themselves when flooded by backscatter traffic.
     */
    if (naddr > 1
	|| (strict_rfc821 && (non_addr || *STR(arg->vstrval) != '<'))) {
	msg_warn("Illegal address syntax from %s in %s command: %s",
		 state->namaddr, state->where,
		 printable(STR(arg->vstrval), '?'));
	err = 1;
    }

    /*
     * Don't overwrite the input with the extracted address. We need the
     * original (external) form in case the client does not send ORCPT
     * information; and error messages are more accurate if we log the
     * unmodified form. We need the internal form for all other purposes.
     */
    if (addr)
	tok822_internalize(state->addr_buf, addr->head, TOK822_STR_DEFL);
    else
	vstring_strcpy(state->addr_buf, "");

    /*
     * Report trouble. XXX Should log a warning only if we are going to
     * sleep+reject so that attackers can't flood our logfiles. Log the
     * original address.
     */
    if (err == 0)
	if ((STR(state->addr_buf)[0] == 0 && !allow_empty_addr)
	    || (strict_rfc821 && STR(state->addr_buf)[0] == '@')
	    || (SMTPD_STAND_ALONE(state) == 0
		&& smtpd_check_addr(STR(state->addr_buf)) != 0)) {
	    msg_warn("Illegal address syntax from %s in %s command: %s",
		     state->namaddr, state->where,
		     printable(STR(arg->vstrval), '?'));
	    err = 1;
	}

    /*
     * Cleanup.
     */
    tok822_free_tree(tree);
    if (msg_verbose)
	msg_info("%s: in: %s, result: %s",
		 myname, STR(arg->vstrval), STR(state->addr_buf));
    return (err);
}

/* milter_argv - impedance adapter */

static const char **milter_argv(SMTPD_STATE *state, int argc, SMTPD_TOKEN *argv)
{
    int     n;
    ssize_t len = argc + 1;

    if (state->milter_argc < len) {
	if (state->milter_argc > 0)
	    state->milter_argv = (const char **)
		myrealloc((char *) state->milter_argv,
			  sizeof(const char *) * len);
	else
	    state->milter_argv = (const char **)
		mymalloc(sizeof(const char *) * len);
	state->milter_argc = len;
    }
    for (n = 0; n < argc; n++)
	state->milter_argv[n] = argv[n].strval;
    state->milter_argv[n] = 0;
    return (state->milter_argv);
}

/* mail_cmd - process MAIL command */

static int mail_cmd(SMTPD_STATE *state, int argc, SMTPD_TOKEN *argv)
{
    const char *err;
    int     narg;
    char   *arg;
    char   *verp_delims = 0;
    int     rate;
    int     dsn_envid = 0;

    state->encoding = 0;
    state->dsn_ret = 0;

    /*
     * Sanity checks.
     * 
     * XXX 2821 pedantism: Section 4.1.2 says that SMTP servers that receive a
     * command in which invalid character codes have been employed, and for
     * which there are no other reasons for rejection, MUST reject that
     * command with a 501 response. Postfix attempts to be 8-bit clean.
     */
    if (var_helo_required && state->helo_name == 0) {
	state->error_mask |= MAIL_ERROR_POLICY;
	smtpd_chat_reply(state, "503 5.5.1 Error: send HELO/EHLO first");
	return (-1);
    }
#define IN_MAIL_TRANSACTION(state) ((state)->sender != 0)

    if (IN_MAIL_TRANSACTION(state)) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "503 5.5.1 Error: nested MAIL command");
	return (-1);
    }
    if (argc < 3
	|| strcasecmp(argv[1].strval, "from:") != 0) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "501 5.5.4 Syntax: MAIL FROM:<address>");
	return (-1);
    }

    /*
     * XXX The client event count/rate control must be consistent in its use
     * of client address information in connect and disconnect events. For
     * now we exclude xclient authorized hosts from event count/rate control.
     */
    if (SMTPD_STAND_ALONE(state) == 0
	&& !xclient_allowed
	&& anvil_clnt
	&& var_smtpd_cmail_limit > 0
	&& !namadr_list_match(hogger_list, state->name, state->addr)
	&& anvil_clnt_mail(anvil_clnt, state->service, state->addr,
			   &rate) == ANVIL_STAT_OK
	&& rate > var_smtpd_cmail_limit) {
	state->error_mask |= MAIL_ERROR_POLICY;
	smtpd_chat_reply(state, "450 4.7.1 Error: too much mail from %s",
			 state->addr);
	msg_warn("Message delivery request rate limit exceeded: %d from %s for service %s",
		 rate, state->namaddr, state->service);
	return (-1);
    }
    if (argv[2].tokval == SMTPD_TOK_ERROR) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "501 5.1.7 Bad sender address syntax");
	return (-1);
    }
    if (extract_addr(state, argv + 2, PERMIT_EMPTY_ADDR, var_strict_rfc821_env) != 0) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "501 5.1.7 Bad sender address syntax");
	return (-1);
    }
    for (narg = 3; narg < argc; narg++) {
	arg = argv[narg].strval;
	if (strcasecmp(arg, "BODY=8BITMIME") == 0) {	/* RFC 1652 */
	    state->encoding = MAIL_ATTR_ENC_8BIT;
	} else if (strcasecmp(arg, "BODY=7BIT") == 0) {	/* RFC 1652 */
	    state->encoding = MAIL_ATTR_ENC_7BIT;
	} else if (strncasecmp(arg, "SIZE=", 5) == 0) {	/* RFC 1870 */
	    /* Reject non-numeric size. */
	    if (!alldig(arg + 5)) {
		state->error_mask |= MAIL_ERROR_PROTOCOL;
		smtpd_chat_reply(state, "501 5.5.4 Bad message size syntax");
		return (-1);
	    }
	    /* Reject size overflow. */
	    if ((state->msg_size = off_cvt_string(arg + 5)) < 0) {
		state->error_mask |= MAIL_ERROR_POLICY;
		smtpd_chat_reply(state, "552 5.3.4 Message size exceeds file system imposed limit");
		return (-1);
	    }
#ifdef USE_SASL_AUTH
	} else if (strncasecmp(arg, "AUTH=", 5) == 0) {
	    if ((err = smtpd_sasl_mail_opt(state, arg + 5)) != 0) {
		smtpd_chat_reply(state, "%s", err);
		return (-1);
	    }
#endif
	} else if (namadr_list_match(verp_clients, state->name, state->addr)
		   && strncasecmp(arg, VERP_CMD, VERP_CMD_LEN) == 0
		   && (arg[VERP_CMD_LEN] == '=' || arg[VERP_CMD_LEN] == 0)) {
	    if (arg[VERP_CMD_LEN] == 0) {
		verp_delims = var_verp_delims;
	    } else {
		verp_delims = arg + VERP_CMD_LEN + 1;
		if (verp_delims_verify(verp_delims) != 0) {
		    state->error_mask |= MAIL_ERROR_PROTOCOL;
		    smtpd_chat_reply(state,
			 "501 5.5.4 Error: %s needs two characters from %s",
				     VERP_CMD, var_verp_filter);
		    return (-1);
		}
	    }
	} else if (strncasecmp(arg, "RET=", 4) == 0) {	/* RFC 3461 */
	    /* Sanitized on input. */
	    if (state->ehlo_discard_mask & EHLO_MASK_DSN) {
		state->error_mask |= MAIL_ERROR_PROTOCOL;
		smtpd_chat_reply(state, "501 5.7.1 DSN support is disabled");
		return (-1);
	    }
	    if (state->dsn_ret
		|| (state->dsn_ret = dsn_ret_code(arg + 4)) == 0) {
		state->error_mask |= MAIL_ERROR_PROTOCOL;
		smtpd_chat_reply(state,
				 "501 5.5.4 Bad RET parameter syntax");
		return (-1);
	    }
	} else if (strncasecmp(arg, "ENVID=", 6) == 0) {	/* RFC 3461 */
	    /* Sanitized by bounce server. */
	    if (state->ehlo_discard_mask & EHLO_MASK_DSN) {
		state->error_mask |= MAIL_ERROR_PROTOCOL;
		smtpd_chat_reply(state, "501 5.7.1 DSN support is disabled");
		return (-1);
	    }
	    if (dsn_envid
		|| xtext_unquote(state->dsn_buf, arg + 6) == 0
		|| !allprint(STR(state->dsn_buf))) {
		state->error_mask |= MAIL_ERROR_PROTOCOL;
		smtpd_chat_reply(state, "501 5.5.4 Bad ENVID parameter syntax");
		return (-1);
	    }
	    dsn_envid = 1;
	} else {
	    state->error_mask |= MAIL_ERROR_PROTOCOL;
	    smtpd_chat_reply(state, "555 5.5.4 Unsupported option: %s", arg);
	    return (-1);
	}
    }
    if ((err = smtpd_check_size(state, state->msg_size)) != 0) {
	smtpd_chat_reply(state, "%s", err);
	return (-1);
    }
    if (verp_delims && STR(state->addr_buf)[0] == 0) {
	smtpd_chat_reply(state, "503 5.5.4 Error: %s requires non-null sender",
			 VERP_CMD);
	return (-1);
    }
    if (SMTPD_STAND_ALONE(state) == 0
	&& var_smtpd_delay_reject == 0
	&& (err = smtpd_check_mail(state, STR(state->addr_buf))) != 0) {
	/* XXX Reset access map side effects. */
	mail_reset(state);
	smtpd_chat_reply(state, "%s", err);
	return (-1);
    }
    if (smtpd_milters != 0
	&& SMTPD_STAND_ALONE(state) == 0
	&& (state->saved_flags & MILTER_SKIP_FLAGS) == 0) {
	PUSH_STRING(saved_sender, state->sender, STR(state->addr_buf));
	err = milter_mail_event(smtpd_milters,
				milter_argv(state, argc - 2, argv + 2));
	if (err != 0) {
	    /* Log reject etc. with correct sender information. */
	    err = check_milter_reply(state, err);
	}
	POP_STRING(saved_sender, state->sender);
	if (err != 0) {
	    /* XXX Reset access map side effects. */
	    mail_reset(state);
	    smtpd_chat_reply(state, "%s", err);
	    return (-1);
	}
    }
    if (SMTPD_STAND_ALONE(state) == 0) {
	err = smtpd_check_rewrite(state);
	if (err != 0) {
	    /* XXX Reset access map side effects. */
	    mail_reset(state);
	    smtpd_chat_reply(state, "%s", err);
	    return (-1);
	}
    }

    /*
     * Check the queue file space, if applicable. The optional before-filter
     * speed-adjust buffers use disk space. However, we don't know if they
     * compete for storage space with the after-filter queue, so we can't
     * simply bump up the free space requirement to 2.5 * message_size_limit.
     */
    if (!USE_SMTPD_PROXY(state)
	|| (smtpd_proxy_opts & SMTPD_PROXY_FLAG_SPEED_ADJUST)) {
	if (SMTPD_STAND_ALONE(state) == 0
	    && (err = smtpd_check_queue(state)) != 0) {
	    /* XXX Reset access map side effects. */
	    mail_reset(state);
	    smtpd_chat_reply(state, "%s", err);
	    return (-1);
	}
    }

    /*
     * No more early returns. The mail transaction is in progress.
     */
    GETTIMEOFDAY(&state->arrival_time);
    state->sender = mystrdup(STR(state->addr_buf));
    vstring_sprintf(state->instance, "%x.%lx.%lx.%x",
		    var_pid, (unsigned long) state->arrival_time.tv_sec,
	       (unsigned long) state->arrival_time.tv_usec, state->seqno++);
    if (verp_delims)
	state->verp_delims = mystrdup(verp_delims);
    if (dsn_envid)
	state->dsn_envid = mystrdup(STR(state->dsn_buf));
    if (USE_SMTPD_PROXY(state))
	state->proxy_mail = mystrdup(STR(state->buffer));
    if (var_smtpd_delay_open == 0 && mail_open_stream(state) < 0) {
	/* XXX Reset access map side effects. */
	mail_reset(state);
	return (-1);
    }
    smtpd_chat_reply(state, "250 2.1.0 Ok");
    return (0);
}

/* mail_reset - reset MAIL command stuff */

static void mail_reset(SMTPD_STATE *state)
{
    state->msg_size = 0;
    state->act_size = 0;
    state->flags &= SMTPD_MASK_MAIL_KEEP;

    /*
     * Unceremoniously close the pipe to the cleanup service. The cleanup
     * service will delete the queue file when it detects a premature
     * end-of-file condition on input.
     */
    if (state->cleanup != 0) {
	mail_stream_cleanup(state->dest);
	state->dest = 0;
	state->cleanup = 0;
    }
    state->err = 0;
    if (state->queue_id != 0) {
	myfree(state->queue_id);
	state->queue_id = 0;
    }
    if (state->sender) {
	if (SMTPD_STAND_ALONE(state) == 0 && smtpd_milters != 0)
	    milter_abort(smtpd_milters);
	myfree(state->sender);
	state->sender = 0;
    }
    if (state->verp_delims) {
	myfree(state->verp_delims);
	state->verp_delims = 0;
    }
    if (state->proxy_mail) {
	myfree(state->proxy_mail);
	state->proxy_mail = 0;
    }
    if (state->saved_filter) {
	myfree(state->saved_filter);
	state->saved_filter = 0;
    }
    if (state->saved_redirect) {
	myfree(state->saved_redirect);
	state->saved_redirect = 0;
    }
    if (state->saved_bcc) {
	myfree(state->saved_bcc);
	state->saved_bcc = 0;
    }
    state->saved_flags = 0;
#ifdef DELAY_ACTION
    state->saved_delay = 0;
#endif
#ifdef USE_SASL_AUTH
    if (state->sasl_sender)
	smtpd_sasl_mail_reset(state);
#endif
    state->discard = 0;
    VSTRING_RESET(state->instance);
    VSTRING_TERMINATE(state->instance);

    if (state->proxy)
	smtpd_proxy_free(state);
    if (state->xforward.flags)
	smtpd_xforward_reset(state);
    if (state->prepend)
	state->prepend = argv_free(state->prepend);
    if (state->dsn_envid) {
	myfree(state->dsn_envid);
	state->dsn_envid = 0;
    }
    if (state->milter_argv) {
	myfree((char *) state->milter_argv);
	state->milter_argv = 0;
	state->milter_argc = 0;
    }
}

/* rcpt_cmd - process RCPT TO command */

static int rcpt_cmd(SMTPD_STATE *state, int argc, SMTPD_TOKEN *argv)
{
    SMTPD_PROXY *proxy;
    const char *err;
    int     narg;
    char   *arg;
    int     rate;
    const char *dsn_orcpt_addr = 0;
    ssize_t dsn_orcpt_addr_len = 0;
    const char *dsn_orcpt_type = 0;
    int     dsn_notify = 0;
    const char *coded_addr;
    const char *milter_err;

    /*
     * Sanity checks.
     * 
     * XXX 2821 pedantism: Section 4.1.2 says that SMTP servers that receive a
     * command in which invalid character codes have been employed, and for
     * which there are no other reasons for rejection, MUST reject that
     * command with a 501 response. So much for the principle of "be liberal
     * in what you accept, be strict in what you send".
     */
    if (!IN_MAIL_TRANSACTION(state)) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "503 5.5.1 Error: need MAIL command");
	return (-1);
    }
    if (argc < 3
	|| strcasecmp(argv[1].strval, "to:") != 0) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "501 5.5.4 Syntax: RCPT TO:<address>");
	return (-1);
    }

    /*
     * XXX The client event count/rate control must be consistent in its use
     * of client address information in connect and disconnect events. For
     * now we exclude xclient authorized hosts from event count/rate control.
     */
    if (SMTPD_STAND_ALONE(state) == 0
	&& !xclient_allowed
	&& anvil_clnt
	&& var_smtpd_crcpt_limit > 0
	&& !namadr_list_match(hogger_list, state->name, state->addr)
	&& anvil_clnt_rcpt(anvil_clnt, state->service, state->addr,
			   &rate) == ANVIL_STAT_OK
	&& rate > var_smtpd_crcpt_limit) {
	state->error_mask |= MAIL_ERROR_POLICY;
	msg_warn("Recipient address rate limit exceeded: %d from %s for service %s",
		 rate, state->namaddr, state->service);
	smtpd_chat_reply(state, "450 4.7.1 Error: too many recipients from %s",
			 state->addr);
	return (-1);
    }
    if (argv[2].tokval == SMTPD_TOK_ERROR) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "501 5.1.3 Bad recipient address syntax");
	return (-1);
    }
    if (extract_addr(state, argv + 2, REJECT_EMPTY_ADDR, var_strict_rfc821_env) != 0) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "501 5.1.3 Bad recipient address syntax");
	return (-1);
    }
    for (narg = 3; narg < argc; narg++) {
	arg = argv[narg].strval;
	if (strncasecmp(arg, "NOTIFY=", 7) == 0) {	/* RFC 3461 */
	    /* Sanitized on input. */
	    if (state->ehlo_discard_mask & EHLO_MASK_DSN) {
		state->error_mask |= MAIL_ERROR_PROTOCOL;
		smtpd_chat_reply(state, "501 5.7.1 DSN support is disabled");
		return (-1);
	    }
	    if (dsn_notify || (dsn_notify = dsn_notify_mask(arg + 7)) == 0) {
		state->error_mask |= MAIL_ERROR_PROTOCOL;
		smtpd_chat_reply(state,
			    "501 5.5.4 Error: Bad NOTIFY parameter syntax");
		return (-1);
	    }
	} else if (strncasecmp(arg, "ORCPT=", 6) == 0) {	/* RFC 3461 */
	    /* Sanitized by bounce server. */
	    if (state->ehlo_discard_mask & EHLO_MASK_DSN) {
		state->error_mask |= MAIL_ERROR_PROTOCOL;
		smtpd_chat_reply(state, "501 5.7.1 DSN support is disabled");
		return (-1);
	    }
	    vstring_strcpy(state->dsn_orcpt_buf, arg + 6);
	    if (dsn_orcpt_addr
	     || (coded_addr = split_at(STR(state->dsn_orcpt_buf), ';')) == 0
		|| xtext_unquote(state->dsn_buf, coded_addr) == 0
		|| *(dsn_orcpt_type = STR(state->dsn_orcpt_buf)) == 0) {
		state->error_mask |= MAIL_ERROR_PROTOCOL;
		smtpd_chat_reply(state,
			     "501 5.5.4 Error: Bad ORCPT parameter syntax");
		return (-1);
	    }
	    dsn_orcpt_addr = STR(state->dsn_buf);
	    dsn_orcpt_addr_len = LEN(state->dsn_buf);
	} else {
	    state->error_mask |= MAIL_ERROR_PROTOCOL;
	    smtpd_chat_reply(state, "555 5.5.4 Unsupported option: %s", arg);
	    return (-1);
	}
    }
    if (var_smtpd_rcpt_limit && state->rcpt_count >= var_smtpd_rcpt_limit) {
	smtpd_chat_reply(state, "452 4.5.3 Error: too many recipients");
	if (state->rcpt_overshoot++ < var_smtpd_rcpt_overlim)
	    return (0);
	state->error_mask |= MAIL_ERROR_POLICY;
	return (-1);
    }
    if (SMTPD_STAND_ALONE(state) == 0) {
	const char *verify_sender;

	/*
	 * XXX Don't reject the address when we're probed with our own
	 * address verification sender address. Otherwise, some timeout or
	 * some UCE block may result in mutual negative caching, making it
	 * painful to get the mail through. Unfortunately we still have to
	 * send the address to the Milters otherwise they may bail out with a
	 * "missing recipient" protocol error.
	 */
	verify_sender = valid_verify_sender_addr(STR(state->addr_buf));
	if (verify_sender != 0) {
	    vstring_strcpy(state->addr_buf, verify_sender);
	    err = 0;
	} else {
	    err = smtpd_check_rcpt(state, STR(state->addr_buf));
	}
	if (smtpd_milters != 0
	    && (state->saved_flags & MILTER_SKIP_FLAGS) == 0) {
	    PUSH_STRING(saved_rcpt, state->recipient, STR(state->addr_buf));
	    state->milter_reject_text = err;
	    milter_err = milter_rcpt_event(smtpd_milters,
					   err == 0 ? MILTER_FLAG_NONE :
					   MILTER_FLAG_WANT_RCPT_REJ,
				    milter_argv(state, argc - 2, argv + 2));
	    if (err == 0 && milter_err != 0) {
		/* Log reject etc. with correct recipient information. */
		err = check_milter_reply(state, milter_err);
	    }
	    POP_STRING(saved_rcpt, state->recipient);
	}
	if (err != 0) {
	    smtpd_chat_reply(state, "%s", err);
	    return (-1);
	}
    }

    /*
     * Don't access the proxy, queue file, or queue file writer process until
     * we have a valid recipient address.
     */
    if (state->proxy == 0 && state->cleanup == 0 && mail_open_stream(state) < 0)
	return (-1);

    /*
     * Proxy the recipient. OK, so we lied. If the real-time proxy rejects
     * the recipient then we can have a proxy connection without having
     * accepted a recipient.
     */
    proxy = state->proxy;
    if (proxy != 0 && proxy->cmd(state, SMTPD_PROX_WANT_OK,
				 "%s", STR(state->buffer)) != 0) {
	smtpd_chat_reply(state, "%s", STR(proxy->buffer));
	return (-1);
    }

    /*
     * Store the recipient. Remember the first one.
     * 
     * Flush recipients to maintain a stiffer coupling with the next stage and
     * to better utilize parallelism.
     * 
     * RFC 3461 Section 5.2.1: If the NOTIFY parameter was not supplied for a
     * recipient when the message was received, the NOTIFY parameter MUST NOT
     * be supplied for that recipient when the message is relayed.
     * 
     * In other words, we can't simply make up our default NOTIFY value. We have
     * to remember whether the client sent any.
     * 
     * RFC 3461 Section 5.2.1: If no ORCPT parameter was present when the
     * message was received, an ORCPT parameter MAY be added to the RCPT
     * command when the message is relayed.  If an ORCPT parameter is added
     * by the relaying MTA, it MUST contain the recipient address from the
     * RCPT command used when the message was received by that MTA.
     * 
     * In other words, it is OK to make up our own DSN original recipient when
     * the client didn't send one. Although the RFC mentions mail relaying
     * only, we also make up our own original recipient for the purpose of
     * final delivery. For now, we do this here, rather than on the fly.
     * 
     * XXX We use REC_TYPE_ATTR for DSN-related recipient attributes even though
     * 1) REC_TYPE_ATTR is not meant for multiple instances of the same named
     * attribute, and 2) mixing REC_TYPE_ATTR with REC_TYPE_(not attr)
     * requires that we map attributes with rec_attr_map() in order to
     * simplify the recipient record processing loops in the cleanup and qmgr
     * servers.
     * 
     * Another possibility, yet to be explored, is to leave the additional
     * recipient information in the queue file and just pass queue file
     * offsets along with the delivery request. This is a trade off between
     * memory allocation versus numeric conversion overhead.
     * 
     * Since we have no record grouping mechanism, all recipient-specific
     * parameters must be sent to the cleanup server before the actual
     * recipient address.
     */
    state->rcpt_count++;
    if (state->recipient == 0)
	state->recipient = mystrdup(STR(state->addr_buf));
    if (state->cleanup) {
	/* Note: RFC(2)821 externalized address! */
	if (dsn_orcpt_addr == 0) {
	    dsn_orcpt_type = "rfc822";
	    dsn_orcpt_addr = argv[2].strval;
	    dsn_orcpt_addr_len = strlen(argv[2].strval);
	    if (dsn_orcpt_addr[0] == '<'
		&& dsn_orcpt_addr[dsn_orcpt_addr_len - 1] == '>') {
		dsn_orcpt_addr += 1;
		dsn_orcpt_addr_len -= 2;
	    }
	}
	if (dsn_notify)
	    rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%d",
			MAIL_ATTR_DSN_NOTIFY, dsn_notify);
	rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%s;%.*s",
		    MAIL_ATTR_DSN_ORCPT, dsn_orcpt_type,
		    (int) dsn_orcpt_addr_len, dsn_orcpt_addr);
	rec_fputs(state->cleanup, REC_TYPE_RCPT, STR(state->addr_buf));
	vstream_fflush(state->cleanup);
    }
    smtpd_chat_reply(state, "250 2.1.5 Ok");
    return (0);
}

/* rcpt_reset - reset RCPT stuff */

static void rcpt_reset(SMTPD_STATE *state)
{
    if (state->recipient) {
	myfree(state->recipient);
	state->recipient = 0;
    }
    state->rcpt_count = 0;
    /* XXX Must flush the command history. */
    state->rcpt_overshoot = 0;
}

#if 0

/* rfc2047_comment_encode - encode comment string */

static VSTRING *rfc2047_comment_encode(const char *str, const char *charset)
{
    VSTRING *buf = vstring_alloc(30);
    const unsigned char *cp;
    int     ch;

    /*
     * XXX This is problematic code.
     * 
     * XXX Most of the RFC 2047 "especials" are not special in RFC*822 comments,
     * but we encode them anyway to avoid complaints.
     * 
     * XXX In Received: header comments we enclose peer and issuer common names
     * with "" quotes (inherited from the Lutz Jaenicke patch). This is the
     * cause of several quirks.
     * 
     * 1) We encode text that contains the " character, even though that
     * character is not special for RFC*822 comments.
     * 
     * 2) We ignore the recommended limit of 75 characters per encoded word,
     * because long comments look ugly when folded in-between quotes.
     * 
     * 3) We encode the enclosing quotes, to avoid producing invalid encoded
     * words. Microsoft abuses RFC 2047 encoding with attachment names, but
     * we have no information on what decoders do with malformed encoding in
     * comments. This means the comments are Jaenicke-compatible only after
     * decoding.
     */
#define ESPECIALS "()<>@,;:\"/[]?.="		/* Special in RFC 2047 */
#define QSPECIALS "_" ESPECIALS			/* Special in RFC 2047 'Q' */
#define CSPECIALS "\\\"()"			/* Special in our comments */

    /* Don't encode if not needed. */
    for (cp = (unsigned char *) str; /* see below */ ; ++cp) {
	if ((ch = *cp) == 0) {
	    vstring_sprintf(buf, "\"%s\"", str);
	    return (buf);
	}
	if (!ISPRINT(ch) || strchr(CSPECIALS, ch))
	    break;
    }

    /*
     * Use quoted-printable (like) encoding with spaces mapped to underscore.
     */
    vstring_sprintf(buf, "=?%s?Q?=%02X", charset, '"');
    for (cp = (unsigned char *) str; (ch = *cp) != 0; ++cp) {
	if (!ISPRINT(ch) || strchr(QSPECIALS CSPECIALS, ch)) {
	    vstring_sprintf_append(buf, "=%02X", ch);
	} else if (ch == ' ') {
	    VSTRING_ADDCH(buf, '_');
	} else {
	    VSTRING_ADDCH(buf, ch);
	}
    }
    vstring_sprintf_append(buf, "=%02X?=", '"');
    return (buf);
}

#endif

/* comment_sanitize - clean up comment string */

static void comment_sanitize(VSTRING *comment_string)
{
    unsigned char *cp;
    int     ch;
    int     pc;

    /*
     * Postfix Received: headers can be configured to include a comment with
     * the CN (CommonName) of the peer and its issuer, or the login name of a
     * SASL authenticated user. To avoid problems with RFC 822 etc. syntax,
     * we limit this information to printable ASCII text, and neutralize
     * characters that affect comment parsing: the backslash and unbalanced
     * parentheses.
     */
    for (pc = 0, cp = (unsigned char *) STR(comment_string); (ch = *cp) != 0; cp++) {
	if (!ISASCII(ch) || !ISPRINT(ch) || ch == '\\') {
	    *cp = '?';
	} else if (ch == '(') {
	    pc++;
	} else if (ch == ')') {
	    if (pc > 0)
		pc--;
	    else
		*cp = '?';
	}
    }
    while (pc-- > 0)
	VSTRING_ADDCH(comment_string, ')');
    VSTRING_TERMINATE(comment_string);
}

/* data_cmd - process DATA command */

static int data_cmd(SMTPD_STATE *state, int argc, SMTPD_TOKEN *unused_argv)
{
    SMTPD_PROXY *proxy;
    const char *err;
    char   *start;
    int     len;
    int     curr_rec_type;
    int     prev_rec_type;
    int     first = 1;
    VSTRING *why = 0;
    int     saved_err;
    int     (*out_record) (VSTREAM *, int, const char *, ssize_t);
    int     (*out_fprintf) (VSTREAM *, int, const char *,...);
    VSTREAM *out_stream;
    int     out_error;
    char  **cpp;
    const CLEANUP_STAT_DETAIL *detail;
    const char *rfc3848_sess;
    const char *rfc3848_auth;

#ifdef USE_TLS
    VSTRING *peer_CN;
    VSTRING *issuer_CN;

#endif
#ifdef USE_SASL_AUTH
    VSTRING *username;

#endif

    /*
     * Sanity checks. With ESMTP command pipelining the client can send DATA
     * before all recipients are rejected, so don't report that as a protocol
     * error.
     */
    if (state->rcpt_count == 0) {
	if (!IN_MAIL_TRANSACTION(state)) {
	    state->error_mask |= MAIL_ERROR_PROTOCOL;
	    smtpd_chat_reply(state, "503 5.5.1 Error: need RCPT command");
	} else {
	    smtpd_chat_reply(state, "554 5.5.1 Error: no valid recipients");
	}
	return (-1);
    }
    if (argc != 1) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "501 5.5.4 Syntax: DATA");
	return (-1);
    }
    if (SMTPD_STAND_ALONE(state) == 0 && (err = smtpd_check_data(state)) != 0) {
	smtpd_chat_reply(state, "%s", err);
	return (-1);
    }
    if (smtpd_milters != 0
	&& SMTPD_STAND_ALONE(state) == 0
	&& (state->saved_flags & MILTER_SKIP_FLAGS) == 0
	&& (err = milter_data_event(smtpd_milters)) != 0
	&& (err = check_milter_reply(state, err)) != 0) {
	smtpd_chat_reply(state, "%s", err);
	return (-1);
    }
    proxy = state->proxy;
    if (proxy != 0 && proxy->cmd(state, SMTPD_PROX_WANT_MORE,
				 "%s", STR(state->buffer)) != 0) {
	smtpd_chat_reply(state, "%s", STR(proxy->buffer));
	return (-1);
    }

    /*
     * One level of indirection to choose between normal or proxied
     * operation. We want to avoid massive code duplication within tons of
     * if-else clauses.
     */
    if (proxy) {
	out_stream = proxy->stream;
	out_record = proxy->rec_put;
	out_fprintf = proxy->rec_fprintf;
	out_error = CLEANUP_STAT_PROXY;
    } else {
	out_stream = state->cleanup;
	out_record = rec_put;
	out_fprintf = rec_fprintf;
	out_error = CLEANUP_STAT_WRITE;
    }

    /*
     * Flush out a first batch of access table actions that are delegated to
     * the cleanup server, and that may trigger before we accept the first
     * valid recipient. There will be more after end-of-data.
     * 
     * Terminate the message envelope segment. Start the message content
     * segment, and prepend our own Received: header. If there is only one
     * recipient, list the recipient address.
     */
    if (state->cleanup) {
	if (SMTPD_STAND_ALONE(state) == 0) {
	    if (smtpd_milters != 0
		&& (state->saved_flags & MILTER_SKIP_FLAGS) == 0)
		/* Send actual smtpd_milters list. */
		(void) milter_send(smtpd_milters, state->cleanup);
	    if (state->saved_flags)
		rec_fprintf(state->cleanup, REC_TYPE_FLGS, "%d",
			    state->saved_flags);
	}
	rec_fputs(state->cleanup, REC_TYPE_MESG, "");
    }

    /*
     * PREPEND message headers.
     */
    if (state->prepend)
	for (cpp = state->prepend->argv; *cpp; cpp++)
	    out_fprintf(out_stream, REC_TYPE_NORM, "%s", *cpp);

    /*
     * Suppress our own Received: header in the unlikely case that we are an
     * intermediate proxy.
     */
    if (!proxy || state->xforward.flags == 0) {
	out_fprintf(out_stream, REC_TYPE_NORM,
		    "Received: from %s (%s [%s])",
		    state->helo_name ? state->helo_name : state->name,
		    state->name, state->rfc_addr);

#define VSTRING_STRDUP(s) vstring_strcpy(vstring_alloc(strlen(s) + 1), (s))

#ifdef USE_TLS
	if (var_smtpd_tls_received_header && state->tls_context) {
	    out_fprintf(out_stream, REC_TYPE_NORM,
			"\t(using %s with cipher %s (%d/%d bits))",
			state->tls_context->protocol,
			state->tls_context->cipher_name,
			state->tls_context->cipher_usebits,
			state->tls_context->cipher_algbits);
	    if (TLS_CERT_IS_PRESENT(state->tls_context)) {
		peer_CN = VSTRING_STRDUP(state->tls_context->peer_CN);
		comment_sanitize(peer_CN);
		issuer_CN = VSTRING_STRDUP(state->tls_context->issuer_CN ?
					state->tls_context->issuer_CN : "");
		comment_sanitize(issuer_CN);
		out_fprintf(out_stream, REC_TYPE_NORM,
			    "\t(Client CN \"%s\", Issuer \"%s\" (%s))",
			    STR(peer_CN), STR(issuer_CN),
			    TLS_CERT_IS_TRUSTED(state->tls_context) ?
			    "verified OK" : "not verified");
		vstring_free(issuer_CN);
		vstring_free(peer_CN);
	    } else if (var_smtpd_tls_ask_ccert)
		out_fprintf(out_stream, REC_TYPE_NORM,
			    "\t(Client did not present a certificate)");
	    else
		out_fprintf(out_stream, REC_TYPE_NORM,
			    "\t(No client certificate requested)");
	}
	/* RFC 3848 is defined for ESMTP only. */
	if (state->tls_context != 0
	    && strcmp(state->protocol, MAIL_PROTO_ESMTP) == 0)
	    rfc3848_sess = "S";
	else
#endif
	    rfc3848_sess = "";
#ifdef USE_SASL_AUTH
	if (var_smtpd_sasl_auth_hdr && state->sasl_username) {
	    username = VSTRING_STRDUP(state->sasl_username);
	    comment_sanitize(username);
	    out_fprintf(out_stream, REC_TYPE_NORM,
			"\t(Authenticated sender: %s)", STR(username));
	    vstring_free(username);
	}
	/* RFC 3848 is defined for ESMTP only. */
	if (state->sasl_username
	    && strcmp(state->protocol, MAIL_PROTO_ESMTP) == 0)
	    rfc3848_auth = "A";
	else
#endif
	    rfc3848_auth = "";
	if (state->rcpt_count == 1 && state->recipient) {
	    out_fprintf(out_stream, REC_TYPE_NORM,
			state->cleanup ? "\tby %s (%s) with %s%s%s id %s" :
			"\tby %s (%s) with %s%s%s",
			var_myhostname, var_mail_name,
			state->protocol, rfc3848_sess,
			rfc3848_auth, state->queue_id);
	    quote_822_local(state->buffer, state->recipient);
	    out_fprintf(out_stream, REC_TYPE_NORM,
			"\tfor <%s>; %s", STR(state->buffer),
			mail_date(state->arrival_time.tv_sec));
	} else {
	    out_fprintf(out_stream, REC_TYPE_NORM,
			state->cleanup ? "\tby %s (%s) with %s%s%s id %s;" :
			"\tby %s (%s) with %s%s%s;",
			var_myhostname, var_mail_name,
			state->protocol, rfc3848_sess,
			rfc3848_auth, state->queue_id);
	    out_fprintf(out_stream, REC_TYPE_NORM,
			"\t%s", mail_date(state->arrival_time.tv_sec));
	}
#ifdef RECEIVED_ENVELOPE_FROM
	quote_822_local(state->buffer, state->sender);
	out_fprintf(out_stream, REC_TYPE_NORM,
		    "\t(envelope-from %s)", STR(state->buffer));
#endif
    }
    smtpd_chat_reply(state, "354 End data with <CR><LF>.<CR><LF>");
    state->where = SMTPD_AFTER_DATA;

    /*
     * Copy the message content. If the cleanup process has a problem, keep
     * reading until the remote stops sending, then complain. Produce typed
     * records from the SMTP stream so we can handle data that spans buffers.
     * 
     * XXX Force an empty record when the queue file content begins with
     * whitespace, so that it won't be considered as being part of our own
     * Received: header. What an ugly Kluge.
     * 
     * XXX Deal with UNIX-style From_ lines at the start of message content
     * because sendmail permits it.
     */
    for (prev_rec_type = 0; /* void */ ; prev_rec_type = curr_rec_type) {
	if (smtp_get(state->buffer, state->client, var_line_limit,
		     SMTP_GET_FLAG_NONE) == '\n')
	    curr_rec_type = REC_TYPE_NORM;
	else
	    curr_rec_type = REC_TYPE_CONT;
	start = vstring_str(state->buffer);
	len = VSTRING_LEN(state->buffer);
	if (first) {
	    if (strncmp(start + strspn(start, ">"), "From ", 5) == 0) {
		out_fprintf(out_stream, curr_rec_type,
			    "X-Mailbox-Line: %s", start);
		continue;
	    }
	    first = 0;
	    if (len > 0 && IS_SPACE_TAB(start[0]))
		out_record(out_stream, REC_TYPE_NORM, "", 0);
	}
	if (prev_rec_type != REC_TYPE_CONT && *start == '.'
	    && (proxy == 0 ? (++start, --len) == 0 : len == 1))
	    break;
	if (state->err == CLEANUP_STAT_OK) {
	    if (var_message_limit > 0 && var_message_limit - state->act_size < len + 2) {
		state->err = CLEANUP_STAT_SIZE;
		msg_warn("%s: queue file size limit exceeded",
			 state->queue_id ? state->queue_id : "NOQUEUE");
	    } else {
		state->act_size += len + 2;
		if (out_record(out_stream, curr_rec_type, start, len) < 0)
		    state->err = out_error;
	    }
	}
    }
    state->where = SMTPD_AFTER_DOT;
    if (state->err == CLEANUP_STAT_OK
	&& SMTPD_STAND_ALONE(state) == 0
	&& (err = smtpd_check_eod(state)) != 0) {
	smtpd_chat_reply(state, "%s", err);
	if (proxy) {
	    smtpd_proxy_close(state);
	} else {
	    mail_stream_cleanup(state->dest);
	    state->dest = 0;
	    state->cleanup = 0;
	}
	return (-1);
    }

    /*
     * Send the end of DATA and finish the proxy connection. Set the
     * CLEANUP_STAT_PROXY error flag in case of trouble.
     */
    if (proxy) {
	if (state->err == CLEANUP_STAT_OK) {
	    (void) proxy->cmd(state, SMTPD_PROX_WANT_ANY, ".");
	    if (state->err == CLEANUP_STAT_OK &&
		*STR(proxy->buffer) != '2')
		state->err = CLEANUP_STAT_CONT;
	}
    }

    /*
     * Flush out access table actions that are delegated to the cleanup
     * server. There is similar code at the beginning of the DATA command.
     * 
     * Send the end-of-segment markers and finish the queue file record stream.
     */
    else {
	if (state->err == CLEANUP_STAT_OK) {
	    rec_fputs(state->cleanup, REC_TYPE_XTRA, "");
	    if (state->saved_filter)
		rec_fprintf(state->cleanup, REC_TYPE_FILT, "%s",
			    state->saved_filter);
	    if (state->saved_redirect)
		rec_fprintf(state->cleanup, REC_TYPE_RDR, "%s",
			    state->saved_redirect);
	    if (state->saved_bcc) {
		rec_fprintf(state->cleanup, REC_TYPE_RCPT, "%s",
			    state->saved_bcc);
		rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%d",
			    MAIL_ATTR_DSN_NOTIFY, DSN_NOTIFY_NEVER);
	    }
	    if (state->saved_flags)
		rec_fprintf(state->cleanup, REC_TYPE_FLGS, "%d",
			    state->saved_flags);
#ifdef DELAY_ACTION
	    if (state->saved_delay)
		rec_fprintf(state->cleanup, REC_TYPE_DELAY, "%d",
			    state->saved_delay);
#endif
	    if (vstream_ferror(state->cleanup))
		state->err = CLEANUP_STAT_WRITE;
	}
	if (state->err == CLEANUP_STAT_OK)
	    if (rec_fputs(state->cleanup, REC_TYPE_END, "") < 0
		|| vstream_fflush(state->cleanup))
		state->err = CLEANUP_STAT_WRITE;
	if (state->err == 0) {
	    why = vstring_alloc(10);
	    state->err = mail_stream_finish(state->dest, why);
	    printable(STR(why), ' ');
	} else
	    mail_stream_cleanup(state->dest);
	state->dest = 0;
	state->cleanup = 0;
    }

    /*
     * XXX If we lose the cleanup server while it is editing a queue file,
     * the Postfix SMTP server will be out of sync with Milter applications.
     * Sending an ABORT to the Milters is not sufficient to restore
     * synchronization, because there may be any number of Milter replies
     * already in flight. Destroying and recreating the Milters (and faking
     * the connect and ehlo events) is too much trouble for testing and
     * maintenance. Workaround: force the Postfix SMTP server to hang up with
     * a 421 response in the rare case that the cleanup server breaks AND
     * that the remote SMTP client continues the session after end-of-data.
     * 
     * XXX Should use something other than CLEANUP_STAT_WRITE when we lose
     * contact with the cleanup server. This requires changes to the
     * mail_stream module and its users (smtpd, qmqpd, perhaps sendmail).
     * 
     * XXX See exception below in code that overrides state->access_denied for
     * compliance with RFC 2821 Sec 3.1.
     */
    if (smtpd_milters != 0 && (state->err & CLEANUP_STAT_WRITE) != 0)
	state->access_denied = mystrdup("421 4.3.0 Mail system error");

    /*
     * Handle any errors. One message may suffer from multiple errors, so
     * complain only about the most severe error. Forgive any previous client
     * errors when a message was received successfully.
     * 
     * See also: qmqpd.c
     */
#define IS_SMTP_REJECT(s) \
	(((s)[0] == '4' || (s)[0] == '5') \
	 && ISDIGIT((s)[1]) && ISDIGIT((s)[2]) \
	 && ((s)[3] == '\0' || (s)[3] == ' ' || (s)[3] == '-'))

    if (state->err == CLEANUP_STAT_OK) {
	state->error_count = 0;
	state->error_mask = 0;
	state->junk_cmds = 0;
	if (proxy)
	    smtpd_chat_reply(state, "%s", STR(proxy->buffer));
	else
	    smtpd_chat_reply(state,
			     "250 2.0.0 Ok: queued as %s", state->queue_id);
    } else if (why && IS_SMTP_REJECT(STR(why))) {
	state->error_mask |= MAIL_ERROR_POLICY;
	smtpd_chat_reply(state, "%s", STR(why));
    } else if ((state->err & CLEANUP_STAT_DEFER) != 0) {
	state->error_mask |= MAIL_ERROR_POLICY;
	detail = cleanup_stat_detail(CLEANUP_STAT_DEFER);
	if (why && LEN(why) > 0) {
	    /* Allow address-specific DSN status in header/body_checks. */
	    smtpd_chat_reply(state, "%d %s", detail->smtp, STR(why));
	} else {
	    smtpd_chat_reply(state, "%d %s Error: %s",
			     detail->smtp, detail->dsn, detail->text);
	}
    } else if ((state->err & CLEANUP_STAT_BAD) != 0) {
	state->error_mask |= MAIL_ERROR_SOFTWARE;
	detail = cleanup_stat_detail(CLEANUP_STAT_BAD);
	smtpd_chat_reply(state, "%d %s Error: internal error %d",
			 detail->smtp, detail->dsn, state->err);
    } else if ((state->err & CLEANUP_STAT_SIZE) != 0) {
	state->error_mask |= MAIL_ERROR_BOUNCE;
	detail = cleanup_stat_detail(CLEANUP_STAT_SIZE);
	smtpd_chat_reply(state, "%d %s Error: %s",
			 detail->smtp, detail->dsn, detail->text);
    } else if ((state->err & CLEANUP_STAT_HOPS) != 0) {
	state->error_mask |= MAIL_ERROR_BOUNCE;
	detail = cleanup_stat_detail(CLEANUP_STAT_HOPS);
	smtpd_chat_reply(state, "%d %s Error: %s",
			 detail->smtp, detail->dsn, detail->text);
    } else if ((state->err & CLEANUP_STAT_CONT) != 0) {
	state->error_mask |= MAIL_ERROR_POLICY;
	detail = cleanup_stat_detail(CLEANUP_STAT_CONT);
	if (proxy) {
	    smtpd_chat_reply(state, "%s", STR(proxy->buffer));
	} else if (why && LEN(why) > 0) {
	    /* Allow address-specific DSN status in header/body_checks. */
	    smtpd_chat_reply(state, "%d %s", detail->smtp, STR(why));
	} else {
	    smtpd_chat_reply(state, "%d %s Error: %s",
			     detail->smtp, detail->dsn, detail->text);
	}
    } else if ((state->err & CLEANUP_STAT_WRITE) != 0) {
	state->error_mask |= MAIL_ERROR_RESOURCE;
	detail = cleanup_stat_detail(CLEANUP_STAT_WRITE);
	smtpd_chat_reply(state, "%d %s Error: %s",
			 detail->smtp, detail->dsn, detail->text);
    } else if ((state->err & CLEANUP_STAT_PROXY) != 0) {
	state->error_mask |= MAIL_ERROR_SOFTWARE;
	smtpd_chat_reply(state, "%s", STR(proxy->buffer));
    } else {
	state->error_mask |= MAIL_ERROR_SOFTWARE;
	detail = cleanup_stat_detail(CLEANUP_STAT_BAD);
	smtpd_chat_reply(state, "%d %s Error: internal error %d",
			 detail->smtp, detail->dsn, state->err);
    }

    /*
     * By popular command: the proxy's end-of-data reply.
     */
    if (proxy)
	msg_info("proxy-%s: %s: %s;%s",
		 (state->err == CLEANUP_STAT_OK) ? "accept" : "reject",
		 state->where, STR(proxy->buffer), smtpd_whatsup(state));

    /*
     * Cleanup. The client may send another MAIL command.
     */
    saved_err = state->err;
    chat_reset(state, var_smtpd_hist_thrsh);
    mail_reset(state);
    rcpt_reset(state);
    if (why)
	vstring_free(why);
    return (saved_err);
}

/* rset_cmd - process RSET */

static int rset_cmd(SMTPD_STATE *state, int argc, SMTPD_TOKEN *unused_argv)
{

    /*
     * Sanity checks.
     */
    if (argc != 1) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "501 5.5.4 Syntax: RSET");
	return (-1);
    }

    /*
     * Restore state to right after HELO/EHLO command.
     */
    chat_reset(state, var_smtpd_hist_thrsh);
    mail_reset(state);
    rcpt_reset(state);
    smtpd_chat_reply(state, "250 2.0.0 Ok");
    return (0);
}

/* noop_cmd - process NOOP */

static int noop_cmd(SMTPD_STATE *state, int argc, SMTPD_TOKEN *unused_argv)
{

    /*
     * XXX 2821 incompatibility: Section 4.1.1.9 says that NOOP can have a
     * parameter string which is to be ignored. NOOP instructions with
     * parameters? Go figure.
     * 
     * RFC 2821 violates RFC 821, which says that NOOP takes no parameters.
     */
#ifdef RFC821_SYNTAX

    /*
     * Sanity checks.
     */
    if (argc != 1) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "501 5.5.4 Syntax: NOOP");
	return (-1);
    }
#endif
    smtpd_chat_reply(state, "250 2.0.0 Ok");
    return (0);
}

/* vrfy_cmd - process VRFY */

static int vrfy_cmd(SMTPD_STATE *state, int argc, SMTPD_TOKEN *argv)
{
    const char *err = 0;

    /*
     * The SMTP standard (RFC 821) disallows unquoted special characters in
     * the VRFY argument. Common practice violates the standard, however.
     * Postfix accomodates common practice where it violates the standard.
     * 
     * XXX Impedance mismatch! The SMTP command tokenizer preserves quoting,
     * whereas the recipient restrictions checks expect unquoted (internal)
     * address forms. Therefore we must parse out the address, or we must
     * stop doing recipient restriction checks and lose the opportunity to
     * say "user unknown" at the SMTP port.
     * 
     * XXX 2821 incompatibility and brain damage: Section 4.5.1 requires that
     * VRFY is implemented. RFC 821 specifies that VRFY is optional. It gets
     * even worse: section 3.5.3 says that a 502 (command recognized but not
     * implemented) reply is not fully compliant.
     * 
     * Thus, an RFC 2821 compliant implementation cannot refuse to supply
     * information in reply to VRFY queries. That is simply bogus. The only
     * reply we could supply is a generic 252 reply. This causes spammers to
     * add tons of bogus addresses to their mailing lists (spam harvesting by
     * trying out large lists of potential recipient names with VRFY).
     */
#define SLOPPY	0

    if (var_disable_vrfy_cmd) {
	state->error_mask |= MAIL_ERROR_POLICY;
	smtpd_chat_reply(state, "502 5.5.1 VRFY command is disabled");
	return (-1);
    }
    if (smtpd_milters != 0 && (err = milter_other_event(smtpd_milters)) != 0
	&& (err[0] == '5' || err[0] == '4')) {
	state->error_mask |= MAIL_ERROR_POLICY;
	smtpd_chat_reply(state, "%s", err);
	return (-1);
    }
    if (argc < 2) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "501 5.5.4 Syntax: VRFY address");
	return (-1);
    }
    if (argc > 2)
	collapse_args(argc - 1, argv + 1);
    if (extract_addr(state, argv + 1, REJECT_EMPTY_ADDR, SLOPPY) != 0) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "501 5.1.3 Bad recipient address syntax");
	return (-1);
    }
    /* Use state->addr_buf, with the unquoted result from extract_addr() */
    if (SMTPD_STAND_ALONE(state) == 0
	&& (err = smtpd_check_rcpt(state, STR(state->addr_buf))) != 0) {
	smtpd_chat_reply(state, "%s", err);
	return (-1);
    }

    /*
     * XXX 2821 new feature: Section 3.5.1 requires that the VRFY response is
     * either "full name <user@domain>" or "user@domain". Postfix replies
     * with the string that was provided by the client, whether or not it is
     * in fully qualified domain form and the address is in <>.
     * 
     * Reply code 250 is reserved for the case where the address is verified;
     * reply code 252 should be used when no definitive certainty exists.
     */
    smtpd_chat_reply(state, "252 2.0.0 %s", argv[1].strval);
    return (0);
}

/* etrn_cmd - process ETRN command */

static int etrn_cmd(SMTPD_STATE *state, int argc, SMTPD_TOKEN *argv)
{
    const char *err;

    /*
     * Sanity checks.
     */
    if (var_helo_required && state->helo_name == 0) {
	state->error_mask |= MAIL_ERROR_POLICY;
	smtpd_chat_reply(state, "503 Error: send HELO/EHLO first");
	return (-1);
    }
    if (smtpd_milters != 0 && (err = milter_other_event(smtpd_milters)) != 0
	&& (err[0] == '5' || err[0] == '4')) {
	state->error_mask |= MAIL_ERROR_POLICY;
	smtpd_chat_reply(state, "%s", err);
	return (-1);
    }
    if (IN_MAIL_TRANSACTION(state)) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "503 Error: MAIL transaction in progress");
	return (-1);
    }
    if (argc != 2) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "500 Syntax: ETRN domain");
	return (-1);
    }
    if (argv[1].strval[0] == '@' || argv[1].strval[0] == '#')
	argv[1].strval++;

    /*
     * As an extension to RFC 1985 we also allow an RFC 2821 address literal
     * enclosed in [].
     */
    if (!valid_hostname(argv[1].strval, DONT_GRIPE)
	&& !valid_mailhost_literal(argv[1].strval, DONT_GRIPE)) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "501 Error: invalid parameter syntax");
	return (-1);
    }

    /*
     * XXX The implementation borrows heavily from the code that implements
     * UCE restrictions. These typically return 450 or 550 when a request is
     * rejected. RFC 1985 requires that 459 be sent when the server refuses
     * to perform the request.
     */
    if (SMTPD_STAND_ALONE(state)) {
	msg_warn("do not use ETRN in \"sendmail -bs\" mode");
	smtpd_chat_reply(state, "458 Unable to queue messages");
	return (-1);
    }
    if ((err = smtpd_check_etrn(state, argv[1].strval)) != 0) {
	smtpd_chat_reply(state, "%s", err);
	return (-1);
    }
    switch (flush_send_site(argv[1].strval)) {
    case FLUSH_STAT_OK:
	smtpd_chat_reply(state, "250 Queuing started");
	return (0);
    case FLUSH_STAT_DENY:
	msg_warn("reject: ETRN %.100s... from %s",
		 argv[1].strval, state->namaddr);
	smtpd_chat_reply(state, "459 <%s>: service unavailable",
			 argv[1].strval);
	return (-1);
    case FLUSH_STAT_BAD:
	msg_warn("bad ETRN %.100s... from %s", argv[1].strval, state->namaddr);
	smtpd_chat_reply(state, "458 Unable to queue messages");
	return (-1);
    default:
	msg_warn("unable to talk to fast flush service");
	smtpd_chat_reply(state, "458 Unable to queue messages");
	return (-1);
    }
}

/* quit_cmd - process QUIT command */

static int quit_cmd(SMTPD_STATE *state, int unused_argc, SMTPD_TOKEN *unused_argv)
{
    int     out_pending = vstream_bufstat(state->client, VSTREAM_BST_OUT_PEND);

    /*
     * Don't bother checking the syntax.
     */
    smtpd_chat_reply(state, "221 2.0.0 Bye");

    /*
     * When the "." and quit replies are pipelined, make sure they are
     * flushed now, to avoid repeated mail deliveries in case of a crash in
     * the "clean up before disconnect" code.
     * 
     * XXX When this was added in Postfix 2.1 we used vstream_fflush(). As of
     * Postfix 2.3 we use smtp_flush() for better error reporting.
     */
    if (out_pending > 0)
	smtp_flush(state->client);
    return (0);
}

/* xclient_cmd - override SMTP client attributes */

static int xclient_cmd(SMTPD_STATE *state, int argc, SMTPD_TOKEN *argv)
{
    SMTPD_TOKEN *argp;
    char   *raw_value;
    char   *attr_value;
    const char *bare_value;
    char   *attr_name;
    int     update_namaddr = 0;
    int     name_status;
    static const NAME_CODE peer_codes[] = {
	XCLIENT_UNAVAILABLE, SMTPD_PEER_CODE_PERM,
	XCLIENT_TEMPORARY, SMTPD_PEER_CODE_TEMP,
	0, SMTPD_PEER_CODE_OK,
    };
    static const NAME_CODE proto_names[] = {
	MAIL_PROTO_SMTP, 1,
	MAIL_PROTO_ESMTP, 2,
	0, -1,
    };
    int     got_helo = 0;
    int     got_proto = 0;
    int     got_login = 0;

    /*
     * Sanity checks.
     * 
     * XXX The XCLIENT command will override its own access control, so that
     * connection count/rate restrictions can be correctly simulated.
     */
    if (IN_MAIL_TRANSACTION(state)) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "503 5.5.1 Error: MAIL transaction in progress");
	return (-1);
    }
    if (argc < 2) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "501 5.5.4 Syntax: %s attribute=value...",
			 XCLIENT_CMD);
	return (-1);
    }
    if (xclient_hosts && xclient_hosts->error)
	cant_permit_command(state, XCLIENT_CMD);
    if (!xclient_allowed) {
	state->error_mask |= MAIL_ERROR_POLICY;
	smtpd_chat_reply(state, "550 5.7.0 Error: insufficient authorization");
	return (-1);
    }
#define STREQ(x,y)	(strcasecmp((x), (y)) == 0)
#define UPDATE_STR(s, v) do { \
	    const char *_v = (v); \
	    if (s) myfree(s); \
	    s = (_v) ? mystrdup(_v) : 0; \
	} while(0)

    /*
     * Initialize.
     */
    if (state->expand_buf == 0)
	state->expand_buf = vstring_alloc(100);

    /*
     * Iterate over all attribute=value elements.
     */
    for (argp = argv + 1; argp < argv + argc; argp++) {
	attr_name = argp->strval;

	if ((raw_value = split_at(attr_name, '=')) == 0 || *raw_value == 0) {
	    state->error_mask |= MAIL_ERROR_PROTOCOL;
	    smtpd_chat_reply(state, "501 5.5.4 Error: attribute=value expected");
	    return (-1);
	}
	if (strlen(raw_value) > 255) {
	    state->error_mask |= MAIL_ERROR_PROTOCOL;
	    smtpd_chat_reply(state, "501 5.5.4 Error: attribute value too long");
	    return (-1);
	}

	/*
	 * Backwards compatibility: Postfix prior to version 2.3 does not
	 * xtext encode attribute values.
	 */
	attr_value = xtext_unquote(state->expand_buf, raw_value) ?
	    STR(state->expand_buf) : raw_value;

	/*
	 * For safety's sake mask non-printable characters. We'll do more
	 * specific censoring later.
	 */
	printable(attr_value, '?');

	/*
	 * NAME=substitute SMTP client hostname (and reverse/forward name, in
	 * case of success). Also updates the client hostname lookup status
	 * code.
	 */
	if (STREQ(attr_name, XCLIENT_NAME)) {
	    name_status = name_code(peer_codes, NAME_CODE_FLAG_NONE, attr_value);
	    if (name_status != SMTPD_PEER_CODE_OK) {
		attr_value = CLIENT_NAME_UNKNOWN;
	    } else {
		if (!valid_hostname(attr_value, DONT_GRIPE)) {
		    state->error_mask |= MAIL_ERROR_PROTOCOL;
		    smtpd_chat_reply(state, "501 5.5.4 Bad %s syntax: %s",
				     XCLIENT_NAME, attr_value);
		    return (-1);
		}
	    }
	    state->name_status = name_status;
	    UPDATE_STR(state->name, attr_value);
	    update_namaddr = 1;
	    if (name_status == SMTPD_PEER_CODE_OK) {
		UPDATE_STR(state->reverse_name, attr_value);
		state->reverse_name_status = name_status;
	    }
	}

	/*
	 * REVERSE_NAME=substitute SMTP client reverse hostname. Also updates
	 * the client reverse hostname lookup status code.
	 */
	else if (STREQ(attr_name, XCLIENT_REVERSE_NAME)) {
	    name_status = name_code(peer_codes, NAME_CODE_FLAG_NONE, attr_value);
	    if (name_status != SMTPD_PEER_CODE_OK) {
		attr_value = CLIENT_NAME_UNKNOWN;
	    } else {
		if (!valid_hostname(attr_value, DONT_GRIPE)) {
		    state->error_mask |= MAIL_ERROR_PROTOCOL;
		    smtpd_chat_reply(state, "501 5.5.4 Bad %s syntax: %s",
				     XCLIENT_REVERSE_NAME, attr_value);
		    return (-1);
		}
	    }
	    state->reverse_name_status = name_status;
	    UPDATE_STR(state->reverse_name, attr_value);
	}

	/*
	 * ADDR=substitute SMTP client network address.
	 */
	else if (STREQ(attr_name, XCLIENT_ADDR)) {
	    if (STREQ(attr_value, XCLIENT_UNAVAILABLE)) {
		attr_value = CLIENT_ADDR_UNKNOWN;
		bare_value = attr_value;
	    } else {
		if ((bare_value = valid_mailhost_addr(attr_value, DONT_GRIPE)) == 0) {
		    state->error_mask |= MAIL_ERROR_PROTOCOL;
		    smtpd_chat_reply(state, "501 5.5.4 Bad %s syntax: %s",
				     XCLIENT_ADDR, attr_value);
		    return (-1);
		}
	    }
	    UPDATE_STR(state->addr, bare_value);
	    UPDATE_STR(state->rfc_addr, attr_value);
#ifdef HAS_IPV6
	    if (strncasecmp(attr_value, INET_PROTO_NAME_IPV6 ":",
			    sizeof(INET_PROTO_NAME_IPV6 ":") - 1) == 0)
		state->addr_family = AF_INET6;
	    else
#endif
		state->addr_family = AF_INET;
	    update_namaddr = 1;
	}

	/*
	 * PORT=substitute SMTP client port number.
	 */
	else if (STREQ(attr_name, XCLIENT_PORT)) {
	    if (STREQ(attr_value, XCLIENT_UNAVAILABLE)) {
		attr_value = CLIENT_PORT_UNKNOWN;
	    } else {
		if (!alldig(attr_value)
		    || strlen(attr_value) > sizeof("65535") - 1) {
		    state->error_mask |= MAIL_ERROR_PROTOCOL;
		    smtpd_chat_reply(state, "501 5.5.4 Bad %s syntax: %s",
				     XCLIENT_PORT, attr_value);
		    return (-1);
		}
	    }
	    UPDATE_STR(state->port, attr_value);
	    update_namaddr = 1;
	}

	/*
	 * HELO=substitute SMTP client HELO parameter. Censor special
	 * characters that could mess up message headers.
	 */
	else if (STREQ(attr_name, XCLIENT_HELO)) {
	    if (STREQ(attr_value, XCLIENT_UNAVAILABLE)) {
		attr_value = CLIENT_HELO_UNKNOWN;
	    } else {
		if (strlen(attr_value) > VALID_HOSTNAME_LEN) {
		    state->error_mask |= MAIL_ERROR_PROTOCOL;
		    smtpd_chat_reply(state, "501 5.5.4 Bad %s syntax: %s",
				     XCLIENT_HELO, attr_value);
		    return (-1);
		}
		neuter(attr_value, NEUTER_CHARACTERS, '?');
	    }
	    UPDATE_STR(state->helo_name, attr_value);
	    got_helo = 1;
	}

	/*
	 * PROTO=SMTP protocol name.
	 */
	else if (STREQ(attr_name, XCLIENT_PROTO)) {
	    if (name_code(proto_names, NAME_CODE_FLAG_NONE, attr_value) < 0) {
		state->error_mask |= MAIL_ERROR_PROTOCOL;
		smtpd_chat_reply(state, "501 5.5.4 Bad %s syntax: %s",
				 XCLIENT_PROTO, attr_value);
		return (-1);
	    }
	    UPDATE_STR(state->protocol, uppercase(attr_value));
	    got_proto = 1;
	}

	/*
	 * LOGIN=sasl_username. Sets the authentication method as XCLIENT.
	 * This can be used even if SASL authentication is turned off in
	 * main.cf. We can't make it easier than that.
	 */
#ifdef USE_SASL_AUTH
	else if (STREQ(attr_name, XCLIENT_LOGIN)) {
	    if (STREQ(attr_value, XCLIENT_UNAVAILABLE) == 0) {
		smtpd_sasl_auth_extern(state, attr_value, XCLIENT_CMD);
		got_login = 1;
	    }
	}
#endif

	/*
	 * Unknown attribute name. Complain.
	 */
	else {
	    state->error_mask |= MAIL_ERROR_PROTOCOL;
	    smtpd_chat_reply(state, "501 5.5.4 Bad %s attribute name: %s",
			     XCLIENT_CMD, attr_name);
	    return (-1);
	}
    }

    /*
     * Update the combined name and address when either has changed.
     */
    if (update_namaddr) {
	if (state->namaddr)
	    myfree(state->namaddr);
	state->namaddr =
	    SMTPD_BUILD_NAMADDRPORT(state->name, state->addr, state->port);
    }

    /*
     * XXX Compatibility: when the client issues XCLIENT then we have to go
     * back to initial server greeting stage, otherwise we can't correctly
     * simulate smtpd_client_restrictions (with smtpd_delay_reject=0) and
     * Milter connect restrictions.
     * 
     * XXX Compatibility: for accurate simulation we must also reset the HELO
     * information. We keep the information if it was specified in the
     * XCLIENT command.
     * 
     * XXX The client connection count/rate control must be consistent in its
     * use of client address information in connect and disconnect events. We
     * re-evaluate xclient so that we correctly simulate connection
     * concurrency and connection rate restrictions.
     * 
     * XXX Duplicated from smtpd_proto().
     */
    xclient_allowed =
	namadr_list_match(xclient_hosts, state->name, state->addr);
    /* NOT: tls_reset() */
    if (got_helo == 0)
	helo_reset(state);
    if (got_proto == 0 && strcasecmp(state->protocol, MAIL_PROTO_SMTP) != 0) {
	myfree(state->protocol);
	state->protocol = mystrdup(MAIL_PROTO_SMTP);
    }
#ifdef USE_SASL_AUTH
    if (got_login == 0)
	smtpd_sasl_auth_reset(state);
#endif
    chat_reset(state, 0);
    mail_reset(state);
    rcpt_reset(state);
    if (smtpd_milters)
	milter_disc_event(smtpd_milters);
    vstream_longjmp(state->client, SMTP_ERR_NONE);
    return (0);
}

/* xforward_cmd - forward logging attributes */

static int xforward_cmd(SMTPD_STATE *state, int argc, SMTPD_TOKEN *argv)
{
    SMTPD_TOKEN *argp;
    char   *raw_value;
    char   *attr_value;
    const char *bare_value;
    char   *attr_name;
    int     updated = 0;
    static const NAME_CODE xforward_flags[] = {
	XFORWARD_NAME, SMTPD_STATE_XFORWARD_NAME,
	XFORWARD_ADDR, SMTPD_STATE_XFORWARD_ADDR,
	XFORWARD_PORT, SMTPD_STATE_XFORWARD_PORT,
	XFORWARD_PROTO, SMTPD_STATE_XFORWARD_PROTO,
	XFORWARD_HELO, SMTPD_STATE_XFORWARD_HELO,
	XFORWARD_IDENT, SMTPD_STATE_XFORWARD_IDENT,
	XFORWARD_DOMAIN, SMTPD_STATE_XFORWARD_DOMAIN,
	0, 0,
    };
    static const char *context_name[] = {
	MAIL_ATTR_RWR_LOCAL,		/* Postfix internal form */
	MAIL_ATTR_RWR_REMOTE,		/* Postfix internal form */
    };
    static const NAME_CODE xforward_to_context[] = {
	XFORWARD_DOM_LOCAL, 0,		/* XFORWARD representation */
	XFORWARD_DOM_REMOTE, 1,		/* XFORWARD representation */
	0, -1,
    };
    int     flag;
    int     context_code;

    /*
     * Sanity checks.
     */
    if (IN_MAIL_TRANSACTION(state)) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "503 5.5.1 Error: MAIL transaction in progress");
	return (-1);
    }
    if (argc < 2) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "501 5.5.4 Syntax: %s attribute=value...",
			 XFORWARD_CMD);
	return (-1);
    }
    if (xforward_hosts && xforward_hosts->error)
	cant_permit_command(state, XFORWARD_CMD);
    if (!xforward_allowed) {
	state->error_mask |= MAIL_ERROR_POLICY;
	smtpd_chat_reply(state, "550 5.7.0 Error: insufficient authorization");
	return (-1);
    }

    /*
     * Initialize.
     */
    if (state->xforward.flags == 0)
	smtpd_xforward_preset(state);
    if (state->expand_buf == 0)
	state->expand_buf = vstring_alloc(100);

    /*
     * Iterate over all attribute=value elements.
     */
    for (argp = argv + 1; argp < argv + argc; argp++) {
	attr_name = argp->strval;

	if ((raw_value = split_at(attr_name, '=')) == 0 || *raw_value == 0) {
	    state->error_mask |= MAIL_ERROR_PROTOCOL;
	    smtpd_chat_reply(state, "501 5.5.4 Error: attribute=value expected");
	    return (-1);
	}
	if (strlen(raw_value) > 255) {
	    state->error_mask |= MAIL_ERROR_PROTOCOL;
	    smtpd_chat_reply(state, "501 5.5.4 Error: attribute value too long");
	    return (-1);
	}

	/*
	 * Backwards compatibility: Postfix prior to version 2.3 does not
	 * xtext encode attribute values.
	 */
	attr_value = xtext_unquote(state->expand_buf, raw_value) ?
	    STR(state->expand_buf) : raw_value;

	/*
	 * For safety's sake mask non-printable characters. We'll do more
	 * specific censoring later.
	 */
	printable(attr_value, '?');

	flag = name_code(xforward_flags, NAME_CODE_FLAG_NONE, attr_name);
	switch (flag) {

	    /*
	     * NAME=up-stream host name, not necessarily in the DNS. Censor
	     * special characters that could mess up message headers.
	     */
	case SMTPD_STATE_XFORWARD_NAME:
	    if (STREQ(attr_value, XFORWARD_UNAVAILABLE)) {
		attr_value = CLIENT_NAME_UNKNOWN;
	    } else {
		neuter(attr_value, NEUTER_CHARACTERS, '?');
		if (!valid_hostname(attr_value, DONT_GRIPE)) {
		    state->error_mask |= MAIL_ERROR_PROTOCOL;
		    smtpd_chat_reply(state, "501 5.5.4 Bad %s syntax: %s",
				     XFORWARD_NAME, attr_value);
		    return (-1);
		}
	    }
	    UPDATE_STR(state->xforward.name, attr_value);
	    break;

	    /*
	     * ADDR=up-stream host network address, not necessarily on the
	     * Internet. Censor special characters that could mess up message
	     * headers.
	     */
	case SMTPD_STATE_XFORWARD_ADDR:
	    if (STREQ(attr_value, XFORWARD_UNAVAILABLE)) {
		attr_value = CLIENT_ADDR_UNKNOWN;
		bare_value = attr_value;
	    } else {
		neuter(attr_value, NEUTER_CHARACTERS, '?');
		if ((bare_value = valid_mailhost_addr(attr_value, DONT_GRIPE)) == 0) {
		    state->error_mask |= MAIL_ERROR_PROTOCOL;
		    smtpd_chat_reply(state, "501 5.5.4 Bad %s syntax: %s",
				     XFORWARD_ADDR, attr_value);
		    return (-1);
		}
	    }
	    UPDATE_STR(state->xforward.addr, bare_value);
	    UPDATE_STR(state->xforward.rfc_addr, attr_value);
	    break;

	    /*
	     * PORT=up-stream port number.
	     */
	case SMTPD_STATE_XFORWARD_PORT:
	    if (STREQ(attr_value, XFORWARD_UNAVAILABLE)) {
		attr_value = CLIENT_PORT_UNKNOWN;
	    } else {
		if (!alldig(attr_value)
		    || strlen(attr_value) > sizeof("65535") - 1) {
		    state->error_mask |= MAIL_ERROR_PROTOCOL;
		    smtpd_chat_reply(state, "501 5.5.4 Bad %s syntax: %s",
				     XFORWARD_PORT, attr_value);
		    return (-1);
		}
	    }
	    UPDATE_STR(state->xforward.port, attr_value);
	    break;

	    /*
	     * HELO=hostname that the up-stream MTA introduced itself with
	     * (not necessarily SMTP HELO). Censor special characters that
	     * could mess up message headers.
	     */
	case SMTPD_STATE_XFORWARD_HELO:
	    if (STREQ(attr_value, XFORWARD_UNAVAILABLE)) {
		attr_value = CLIENT_HELO_UNKNOWN;
	    } else {
		neuter(attr_value, NEUTER_CHARACTERS, '?');
	    }
	    UPDATE_STR(state->xforward.helo_name, attr_value);
	    break;

	    /*
	     * PROTO=up-stream protocol, not necessarily SMTP or ESMTP.
	     * Censor special characters that could mess up message headers.
	     */
	case SMTPD_STATE_XFORWARD_PROTO:
	    if (STREQ(attr_value, XFORWARD_UNAVAILABLE)) {
		attr_value = CLIENT_PROTO_UNKNOWN;
	    } else {
		if (strlen(attr_value) > 64) {
		    state->error_mask |= MAIL_ERROR_PROTOCOL;
		    smtpd_chat_reply(state, "501 5.5.4 Bad %s syntax: %s",
				     XFORWARD_PROTO, attr_value);
		    return (-1);
		}
		neuter(attr_value, NEUTER_CHARACTERS, '?');
	    }
	    UPDATE_STR(state->xforward.protocol, attr_value);
	    break;

	    /*
	     * IDENT=local message identifier on the up-stream MTA. Censor
	     * special characters that could mess up logging or macro
	     * expansions.
	     */
	case SMTPD_STATE_XFORWARD_IDENT:
	    if (STREQ(attr_value, XFORWARD_UNAVAILABLE)) {
		attr_value = CLIENT_IDENT_UNKNOWN;
	    } else {
		neuter(attr_value, NEUTER_CHARACTERS, '?');
	    }
	    UPDATE_STR(state->xforward.ident, attr_value);
	    break;

	    /*
	     * DOMAIN=local or remote.
	     */
	case SMTPD_STATE_XFORWARD_DOMAIN:
	    if (STREQ(attr_value, XFORWARD_UNAVAILABLE))
		attr_value = XFORWARD_DOM_LOCAL;
	    if ((context_code = name_code(xforward_to_context,
					  NAME_CODE_FLAG_NONE,
					  attr_value)) < 0) {
		state->error_mask |= MAIL_ERROR_PROTOCOL;
		smtpd_chat_reply(state, "501 5.5.4 Bad %s syntax: %s",
				 XFORWARD_DOMAIN, attr_value);
		return (-1);
	    }
	    UPDATE_STR(state->xforward.domain, context_name[context_code]);
	    break;

	    /*
	     * Unknown attribute name. Complain.
	     */
	default:
	    state->error_mask |= MAIL_ERROR_PROTOCOL;
	    smtpd_chat_reply(state, "501 5.5.4 Bad %s attribute name: %s",
			     XFORWARD_CMD, attr_name);
	    return (-1);
	}
	updated |= flag;
    }
    state->xforward.flags |= updated;

    /*
     * Update the combined name and address when either has changed. Use only
     * the name when no address is available.
     */
    if (updated & (SMTPD_STATE_XFORWARD_NAME | SMTPD_STATE_XFORWARD_ADDR)) {
	if (state->xforward.namaddr)
	    myfree(state->xforward.namaddr);
	state->xforward.namaddr =
	    IS_AVAIL_CLIENT_ADDR(state->xforward.addr) ?
	    SMTPD_BUILD_NAMADDRPORT(state->xforward.name,
				    state->xforward.addr,
				    state->xforward.port) :
	    mystrdup(state->xforward.name);
    }
    smtpd_chat_reply(state, "250 2.0.0 Ok");
    return (0);
}

/* chat_reset - notify postmaster and reset conversation log */

static void chat_reset(SMTPD_STATE *state, int threshold)
{

    /*
     * Notify the postmaster if there were errors. This usually indicates a
     * client configuration problem, or that someone is trying nasty things.
     * Either is significant enough to bother the postmaster. XXX Can't
     * report problems when running in stand-alone mode: postmaster notices
     * require availability of the cleanup service.
     */
    if (state->history != 0 && state->history->argc > threshold) {
	if (SMTPD_STAND_ALONE(state) == 0
	    && (state->error_mask & state->notify_mask))
	    smtpd_chat_notify(state);
	state->error_mask = 0;
	smtpd_chat_reset(state);
    }
}

#ifdef USE_TLS

/* smtpd_start_tls - turn on TLS or force disconnect */

static void smtpd_start_tls(SMTPD_STATE *state)
{
    int     rate;
    int     cert_present;
    int     requirecert;

#ifdef USE_TLSPROXY

    /*
     * This is non-production code, for tlsproxy(8) load testing only. It
     * implements enough to enable some Postfix features that depend on TLS
     * encryption.
     * 
     * To insert tlsproxy(8) between this process and the SMTP client, we swap
     * the file descriptors between the state->tlsproxy and state->client
     * VSTREAMS, so that we don't lose all the user-configurable
     * state->client attributes (such as longjump buffers or timeouts).
     * 
     * As we implement tlsproy support in the Postfix SMTP client we should
     * develop a usable abstraction that encapsulates this stream plumbing in
     * a library module.
     */
    vstream_control(state->tlsproxy, VSTREAM_CTL_DOUBLE, VSTREAM_CTL_END);
    vstream_control(state->client, VSTREAM_CTL_SWAP_FD, state->tlsproxy,
		    VSTREAM_CTL_END);
    (void) vstream_fclose(state->tlsproxy);	/* direct-to-client stream! */
    state->tlsproxy = 0;

    /*
     * After plumbing the plaintext stream, receive the TLS context object.
     * For this we must use the same VSTREAM buffer that we also use to
     * receive subsequent SMTP commands. The attribute protocol is robust
     * enough that an adversary cannot inject their own bogus TLS context
     * attributes into the stream.
     */
    state->tls_context = tls_proxy_context_receive(state->client);

    /*
     * XXX Maybe it is better to send this information to tlsproxy(8) when
     * requesting service, effectively making a remote tls_server_start()
     * call.
     */
    requirecert = (var_smtpd_tls_req_ccert && var_smtpd_enforce_tls);

#else						/* USE_TLSPROXY */
    TLS_SERVER_START_PROPS props;
    static char *cipher_grade;
    static VSTRING *cipher_exclusions;

    /*
     * Wrapper mode uses a dedicated port and always requires TLS.
     * 
     * XXX In non-wrapper mode, it is possible to require client certificate
     * verification without requiring TLS. Since certificates can be verified
     * only while TLS is turned on, this means that Postfix will happily
     * perform SMTP transactions when the client does not use the STARTTLS
     * command. For this reason, Postfix does not require client certificate
     * verification unless TLS is required.
     * 
     * The cipher grade and exclusions don't change between sessions. Compute
     * just once and cache.
     */
#define ADD_EXCLUDE(vstr, str) \
    do { \
	if (*(str)) \
	    vstring_sprintf_append((vstr), "%s%s", \
				   VSTRING_LEN(vstr) ? " " : "", (str)); \
    } while (0)

    if (cipher_grade == 0) {
	cipher_grade = var_smtpd_enforce_tls ?
	    var_smtpd_tls_mand_ciph : var_smtpd_tls_ciph;
	cipher_exclusions = vstring_alloc(10);
	ADD_EXCLUDE(cipher_exclusions, var_smtpd_tls_excl_ciph);
	if (var_smtpd_enforce_tls)
	    ADD_EXCLUDE(cipher_exclusions, var_smtpd_tls_mand_excl);
	if (ask_client_cert)
	    ADD_EXCLUDE(cipher_exclusions, "aNULL");
    }

    /*
     * Perform the TLS handshake now. Check the client certificate
     * requirements later, if necessary.
     */
    requirecert = (var_smtpd_tls_req_ccert && var_smtpd_enforce_tls);

    state->tls_context =
	TLS_SERVER_START(&props,
			 ctx = smtpd_tls_ctx,
			 stream = state->client,
			 fd = -1,
			 timeout = var_smtpd_starttls_tmout,
			 requirecert = requirecert,
			 serverid = state->service,
			 namaddr = state->namaddr,
			 cipher_grade = cipher_grade,
			 cipher_exclusions = STR(cipher_exclusions),
			 fpt_dgst = var_smtpd_tls_fpt_dgst);

#endif						/* USE_TLSPROXY */

    /*
     * For new (i.e. not re-used) TLS sessions, increment the client's new
     * TLS session rate counter. We enforce the limit here only for human
     * factors reasons (reduce the WTF factor), even though it is too late to
     * save the CPU that was already burnt on PKI ops. The real safety
     * mechanism applies with future STARTTLS commands (or wrappermode
     * connections), prior to the SSL handshake.
     * 
     * XXX The client event count/rate control must be consistent in its use of
     * client address information in connect and disconnect events. For now
     * we exclude xclient authorized hosts from event count/rate control.
     */
    if (var_smtpd_cntls_limit > 0
     && (state->tls_context == 0 || state->tls_context->session_reused == 0)
	&& SMTPD_STAND_ALONE(state) == 0
	&& !xclient_allowed
	&& anvil_clnt
	&& !namadr_list_match(hogger_list, state->name, state->addr)
	&& anvil_clnt_newtls(anvil_clnt, state->service, state->addr,
			     &rate) == ANVIL_STAT_OK
	&& rate > var_smtpd_cntls_limit) {
	state->error_mask |= MAIL_ERROR_POLICY;
	msg_warn("New TLS session rate limit exceeded: %d from %s for service %s",
		 rate, state->namaddr, state->service);
	if (state->tls_context)
	    smtpd_chat_reply(state,
		    "421 4.7.0 %s Error: too many new TLS sessions from %s",
			     var_myhostname, state->namaddr);
	/* XXX Use regular return to signal end of session. */
	vstream_longjmp(state->client, SMTP_ERR_QUIET);
    }

    /*
     * When the TLS handshake fails, the conversation is in an unknown state.
     * There is nothing we can do except to disconnect from the client.
     */
    if (state->tls_context == 0)
	vstream_longjmp(state->client, SMTP_ERR_EOF);

    /*
     * If we are requiring verified client certs, enforce the constraint
     * here. We have a usable TLS session with the client, so no need to
     * disable I/O, ...  we can even be polite and send "421 ...".
     */
    if (requirecert && TLS_CERT_IS_TRUSTED(state->tls_context) == 0) {

	/*
	 * Fetch and reject the next command (should be EHLO), then
	 * disconnect (side-effect of returning "421 ...".
	 */
	cert_present = TLS_CERT_IS_PRESENT(state->tls_context);
	msg_info("NOQUEUE: abort: TLS from %s: %s",
		 state->namaddr, cert_present ?
		 "Client certificate not trusted" :
		 "No client certificate presented");
	smtpd_chat_query(state);
	smtpd_chat_reply(state, "421 4.7.1 %s Error: %s",
			 var_myhostname, cert_present ?
			 "Client certificate not trusted" :
			 "No client certificate presented");
	state->error_mask |= MAIL_ERROR_POLICY;
	return;
    }

    /*
     * When TLS is turned on, we may offer AUTH methods that would not be
     * offered within a plain-text session.
     * 
     * XXX Always refresh SASL the mechanism list after STARTTLS. Dovecot
     * responses may depend on whether the SMTP connection is encrypted.
     */
#ifdef USE_SASL_AUTH
    if (var_smtpd_sasl_enable) {
	/* Non-wrappermode, presumably. */
	if (smtpd_sasl_is_active(state)) {
	    smtpd_sasl_auth_reset(state);
	    smtpd_sasl_deactivate(state);
	}
	/* Wrappermode and non-wrappermode. */
	if (smtpd_sasl_is_active(state) == 0)
	    smtpd_sasl_activate(state, VAR_SMTPD_SASL_TLS_OPTS,
				var_smtpd_sasl_tls_opts);
    }
#endif
}

/* starttls_cmd - respond to STARTTLS */

static int starttls_cmd(SMTPD_STATE *state, int argc, SMTPD_TOKEN *unused_argv)
{
    const char *err;
    int     rate;

    if (argc != 1) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "501 5.5.4 Syntax: STARTTLS");
	return (-1);
    }
    if (smtpd_milters != 0 && (err = milter_other_event(smtpd_milters)) != 0) {
	if (err[0] == '5') {
	    state->error_mask |= MAIL_ERROR_POLICY;
	    smtpd_chat_reply(state, "%s", err);
	    return (-1);
	}
	/* Sendmail compatibility: map 4xx into 454. */
	else if (err[0] == '4') {
	    state->error_mask |= MAIL_ERROR_POLICY;
	    smtpd_chat_reply(state, "454 4.3.0 Try again later");
	    return (-1);
	}
    }
    if (state->tls_context != 0) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "554 5.5.1 Error: TLS already active");
	return (-1);
    }
    if (var_smtpd_use_tls == 0
	|| (state->ehlo_discard_mask & EHLO_MASK_STARTTLS)) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "502 5.5.1 Error: command not implemented");
	return (-1);
    }
#ifdef USE_TLSPROXY

    /*
     * Note: state->tlsproxy is left open when smtp_flush() calls longjmp(),
     * so we garbage-collect the VSTREAM in smtpd_state_reset().
     */
#define PROXY_OPEN_FLAGS \
	(TLS_PROXY_FLAG_ROLE_SERVER | TLS_PROXY_FLAG_SEND_CONTEXT)

    state->tlsproxy = tls_proxy_open(var_tlsproxy_service, PROXY_OPEN_FLAGS,
				     state->client, state->addr,
				     state->port, var_smtpd_tmout);
    if (state->tlsproxy == 0) {
	state->error_mask |= MAIL_ERROR_SOFTWARE;
	/* RFC 4954 Section 6. */
	smtpd_chat_reply(state, "454 4.7.0 TLS not available due to local problem");
	return (-1);
    }
#else						/* USE_TLSPROXY */
    if (smtpd_tls_ctx == 0) {
	state->error_mask |= MAIL_ERROR_SOFTWARE;
	/* RFC 4954 Section 6. */
	smtpd_chat_reply(state, "454 4.7.0 TLS not available due to local problem");
	return (-1);
    }
#endif						/* USE_TLSPROXY */

    /*
     * Enforce TLS handshake rate limit when this client negotiated too many
     * new TLS sessions in the recent past.
     * 
     * XXX The client event count/rate control must be consistent in its use of
     * client address information in connect and disconnect events. For now
     * we exclude xclient authorized hosts from event count/rate control.
     */
    if (var_smtpd_cntls_limit > 0
	&& SMTPD_STAND_ALONE(state) == 0
	&& !xclient_allowed
	&& anvil_clnt
	&& !namadr_list_match(hogger_list, state->name, state->addr)
	&& anvil_clnt_newtls_stat(anvil_clnt, state->service, state->addr,
				  &rate) == ANVIL_STAT_OK
	&& rate > var_smtpd_cntls_limit) {
	state->error_mask |= MAIL_ERROR_POLICY;
	msg_warn("Refusing STARTTLS request from %s for service %s",
		 state->namaddr, state->service);
	smtpd_chat_reply(state,
		       "454 4.7.0 Error: too many new TLS sessions from %s",
			 state->namaddr);
#ifdef USE_TLSPROXY
	(void) vstream_fclose(state->tlsproxy);
	state->tlsproxy = 0;
#endif
	return (-1);
    }
    smtpd_chat_reply(state, "220 2.0.0 Ready to start TLS");
    /* Flush before we switch read/write routines or file descriptors. */
    smtp_flush(state->client);
    /* At this point there must not be any pending plaintext. */
    vstream_fpurge(state->client, VSTREAM_PURGE_BOTH);

    /*
     * Reset all inputs to the initial state.
     * 
     * XXX RFC 2487 does not forbid the use of STARTTLS while mail transfer is
     * in progress, so we have to allow it even when it makes no sense.
     */
    helo_reset(state);
    mail_reset(state);
    rcpt_reset(state);

    /*
     * Turn on TLS, using code that is shared with TLS wrapper mode. This
     * code does not return when the handshake fails.
     */
    smtpd_start_tls(state);
    return (0);
}

/* tls_reset - undo STARTTLS */

static void tls_reset(SMTPD_STATE *state)
{
    int     failure = 0;

    /*
     * Don't waste time when we lost contact.
     */
    if (state->tls_context) {
	if (vstream_feof(state->client) || vstream_ferror(state->client))
	    failure = 1;
	vstream_fflush(state->client);		/* NOT: smtp_flush() */
#ifdef USE_TLSPROXY
	tls_proxy_context_free(state->tls_context);
#else
	tls_server_stop(smtpd_tls_ctx, state->client, var_smtpd_starttls_tmout,
			failure, state->tls_context);
#endif
	state->tls_context = 0;
    }
}

#endif

 /*
  * The table of all SMTP commands that we know. Set the junk limit flag on
  * any command that can be repeated an arbitrary number of times without
  * triggering a tarpit delay of some sort.
  */
typedef struct SMTPD_CMD {
    char   *name;
    int     (*action) (SMTPD_STATE *, int, SMTPD_TOKEN *);
    int     flags;
} SMTPD_CMD;

#define SMTPD_CMD_FLAG_LIMIT	(1<<0)	/* limit usage */
#define SMTPD_CMD_FLAG_PRE_TLS	(1<<1)	/* allow before STARTTLS */
#define SMTPD_CMD_FLAG_LAST	(1<<2)	/* last in PIPELINING command group */

static SMTPD_CMD smtpd_cmd_table[] = {
    SMTPD_CMD_HELO, helo_cmd, SMTPD_CMD_FLAG_LIMIT | SMTPD_CMD_FLAG_PRE_TLS | SMTPD_CMD_FLAG_LAST,
    SMTPD_CMD_EHLO, ehlo_cmd, SMTPD_CMD_FLAG_LIMIT | SMTPD_CMD_FLAG_PRE_TLS | SMTPD_CMD_FLAG_LAST,
#ifdef USE_TLS
    SMTPD_CMD_STARTTLS, starttls_cmd, SMTPD_CMD_FLAG_PRE_TLS,
#endif
#ifdef USE_SASL_AUTH
    SMTPD_CMD_AUTH, smtpd_sasl_auth_cmd, 0,
#endif
    SMTPD_CMD_MAIL, mail_cmd, 0,
    SMTPD_CMD_RCPT, rcpt_cmd, 0,
    SMTPD_CMD_DATA, data_cmd, SMTPD_CMD_FLAG_LAST,
    SMTPD_CMD_RSET, rset_cmd, SMTPD_CMD_FLAG_LIMIT,
    SMTPD_CMD_NOOP, noop_cmd, SMTPD_CMD_FLAG_LIMIT | SMTPD_CMD_FLAG_PRE_TLS | SMTPD_CMD_FLAG_LAST,
    SMTPD_CMD_VRFY, vrfy_cmd, SMTPD_CMD_FLAG_LIMIT | SMTPD_CMD_FLAG_LAST,
    SMTPD_CMD_ETRN, etrn_cmd, SMTPD_CMD_FLAG_LIMIT,
    SMTPD_CMD_QUIT, quit_cmd, SMTPD_CMD_FLAG_PRE_TLS,
    SMTPD_CMD_XCLIENT, xclient_cmd, 0,
    SMTPD_CMD_XFORWARD, xforward_cmd, 0,
    0,
};

static STRING_LIST *smtpd_noop_cmds;
static STRING_LIST *smtpd_forbid_cmds;

/* smtpd_proto - talk the SMTP protocol */

static void smtpd_proto(SMTPD_STATE *state)
{
    int     argc;
    SMTPD_TOKEN *argv;
    SMTPD_CMD *cmdp;
    int     tls_rate;
    const char *ehlo_words;
    const char *err;
    int     status;
    const char *cp;

    /*
     * Print a greeting banner and run the state machine. Read SMTP commands
     * one line at a time. According to the standard, a sender or recipient
     * address could contain an escaped newline. I think this is perverse,
     * and anyone depending on this is really asking for trouble.
     * 
     * In case of mail protocol trouble, the program jumps back to this place,
     * so that it can perform the necessary cleanup before talking to the
     * next client. The setjmp/longjmp primitives are like a sharp tool: use
     * with care. I would certainly recommend against the use of
     * setjmp/longjmp in programs that change privilege levels.
     * 
     * In case of file system trouble the program terminates after logging the
     * error and after informing the client. In all other cases (out of
     * memory, panic) the error is logged, and the msg_cleanup() exit handler
     * cleans up, but no attempt is made to inform the client of the nature
     * of the problem.
     */
    smtp_stream_setup(state->client, var_smtpd_tmout, var_smtpd_rec_deadline);

    while ((status = vstream_setjmp(state->client)) == SMTP_ERR_NONE)
	 /* void */ ;
    switch (status) {

    default:
	msg_panic("smtpd_proto: unknown error reading from %s",
		  state->namaddr);
	break;

    case SMTP_ERR_TIME:
	state->reason = REASON_TIMEOUT;
	if (vstream_setjmp(state->client) == 0)
	    smtpd_chat_reply(state, "421 4.4.2 %s Error: timeout exceeded",
			     var_myhostname);
	break;

    case SMTP_ERR_EOF:
	state->reason = REASON_LOST_CONNECTION;
	break;

    case SMTP_ERR_QUIET:
	break;

    case SMTP_ERR_DATA:
	msg_info("%s: reject: %s from %s: "
		 "421 4.3.0 %s Server local data error",
		 (state->queue_id ? state->queue_id : "NOQUEUE"),
		 state->where, state->namaddr, var_myhostname);
	state->error_mask |= MAIL_ERROR_DATA;
	if (vstream_setjmp(state->client) == 0)
	    smtpd_chat_reply(state, "421 4.3.0 %s Server local data error",
			     var_myhostname);
	break;

    case 0:

	/*
	 * In TLS wrapper mode, turn on TLS using code that is shared with
	 * the STARTTLS command. This code does not return when the handshake
	 * fails.
	 * 
	 * Enforce TLS handshake rate limit when this client negotiated too many
	 * new TLS sessions in the recent past.
	 * 
	 * XXX This means we don't complete a TLS handshake just to tell the
	 * client that we don't provide service. TLS wrapper mode is
	 * obsolete, so we don't have to provide perfect support.
	 */
#ifdef USE_TLS
	if (SMTPD_STAND_ALONE(state) == 0 && var_smtpd_tls_wrappermode) {
#ifdef USE_TLSPROXY
	    /* We garbage-collect the VSTREAM in smtpd_state_reset() */
	    state->tlsproxy = tls_proxy_open(var_tlsproxy_service,
					     PROXY_OPEN_FLAGS,
					     state->client, state->addr,
					     state->port, var_smtpd_tmout);
	    if (state->tlsproxy == 0) {
		msg_warn("Wrapper-mode request dropped from %s for service %s."
		       " TLS context initialization failed. For details see"
			 " earlier warnings in your logs.",
			 state->namaddr, state->service);
		break;
	    }
#else						/* USE_TLSPROXY */
	    if (smtpd_tls_ctx == 0) {
		msg_warn("Wrapper-mode request dropped from %s for service %s."
		       " TLS context initialization failed. For details see"
			 " earlier warnings in your logs.",
			 state->namaddr, state->service);
		break;
	    }
#endif						/* USE_TLSPROXY */
	    if (var_smtpd_cntls_limit > 0
		&& !xclient_allowed
		&& anvil_clnt
		&& !namadr_list_match(hogger_list, state->name, state->addr)
		&& anvil_clnt_newtls_stat(anvil_clnt, state->service,
				    state->addr, &tls_rate) == ANVIL_STAT_OK
		&& tls_rate > var_smtpd_cntls_limit) {
		state->error_mask |= MAIL_ERROR_POLICY;
		msg_warn("Refusing TLS service request from %s for service %s",
			 state->namaddr, state->service);
		break;
	    }
	    smtpd_start_tls(state);
	}
#endif

	/*
	 * XXX The client connection count/rate control must be consistent in
	 * its use of client address information in connect and disconnect
	 * events. For now we exclude xclient authorized hosts from
	 * connection count/rate control.
	 * 
	 * XXX Must send connect/disconnect events to the anvil server even when
	 * this service is not connection count or rate limited, otherwise it
	 * will discard client message or recipient rate information too
	 * early or too late.
	 */
	if (SMTPD_STAND_ALONE(state) == 0
	    && !xclient_allowed
	    && anvil_clnt
	    && !namadr_list_match(hogger_list, state->name, state->addr)
	    && anvil_clnt_connect(anvil_clnt, state->service, state->addr,
				  &state->conn_count, &state->conn_rate)
	    == ANVIL_STAT_OK) {
	    if (var_smtpd_cconn_limit > 0
		&& state->conn_count > var_smtpd_cconn_limit) {
		state->error_mask |= MAIL_ERROR_POLICY;
		msg_warn("Connection concurrency limit exceeded: %d from %s for service %s",
			 state->conn_count, state->namaddr, state->service);
		smtpd_chat_reply(state, "421 4.7.0 %s Error: too many connections from %s",
				 var_myhostname, state->addr);
		break;
	    }
	    if (var_smtpd_crate_limit > 0
		&& state->conn_rate > var_smtpd_crate_limit) {
		msg_warn("Connection rate limit exceeded: %d from %s for service %s",
			 state->conn_rate, state->namaddr, state->service);
		smtpd_chat_reply(state, "421 4.7.0 %s Error: too many connections from %s",
				 var_myhostname, state->addr);
		break;
	    }
	}

	/*
	 * Determine what server ESMTP features to suppress, typically to
	 * avoid inter-operability problems. Moved up so we don't send 421
	 * immediately after sending the initial server response.
	 */
	if (ehlo_discard_maps == 0
	|| (ehlo_words = maps_find(ehlo_discard_maps, state->addr, 0)) == 0)
	    ehlo_words = var_smtpd_ehlo_dis_words;
	state->ehlo_discard_mask = ehlo_mask(ehlo_words);

	/* XXX We use the real client for connect access control. */
	if (SMTPD_STAND_ALONE(state) == 0
	    && var_smtpd_delay_reject == 0
	    && (err = smtpd_check_client(state)) != 0) {
	    state->error_mask |= MAIL_ERROR_POLICY;
	    state->access_denied = mystrdup(err);
	    smtpd_chat_reply(state, "%s", state->access_denied);
	    state->error_count++;
	}

	/*
	 * RFC 2034: the text part of all 2xx, 4xx, and 5xx SMTP responses
	 * other than the initial greeting and any response to HELO or EHLO
	 * are prefaced with a status code as defined in RFC 3463.
	 */

	/*
	 * XXX If a Milter rejects CONNECT, reply with 220 except in case of
	 * hard reject or 421 (disconnect). The reply persists so it will
	 * apply to MAIL FROM and to other commands such as AUTH, STARTTLS,
	 * and VRFY. Note: after a Milter CONNECT reject, we must not reject
	 * HELO or EHLO, but we do change the feature list that is announced
	 * in the EHLO response.
	 */
	else {
	    err = 0;
	    if (smtpd_milters != 0 && SMTPD_STAND_ALONE(state) == 0) {
		milter_macro_callback(smtpd_milters, smtpd_milter_eval,
				      (void *) state);
		if ((err = milter_conn_event(smtpd_milters, state->name,
					     state->addr,
				  strcmp(state->port, CLIENT_PORT_UNKNOWN) ?
					     state->port : "0",
					     state->addr_family)) != 0)
		    err = check_milter_reply(state, err);
	    }
	    if (err && err[0] == '5') {
		state->error_mask |= MAIL_ERROR_POLICY;
		smtpd_chat_reply(state, "554 %s ESMTP not accepting connections",
				 var_myhostname);
		state->error_count++;
	    } else if (err && strncmp(err, "421", 3) == 0) {
		state->error_mask |= MAIL_ERROR_POLICY;
		smtpd_chat_reply(state, "421 %s Service unavailable - try again later",
				 var_myhostname);
		/* Not: state->error_count++; */
	    } else {
		smtpd_chat_reply(state, "220 %s", var_smtpd_banner);
	    }
	}

	/*
	 * SASL initialization for plaintext mode.
	 * 
	 * XXX Backwards compatibility: allow AUTH commands when the AUTH
	 * announcement is suppressed via smtpd_sasl_exceptions_networks.
	 * 
	 * XXX Safety: don't enable SASL with "smtpd_tls_auth_only = yes" and
	 * non-TLS build.
	 */
#ifdef USE_SASL_AUTH
	if (var_smtpd_sasl_enable && smtpd_sasl_is_active(state) == 0
#ifdef USE_TLS
	    && state->tls_context == 0 && !var_smtpd_tls_auth_only
#else
	    && var_smtpd_tls_auth_only == 0
#endif
	    )
	    smtpd_sasl_activate(state, VAR_SMTPD_SASL_OPTS,
				var_smtpd_sasl_opts);
#endif

	for (;;) {
	    if (state->flags & SMTPD_FLAG_HANGUP)
		break;
	    if (state->error_count >= var_smtpd_hard_erlim) {
		state->reason = REASON_ERROR_LIMIT;
		state->error_mask |= MAIL_ERROR_PROTOCOL;
		smtpd_chat_reply(state, "421 4.7.0 %s Error: too many errors",
				 var_myhostname);
		break;
	    }
	    watchdog_pat();
	    smtpd_chat_query(state);
	    /* Move into smtpd_chat_query() and update session transcript. */
	    if (smtpd_cmd_filter != 0) {
		for (cp = STR(state->buffer); *cp && IS_SPACE_TAB(*cp); cp++)
		     /* void */ ;
		if ((cp = dict_get(smtpd_cmd_filter, cp)) != 0) {
		    msg_info("%s: replacing command \"%.100s\" with \"%.100s\"",
			     state->namaddr, STR(state->buffer), cp);
		    vstring_strcpy(state->buffer, cp);
		} else if (smtpd_cmd_filter->error != 0) {
		    msg_warn("%s:%s lookup error for \"%.100s\"",
			     smtpd_cmd_filter->type, smtpd_cmd_filter->name,
			     printable(STR(state->buffer), '?'));
		    vstream_longjmp(state->client, SMTP_ERR_DATA);
		}
	    }
	    if ((argc = smtpd_token(vstring_str(state->buffer), &argv)) == 0) {
		state->error_mask |= MAIL_ERROR_PROTOCOL;
		smtpd_chat_reply(state, "500 5.5.2 Error: bad syntax");
		state->error_count++;
		continue;
	    }
	    /* Ignore smtpd_noop_cmds lookup errors. Non-critical feature. */
	    if (*var_smtpd_noop_cmds
		&& string_list_match(smtpd_noop_cmds, argv[0].strval)) {
		smtpd_chat_reply(state, "250 2.0.0 Ok");
		if (state->junk_cmds++ > var_smtpd_junk_cmd_limit)
		    state->error_count++;
		continue;
	    }
	    for (cmdp = smtpd_cmd_table; cmdp->name != 0; cmdp++)
		if (strcasecmp(argv[0].strval, cmdp->name) == 0)
		    break;
	    /* Ignore smtpd_forbid_cmds lookup errors. Non-critical feature. */
	    if (cmdp->name == 0) {
		state->where = SMTPD_CMD_UNKNOWN;
		if (is_header(argv[0].strval)
		    || (*var_smtpd_forbid_cmds
		 && string_list_match(smtpd_forbid_cmds, argv[0].strval))) {
		    msg_warn("non-SMTP command from %s: %.100s",
			     state->namaddr, vstring_str(state->buffer));
		    smtpd_chat_reply(state, "221 2.7.0 Error: I can break rules, too. Goodbye.");
		    break;
		}
	    }
	    /* XXX We use the real client for connect access control. */
	    if (state->access_denied && cmdp->action != quit_cmd) {
		/* XXX Exception for Milter override. */
		if (strncmp(state->access_denied + 1, "21", 2) == 0) {
		    smtpd_chat_reply(state, "%s", state->access_denied);
		    continue;
		}
		smtpd_chat_reply(state, "503 5.7.0 Error: access denied for %s",
				 state->namaddr);	/* RFC 2821 Sec 3.1 */
		state->error_count++;
		continue;
	    }
	    /* state->access_denied == 0 || cmdp->action == quit_cmd */
	    if (cmdp->name == 0) {
		if (smtpd_milters != 0
		    && SMTPD_STAND_ALONE(state) == 0
		    && (err = milter_unknown_event(smtpd_milters,
						   argv[0].strval)) != 0
		    && (err = check_milter_reply(state, err)) != 0) {
		    smtpd_chat_reply(state, "%s", err);
		} else
		    smtpd_chat_reply(state, "502 5.5.2 Error: command not recognized");
		state->error_mask |= MAIL_ERROR_PROTOCOL;
		state->error_count++;
		continue;
	    }
#ifdef USE_TLS
	    if (var_smtpd_enforce_tls &&
		!state->tls_context &&
		(cmdp->flags & SMTPD_CMD_FLAG_PRE_TLS) == 0) {
		smtpd_chat_reply(state,
			   "530 5.7.0 Must issue a STARTTLS command first");
		state->error_count++;
		continue;
	    }
#endif
	    state->where = cmdp->name;
	    if (SMTPD_STAND_ALONE(state) == 0
		&& (strcasecmp(state->protocol, MAIL_PROTO_ESMTP) != 0
		    || (cmdp->flags & SMTPD_CMD_FLAG_LAST))
		&& (state->flags & SMTPD_FLAG_ILL_PIPELINING) == 0
		&& (vstream_peek(state->client) > 0
		    || peekfd(vstream_fileno(state->client)) > 0)) {
		if (state->expand_buf == 0)
		    state->expand_buf = vstring_alloc(100);
		escape(state->expand_buf, vstream_peek_data(state->client),
		       vstream_peek(state->client) < 100 ?
		       vstream_peek(state->client) : 100);
		msg_info("improper command pipelining after %s from %s: %s",
			 cmdp->name, state->namaddr, STR(state->expand_buf));
		state->flags |= SMTPD_FLAG_ILL_PIPELINING;
	    }
	    if (cmdp->action(state, argc, argv) != 0)
		state->error_count++;
	    if ((cmdp->flags & SMTPD_CMD_FLAG_LIMIT)
		&& state->junk_cmds++ > var_smtpd_junk_cmd_limit)
		state->error_count++;
	    if (cmdp->action == quit_cmd)
		break;
	}
	break;
    }

    /*
     * XXX The client connection count/rate control must be consistent in its
     * use of client address information in connect and disconnect events.
     * For now we exclude xclient authorized hosts from connection count/rate
     * control.
     * 
     * XXX Must send connect/disconnect events to the anvil server even when
     * this service is not connection count or rate limited, otherwise it
     * will discard client message or recipient rate information too early or
     * too late.
     */
    if (SMTPD_STAND_ALONE(state) == 0
	&& !xclient_allowed
	&& anvil_clnt
	&& !namadr_list_match(hogger_list, state->name, state->addr))
	anvil_clnt_disconnect(anvil_clnt, state->service, state->addr);

    /*
     * Log abnormal session termination, in case postmaster notification has
     * been turned off. In the log, indicate the last recognized state before
     * things went wrong. Don't complain about clients that go away without
     * sending QUIT. Log the byte count after DATA to help diagnose MTU
     * troubles.
     */
    if (state->reason && state->where) {
	if (strcmp(state->where, SMTPD_AFTER_DATA) == 0) {
	    msg_info("%s after %s (%lu bytes) from %s",	/* 2.5 compat */
		     state->reason, SMTPD_CMD_DATA,	/* 2.5 compat */
		     (long) (state->act_size + vstream_peek(state->client)),
		     state->namaddr);
	} else if (strcmp(state->where, SMTPD_AFTER_DOT)
		   || strcmp(state->reason, REASON_LOST_CONNECTION)) {
	    msg_info("%s after %s from %s",
		     state->reason, state->where, state->namaddr);
	}
    }

    /*
     * Cleanup whatever information the client gave us during the SMTP
     * dialog.
     * 
     * XXX Duplicated in xclient_cmd().
     */
#ifdef USE_TLS
    tls_reset(state);
#endif
    helo_reset(state);
#ifdef USE_SASL_AUTH
    smtpd_sasl_auth_reset(state);
    if (smtpd_sasl_is_active(state)) {
	smtpd_sasl_deactivate(state);
    }
#endif
    chat_reset(state, 0);
    mail_reset(state);
    rcpt_reset(state);
    if (smtpd_milters)
	milter_disc_event(smtpd_milters);
}

/* smtpd_service - service one client */

static void smtpd_service(VSTREAM *stream, char *service, char **argv)
{
    SMTPD_STATE state;

    /*
     * Sanity check. This service takes no command-line arguments.
     */
    if (argv[0])
	msg_fatal("unexpected command-line argument: %s", argv[0]);

    /*
     * For sanity, require that at least one of INET or INET6 is enabled.
     * Otherwise, we can't look up interface information, and we can't
     * convert names or addresses.
     */
    if (SMTPD_STAND_ALONE_STREAM(stream) == 0
	&& inet_proto_info()->ai_family_list[0] == 0)
	msg_fatal("all network protocols are disabled (%s = %s)",
		  VAR_INET_PROTOCOLS, var_inet_protocols);

    /*
     * This routine runs when a client has connected to our network port, or
     * when the smtp server is run in stand-alone mode (input from pipe).
     * 
     * Look up and sanitize the peer name, then initialize some connection-
     * specific state. When the name service is hosed, hostname lookup will
     * take a while. This is why I always run a local name server on critical
     * machines.
     */
    smtpd_state_init(&state, stream, service);
    msg_info("connect from %s", state.namaddr);

    /*
     * Disable TLS when running in stand-alone mode via "sendmail -bs".
     */
    if (SMTPD_STAND_ALONE((&state))) {
	var_smtpd_use_tls = 0;
	var_smtpd_enforce_tls = 0;
	var_smtpd_tls_auth_only = 0;
    }

    /*
     * XCLIENT must not override its own access control.
     */
    xclient_allowed = SMTPD_STAND_ALONE((&state)) == 0 &&
	namadr_list_match(xclient_hosts, state.name, state.addr);

    /*
     * Overriding XFORWARD access control makes no sense, either.
     */
    xforward_allowed = SMTPD_STAND_ALONE((&state)) == 0 &&
	namadr_list_match(xforward_hosts, state.name, state.addr);

    /*
     * See if we need to turn on verbose logging for this client.
     */
    debug_peer_check(state.name, state.addr);

    /*
     * Provide the SMTP service.
     */
    smtpd_proto(&state);

    /*
     * After the client has gone away, clean up whatever we have set up at
     * connection time.
     */
    msg_info("disconnect from %s", state.namaddr);
    smtpd_state_reset(&state);
    debug_peer_restore();
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

/* pre_jail_init - pre-jail initialization */

static void pre_jail_init(char *unused_name, char **unused_argv)
{

    /*
     * Initialize blacklist/etc. patterns before entering the chroot jail, in
     * case they specify a filename pattern.
     */
    smtpd_noop_cmds = string_list_init(MATCH_FLAG_RETURN, var_smtpd_noop_cmds);
    smtpd_forbid_cmds = string_list_init(MATCH_FLAG_RETURN, var_smtpd_forbid_cmds);
    verp_clients = namadr_list_init(MATCH_FLAG_RETURN, var_verp_clients);
    xclient_hosts = namadr_list_init(MATCH_FLAG_RETURN, var_xclient_hosts);
    xforward_hosts = namadr_list_init(MATCH_FLAG_RETURN, var_xforward_hosts);
    hogger_list = namadr_list_init(MATCH_FLAG_RETURN, var_smtpd_hoggers);

    /*
     * Open maps before dropping privileges so we can read passwords etc.
     * 
     * XXX We should not do this in stand-alone (sendmail -bs) mode, but we
     * can't use SMTPD_STAND_ALONE(state) here. This means "sendmail -bs"
     * will try to connect to proxymap when invoked by root for mail
     * submission. To fix, we would have to pass stand-alone mode information
     * via different means. For now we have to tell people not to run mail
     * clients as root.
     */
    if (getuid() == 0 || getuid() == var_owner_uid)
	smtpd_check_init();
    smtpd_expand_init();
    debug_peer_init();

    if (var_smtpd_sasl_enable)
#ifdef USE_SASL_AUTH
	smtpd_sasl_initialize();

    if (*var_smtpd_sasl_exceptions_networks)
	sasl_exceptions_networks =
	    namadr_list_init(MATCH_FLAG_RETURN,
			     var_smtpd_sasl_exceptions_networks);
#else
	msg_warn("%s is true, but SASL support is not compiled in",
		 VAR_SMTPD_SASL_ENABLE);
#endif

    if (*var_smtpd_cmd_filter)
	smtpd_cmd_filter = dict_open(var_smtpd_cmd_filter, O_RDONLY,
				     DICT_FLAG_LOCK | DICT_FLAG_FOLD_FIX);

    /*
     * XXX Temporary fix to pretend that we consistently implement TLS
     * security levels. We implement only a subset for now. If we implement
     * more levels, wrappermode should override only weaker TLS security
     * levels.
     * 
     * Note: tls_level_lookup() logs no warning.
     */
    if (!var_smtpd_tls_wrappermode && *var_smtpd_tls_level) {
	switch (tls_level_lookup(var_smtpd_tls_level)) {
	default:
	    msg_fatal("Invalid TLS level \"%s\"", var_smtpd_tls_level);
	    /* NOTREACHED */
	    break;
	case TLS_LEV_SECURE:
	case TLS_LEV_VERIFY:
	case TLS_LEV_FPRINT:
	    msg_warn("%s: unsupported TLS level \"%s\", using \"encrypt\"",
		     VAR_SMTPD_TLS_LEVEL, var_smtpd_tls_level);
	    /* FALLTHROUGH */
	case TLS_LEV_ENCRYPT:
	    var_smtpd_enforce_tls = var_smtpd_use_tls = 1;
	    break;
	case TLS_LEV_MAY:
	    var_smtpd_enforce_tls = 0;
	    var_smtpd_use_tls = 1;
	    break;
	case TLS_LEV_NONE:
	    var_smtpd_enforce_tls = var_smtpd_use_tls = 0;
	    break;
	}
    }

    /*
     * With TLS wrapper mode, we run on a dedicated port and turn on TLS
     * before actually speaking the SMTP protocol. This implies TLS enforce
     * mode.
     * 
     * With non-wrapper mode, TLS enforce mode implies that we don't advertise
     * AUTH before the client issues STARTTLS.
     */
    var_smtpd_enforce_tls = var_smtpd_tls_wrappermode || var_smtpd_enforce_tls;
    var_smtpd_tls_auth_only = var_smtpd_tls_auth_only || var_smtpd_enforce_tls;
    var_smtpd_use_tls = var_smtpd_use_tls || var_smtpd_enforce_tls;

    /*
     * Keys can only be loaded when running with suitable permissions. When
     * called from "sendmail -bs" this is not the case, so we must not
     * announce STARTTLS support.
     */
    if (getuid() == 0 || getuid() == var_owner_uid) {
	if (var_smtpd_use_tls) {
#ifdef USE_TLS
#ifndef USE_TLSPROXY
	    TLS_SERVER_INIT_PROPS props;
	    const char *cert_file;
	    int     have_server_cert;
	    int     no_server_cert_ok;
	    int     require_server_cert;

	    /*
	     * Can't use anonymous ciphers if we want client certificates.
	     * Must use anonymous ciphers if we have no certificates.
	     * 
	     * XXX: Ugh! Too many booleans!
	     */
	    ask_client_cert = require_server_cert =
		(var_smtpd_tls_ask_ccert
		 || (var_smtpd_enforce_tls && var_smtpd_tls_req_ccert));
	    if (strcasecmp(var_smtpd_tls_cert_file, "none") == 0) {
		no_server_cert_ok = 1;
		cert_file = "";
	    } else {
		no_server_cert_ok = 0;
		cert_file = var_smtpd_tls_cert_file;
	    }
	    have_server_cert =
		(*cert_file || *var_smtpd_tls_dcert_file || *var_smtpd_tls_eccert_file);

	    /* Some TLS configuration errors are not show stoppers. */
	    if (!have_server_cert && require_server_cert)
		msg_warn("Need a server cert to request client certs");
	    if (!var_smtpd_enforce_tls && var_smtpd_tls_req_ccert)
		msg_warn("Can't require client certs unless TLS is required");
	    /* After a show-stopper error, reply with 454 to STARTTLS. */
	    if (have_server_cert || (no_server_cert_ok && !require_server_cert))

		/*
		 * Large parameter lists are error-prone, so we emulate a
		 * language feature that C does not have natively: named
		 * parameter lists.
		 */
		smtpd_tls_ctx =
		    TLS_SERVER_INIT(&props,
				    log_param = VAR_SMTPD_TLS_LOGLEVEL,
				    log_level = var_smtpd_tls_loglevel,
				    verifydepth = var_smtpd_tls_ccert_vd,
				    cache_type = TLS_MGR_SCACHE_SMTPD,
				    scache_timeout
				    = var_smtpd_tls_scache_timeout,
				    set_sessid = var_smtpd_tls_set_sessid,
				    cert_file = cert_file,
				    key_file = var_smtpd_tls_key_file,
				    dcert_file = var_smtpd_tls_dcert_file,
				    dkey_file = var_smtpd_tls_dkey_file,
				    eccert_file = var_smtpd_tls_eccert_file,
				    eckey_file = var_smtpd_tls_eckey_file,
				    CAfile = var_smtpd_tls_CAfile,
				    CApath = var_smtpd_tls_CApath,
				    dh1024_param_file
				    = var_smtpd_tls_dh1024_param_file,
				    dh512_param_file
				    = var_smtpd_tls_dh512_param_file,
				    eecdh_grade = var_smtpd_tls_eecdh,
				    protocols = var_smtpd_enforce_tls ?
				    var_smtpd_tls_mand_proto :
				    var_smtpd_tls_proto,
				    ask_ccert = ask_client_cert,
				    fpt_dgst = var_smtpd_tls_fpt_dgst);
	    else
		msg_warn("No server certs available. TLS won't be enabled");
#endif						/* USE_TLSPROXY */
#else
	    msg_warn("TLS has been selected, but TLS support is not compiled in");
#endif
	}
    }

    /*
     * flush client.
     */
    flush_init();

    /*
     * EHLO keyword filter.
     */
    if (*var_smtpd_ehlo_dis_maps)
	ehlo_discard_maps = maps_create(VAR_SMTPD_EHLO_DIS_MAPS,
					var_smtpd_ehlo_dis_maps,
					DICT_FLAG_LOCK);
}

/* post_jail_init - post-jail initialization */

static void post_jail_init(char *unused_name, char **unused_argv)
{

    /*
     * Initialize the receive transparency options: do we want unknown
     * recipient checks, address mapping, header_body_checks?.
     */
    smtpd_input_transp_mask =
    input_transp_mask(VAR_INPUT_TRANSP, var_input_transp);

    /*
     * Initialize before-queue filter options: do we want speed-matching
     * support so that the entire message is received before we contact a
     * before-queue content filter?
     */
    if (*var_smtpd_proxy_filt)
	smtpd_proxy_opts =
	    smtpd_proxy_parse_opts(VAR_SMTPD_PROXY_OPTS, var_smtpd_proxy_opts);

    /*
     * Sendmail mail filters.
     * 
     * XXX Should not do this when running in stand-alone mode. But that test
     * looks at VSTREAM_IN which is not available at this point.
     * 
     * XXX Disable non_smtpd_milters when not sending our own mail filter list.
     */
    if ((smtpd_input_transp_mask & INPUT_TRANSP_MILTER) == 0) {
	if (*var_smtpd_milters)
	    smtpd_milters = milter_create(var_smtpd_milters,
					  var_milt_conn_time,
					  var_milt_cmd_time,
					  var_milt_msg_time,
					  var_milt_protocol,
					  var_milt_def_action,
					  var_milt_conn_macros,
					  var_milt_helo_macros,
					  var_milt_mail_macros,
					  var_milt_rcpt_macros,
					  var_milt_data_macros,
					  var_milt_eoh_macros,
					  var_milt_eod_macros,
					  var_milt_unk_macros);
	else
	    smtpd_input_transp_mask |= INPUT_TRANSP_MILTER;
    }

    /*
     * Sanity checks. The queue_minfree value should be at least as large as
     * (process_limit * message_size_limit) but that is unpractical, so we
     * arbitrarily pick a small multiple of the per-message size limit. This
     * helps to avoid many unneeded (re)transmissions.
     */
    if (var_queue_minfree > 0
	&& var_message_limit > 0
	&& var_queue_minfree / 1.5 < var_message_limit)
	msg_warn("%s(%lu) should be at least 1.5*%s(%lu)",
		 VAR_QUEUE_MINFREE, (unsigned long) var_queue_minfree,
		 VAR_MESSAGE_LIMIT, (unsigned long) var_message_limit);

    /*
     * Connection rate management.
     */
    if (var_smtpd_crate_limit || var_smtpd_cconn_limit
	|| var_smtpd_cmail_limit || var_smtpd_crcpt_limit
	|| var_smtpd_cntls_limit)
	anvil_clnt = anvil_clnt_create();
}

MAIL_VERSION_STAMP_DECLARE;

/* main - the main program */

int     main(int argc, char **argv)
{
    static const CONFIG_NINT_TABLE nint_table[] = {
	VAR_SMTPD_SOFT_ERLIM, DEF_SMTPD_SOFT_ERLIM, &var_smtpd_soft_erlim, 1, 0,
	VAR_SMTPD_HARD_ERLIM, DEF_SMTPD_HARD_ERLIM, &var_smtpd_hard_erlim, 1, 0,
	VAR_SMTPD_JUNK_CMD, DEF_SMTPD_JUNK_CMD, &var_smtpd_junk_cmd_limit, 1, 0,
	VAR_VERIFY_POLL_COUNT, DEF_VERIFY_POLL_COUNT, &var_verify_poll_count, 1, 0,
	0,
    };
    static const CONFIG_INT_TABLE int_table[] = {
	VAR_SMTPD_RCPT_LIMIT, DEF_SMTPD_RCPT_LIMIT, &var_smtpd_rcpt_limit, 1, 0,
	VAR_QUEUE_MINFREE, DEF_QUEUE_MINFREE, &var_queue_minfree, 0, 0,
	VAR_UNK_CLIENT_CODE, DEF_UNK_CLIENT_CODE, &var_unk_client_code, 0, 0,
	VAR_BAD_NAME_CODE, DEF_BAD_NAME_CODE, &var_bad_name_code, 0, 0,
	VAR_UNK_NAME_CODE, DEF_UNK_NAME_CODE, &var_unk_name_code, 0, 0,
	VAR_UNK_ADDR_CODE, DEF_UNK_ADDR_CODE, &var_unk_addr_code, 0, 0,
	VAR_RELAY_CODE, DEF_RELAY_CODE, &var_relay_code, 0, 0,
	VAR_MAPS_RBL_CODE, DEF_MAPS_RBL_CODE, &var_maps_rbl_code, 0, 0,
	VAR_MAP_REJECT_CODE, DEF_MAP_REJECT_CODE, &var_map_reject_code, 0, 0,
	VAR_MAP_DEFER_CODE, DEF_MAP_DEFER_CODE, &var_map_defer_code, 0, 0,
	VAR_REJECT_CODE, DEF_REJECT_CODE, &var_reject_code, 0, 0,
	VAR_DEFER_CODE, DEF_DEFER_CODE, &var_defer_code, 0, 0,
	VAR_NON_FQDN_CODE, DEF_NON_FQDN_CODE, &var_non_fqdn_code, 0, 0,
	VAR_SMTPD_RCPT_OVERLIM, DEF_SMTPD_RCPT_OVERLIM, &var_smtpd_rcpt_overlim, 1, 0,
	VAR_SMTPD_HIST_THRSH, DEF_SMTPD_HIST_THRSH, &var_smtpd_hist_thrsh, 1, 0,
	VAR_UNV_FROM_RCODE, DEF_UNV_FROM_RCODE, &var_unv_from_rcode, 200, 599,
	VAR_UNV_RCPT_RCODE, DEF_UNV_RCPT_RCODE, &var_unv_rcpt_rcode, 200, 599,
	VAR_UNV_FROM_DCODE, DEF_UNV_FROM_DCODE, &var_unv_from_dcode, 200, 499,
	VAR_UNV_RCPT_DCODE, DEF_UNV_RCPT_DCODE, &var_unv_rcpt_dcode, 200, 499,
	VAR_MUL_RCPT_CODE, DEF_MUL_RCPT_CODE, &var_mul_rcpt_code, 0, 0,
	VAR_LOCAL_RCPT_CODE, DEF_LOCAL_RCPT_CODE, &var_local_rcpt_code, 0, 0,
	VAR_VIRT_ALIAS_CODE, DEF_VIRT_ALIAS_CODE, &var_virt_alias_code, 0, 0,
	VAR_VIRT_MAILBOX_CODE, DEF_VIRT_MAILBOX_CODE, &var_virt_mailbox_code, 0, 0,
	VAR_RELAY_RCPT_CODE, DEF_RELAY_RCPT_CODE, &var_relay_rcpt_code, 0, 0,
	VAR_PLAINTEXT_CODE, DEF_PLAINTEXT_CODE, &var_plaintext_code, 0, 0,
	VAR_SMTPD_CRATE_LIMIT, DEF_SMTPD_CRATE_LIMIT, &var_smtpd_crate_limit, 0, 0,
	VAR_SMTPD_CCONN_LIMIT, DEF_SMTPD_CCONN_LIMIT, &var_smtpd_cconn_limit, 0, 0,
	VAR_SMTPD_CMAIL_LIMIT, DEF_SMTPD_CMAIL_LIMIT, &var_smtpd_cmail_limit, 0, 0,
	VAR_SMTPD_CRCPT_LIMIT, DEF_SMTPD_CRCPT_LIMIT, &var_smtpd_crcpt_limit, 0, 0,
	VAR_SMTPD_CNTLS_LIMIT, DEF_SMTPD_CNTLS_LIMIT, &var_smtpd_cntls_limit, 0, 0,
#ifdef USE_TLS
	VAR_SMTPD_TLS_CCERT_VD, DEF_SMTPD_TLS_CCERT_VD, &var_smtpd_tls_ccert_vd, 0, 0,
#endif
	0,
    };
    static const CONFIG_TIME_TABLE time_table[] = {
	VAR_SMTPD_TMOUT, DEF_SMTPD_TMOUT, &var_smtpd_tmout, 1, 0,
	VAR_SMTPD_ERR_SLEEP, DEF_SMTPD_ERR_SLEEP, &var_smtpd_err_sleep, 0, 0,
	VAR_SMTPD_PROXY_TMOUT, DEF_SMTPD_PROXY_TMOUT, &var_smtpd_proxy_tmout, 1, 0,
	VAR_VERIFY_POLL_DELAY, DEF_VERIFY_POLL_DELAY, &var_verify_poll_delay, 1, 0,
	VAR_SMTPD_POLICY_TMOUT, DEF_SMTPD_POLICY_TMOUT, &var_smtpd_policy_tmout, 1, 0,
	VAR_SMTPD_POLICY_IDLE, DEF_SMTPD_POLICY_IDLE, &var_smtpd_policy_idle, 1, 0,
	VAR_SMTPD_POLICY_TTL, DEF_SMTPD_POLICY_TTL, &var_smtpd_policy_ttl, 1, 0,
#ifdef USE_TLS
	VAR_SMTPD_STARTTLS_TMOUT, DEF_SMTPD_STARTTLS_TMOUT, &var_smtpd_starttls_tmout, 1, 0,
	VAR_SMTPD_TLS_SCACHTIME, DEF_SMTPD_TLS_SCACHTIME, &var_smtpd_tls_scache_timeout, 0, 0,
#endif
	VAR_MILT_CONN_TIME, DEF_MILT_CONN_TIME, &var_milt_conn_time, 1, 0,
	VAR_MILT_CMD_TIME, DEF_MILT_CMD_TIME, &var_milt_cmd_time, 1, 0,
	VAR_MILT_MSG_TIME, DEF_MILT_MSG_TIME, &var_milt_msg_time, 1, 0,
	VAR_VERIFY_SENDER_TTL, DEF_VERIFY_SENDER_TTL, &var_verify_sender_ttl, 0, 0,
	0,
    };
    static const CONFIG_BOOL_TABLE bool_table[] = {
	VAR_HELO_REQUIRED, DEF_HELO_REQUIRED, &var_helo_required,
	VAR_SMTPD_DELAY_REJECT, DEF_SMTPD_DELAY_REJECT, &var_smtpd_delay_reject,
	VAR_STRICT_RFC821_ENV, DEF_STRICT_RFC821_ENV, &var_strict_rfc821_env,
	VAR_DISABLE_VRFY_CMD, DEF_DISABLE_VRFY_CMD, &var_disable_vrfy_cmd,
	VAR_ALLOW_UNTRUST_ROUTE, DEF_ALLOW_UNTRUST_ROUTE, &var_allow_untrust_route,
	VAR_SMTPD_SASL_ENABLE, DEF_SMTPD_SASL_ENABLE, &var_smtpd_sasl_enable,
	VAR_SMTPD_SASL_AUTH_HDR, DEF_SMTPD_SASL_AUTH_HDR, &var_smtpd_sasl_auth_hdr,
	VAR_BROKEN_AUTH_CLNTS, DEF_BROKEN_AUTH_CLNTS, &var_broken_auth_clients,
	VAR_SHOW_UNK_RCPT_TABLE, DEF_SHOW_UNK_RCPT_TABLE, &var_show_unk_rcpt_table,
	VAR_SMTPD_REJ_UNL_FROM, DEF_SMTPD_REJ_UNL_FROM, &var_smtpd_rej_unl_from,
	VAR_SMTPD_REJ_UNL_RCPT, DEF_SMTPD_REJ_UNL_RCPT, &var_smtpd_rej_unl_rcpt,
	VAR_SMTPD_USE_TLS, DEF_SMTPD_USE_TLS, &var_smtpd_use_tls,
	VAR_SMTPD_ENFORCE_TLS, DEF_SMTPD_ENFORCE_TLS, &var_smtpd_enforce_tls,
	VAR_SMTPD_TLS_WRAPPER, DEF_SMTPD_TLS_WRAPPER, &var_smtpd_tls_wrappermode,
	VAR_SMTPD_TLS_AUTH_ONLY, DEF_SMTPD_TLS_AUTH_ONLY, &var_smtpd_tls_auth_only,
#ifdef USE_TLS
	VAR_SMTPD_TLS_ACERT, DEF_SMTPD_TLS_ACERT, &var_smtpd_tls_ask_ccert,
	VAR_SMTPD_TLS_RCERT, DEF_SMTPD_TLS_RCERT, &var_smtpd_tls_req_ccert,
	VAR_SMTPD_TLS_RECHEAD, DEF_SMTPD_TLS_RECHEAD, &var_smtpd_tls_received_header,
	VAR_SMTPD_TLS_SET_SESSID, DEF_SMTPD_TLS_SET_SESSID, &var_smtpd_tls_set_sessid,
#endif
	VAR_SMTPD_PEERNAME_LOOKUP, DEF_SMTPD_PEERNAME_LOOKUP, &var_smtpd_peername_lookup,
	VAR_SMTPD_DELAY_OPEN, DEF_SMTPD_DELAY_OPEN, &var_smtpd_delay_open,
	VAR_SMTPD_CLIENT_PORT_LOG, DEF_SMTPD_CLIENT_PORT_LOG, &var_smtpd_client_port_log,
	0,
    };
    static const CONFIG_NBOOL_TABLE nbool_table[] = {
	VAR_SMTPD_REC_DEADLINE, DEF_SMTPD_REC_DEADLINE, &var_smtpd_rec_deadline,
	0,
    };
    static const CONFIG_STR_TABLE str_table[] = {
	VAR_SMTPD_BANNER, DEF_SMTPD_BANNER, &var_smtpd_banner, 1, 0,
	VAR_NOTIFY_CLASSES, DEF_NOTIFY_CLASSES, &var_notify_classes, 0, 0,
	VAR_CLIENT_CHECKS, DEF_CLIENT_CHECKS, &var_client_checks, 0, 0,
	VAR_HELO_CHECKS, DEF_HELO_CHECKS, &var_helo_checks, 0, 0,
	VAR_MAIL_CHECKS, DEF_MAIL_CHECKS, &var_mail_checks, 0, 0,
	VAR_RCPT_CHECKS, DEF_RCPT_CHECKS, &var_rcpt_checks, 0, 0,
	VAR_ETRN_CHECKS, DEF_ETRN_CHECKS, &var_etrn_checks, 0, 0,
	VAR_DATA_CHECKS, DEF_DATA_CHECKS, &var_data_checks, 0, 0,
	VAR_EOD_CHECKS, DEF_EOD_CHECKS, &var_eod_checks, 0, 0,
	VAR_MAPS_RBL_DOMAINS, DEF_MAPS_RBL_DOMAINS, &var_maps_rbl_domains, 0, 0,
	VAR_RBL_REPLY_MAPS, DEF_RBL_REPLY_MAPS, &var_rbl_reply_maps, 0, 0,
	VAR_ERROR_RCPT, DEF_ERROR_RCPT, &var_error_rcpt, 1, 0,
	VAR_REST_CLASSES, DEF_REST_CLASSES, &var_rest_classes, 0, 0,
	VAR_CANONICAL_MAPS, DEF_CANONICAL_MAPS, &var_canonical_maps, 0, 0,
	VAR_RCPT_CANON_MAPS, DEF_RCPT_CANON_MAPS, &var_rcpt_canon_maps, 0, 0,
	VAR_VIRT_ALIAS_MAPS, DEF_VIRT_ALIAS_MAPS, &var_virt_alias_maps, 0, 0,
	VAR_VIRT_MAILBOX_MAPS, DEF_VIRT_MAILBOX_MAPS, &var_virt_mailbox_maps, 0, 0,
	VAR_ALIAS_MAPS, DEF_ALIAS_MAPS, &var_alias_maps, 0, 0,
	VAR_LOCAL_RCPT_MAPS, DEF_LOCAL_RCPT_MAPS, &var_local_rcpt_maps, 0, 0,
	VAR_SMTPD_SASL_OPTS, DEF_SMTPD_SASL_OPTS, &var_smtpd_sasl_opts, 0, 0,
	VAR_SMTPD_SASL_PATH, DEF_SMTPD_SASL_PATH, &var_smtpd_sasl_path, 1, 0,
	VAR_CYRUS_CONF_PATH, DEF_CYRUS_CONF_PATH, &var_cyrus_conf_path, 0, 0,
	VAR_SMTPD_SASL_REALM, DEF_SMTPD_SASL_REALM, &var_smtpd_sasl_realm, 0, 0,
	VAR_SMTPD_SASL_EXCEPTIONS_NETWORKS, DEF_SMTPD_SASL_EXCEPTIONS_NETWORKS, &var_smtpd_sasl_exceptions_networks, 0, 0,
	VAR_FILTER_XPORT, DEF_FILTER_XPORT, &var_filter_xport, 0, 0,
	VAR_PERM_MX_NETWORKS, DEF_PERM_MX_NETWORKS, &var_perm_mx_networks, 0, 0,
	VAR_SMTPD_SND_AUTH_MAPS, DEF_SMTPD_SND_AUTH_MAPS, &var_smtpd_snd_auth_maps, 0, 0,
	VAR_SMTPD_NOOP_CMDS, DEF_SMTPD_NOOP_CMDS, &var_smtpd_noop_cmds, 0, 0,
	VAR_SMTPD_FORBID_CMDS, DEF_SMTPD_FORBID_CMDS, &var_smtpd_forbid_cmds, 0, 0,
	VAR_SMTPD_NULL_KEY, DEF_SMTPD_NULL_KEY, &var_smtpd_null_key, 0, 0,
	VAR_RELAY_RCPT_MAPS, DEF_RELAY_RCPT_MAPS, &var_relay_rcpt_maps, 0, 0,
	VAR_VERIFY_SENDER, DEF_VERIFY_SENDER, &var_verify_sender, 0, 0,
	VAR_VERP_CLIENTS, DEF_VERP_CLIENTS, &var_verp_clients, 0, 0,
	VAR_SMTPD_PROXY_FILT, DEF_SMTPD_PROXY_FILT, &var_smtpd_proxy_filt, 0, 0,
	VAR_SMTPD_PROXY_EHLO, DEF_SMTPD_PROXY_EHLO, &var_smtpd_proxy_ehlo, 0, 0,
	VAR_SMTPD_PROXY_OPTS, DEF_SMTPD_PROXY_OPTS, &var_smtpd_proxy_opts, 0, 0,
	VAR_INPUT_TRANSP, DEF_INPUT_TRANSP, &var_input_transp, 0, 0,
	VAR_XCLIENT_HOSTS, DEF_XCLIENT_HOSTS, &var_xclient_hosts, 0, 0,
	VAR_XFORWARD_HOSTS, DEF_XFORWARD_HOSTS, &var_xforward_hosts, 0, 0,
	VAR_SMTPD_HOGGERS, DEF_SMTPD_HOGGERS, &var_smtpd_hoggers, 0, 0,
	VAR_LOC_RWR_CLIENTS, DEF_LOC_RWR_CLIENTS, &var_local_rwr_clients, 0, 0,
	VAR_SMTPD_EHLO_DIS_WORDS, DEF_SMTPD_EHLO_DIS_WORDS, &var_smtpd_ehlo_dis_words, 0, 0,
	VAR_SMTPD_EHLO_DIS_MAPS, DEF_SMTPD_EHLO_DIS_MAPS, &var_smtpd_ehlo_dis_maps, 0, 0,
#ifdef USE_TLS
	VAR_RELAY_CCERTS, DEF_RELAY_CCERTS, &var_smtpd_relay_ccerts, 0, 0,
	VAR_SMTPD_SASL_TLS_OPTS, DEF_SMTPD_SASL_TLS_OPTS, &var_smtpd_sasl_tls_opts, 0, 0,
	VAR_SMTPD_TLS_CERT_FILE, DEF_SMTPD_TLS_CERT_FILE, &var_smtpd_tls_cert_file, 0, 0,
	VAR_SMTPD_TLS_KEY_FILE, DEF_SMTPD_TLS_KEY_FILE, &var_smtpd_tls_key_file, 0, 0,
	VAR_SMTPD_TLS_DCERT_FILE, DEF_SMTPD_TLS_DCERT_FILE, &var_smtpd_tls_dcert_file, 0, 0,
	VAR_SMTPD_TLS_DKEY_FILE, DEF_SMTPD_TLS_DKEY_FILE, &var_smtpd_tls_dkey_file, 0, 0,
	VAR_SMTPD_TLS_ECCERT_FILE, DEF_SMTPD_TLS_ECCERT_FILE, &var_smtpd_tls_eccert_file, 0, 0,
	VAR_SMTPD_TLS_ECKEY_FILE, DEF_SMTPD_TLS_ECKEY_FILE, &var_smtpd_tls_eckey_file, 0, 0,
	VAR_SMTPD_TLS_CA_FILE, DEF_SMTPD_TLS_CA_FILE, &var_smtpd_tls_CAfile, 0, 0,
	VAR_SMTPD_TLS_CA_PATH, DEF_SMTPD_TLS_CA_PATH, &var_smtpd_tls_CApath, 0, 0,
	VAR_SMTPD_TLS_CIPH, DEF_SMTPD_TLS_CIPH, &var_smtpd_tls_ciph, 1, 0,
	VAR_SMTPD_TLS_MAND_CIPH, DEF_SMTPD_TLS_MAND_CIPH, &var_smtpd_tls_mand_ciph, 1, 0,
	VAR_SMTPD_TLS_EXCL_CIPH, DEF_SMTPD_TLS_EXCL_CIPH, &var_smtpd_tls_excl_ciph, 0, 0,
	VAR_SMTPD_TLS_MAND_EXCL, DEF_SMTPD_TLS_MAND_EXCL, &var_smtpd_tls_mand_excl, 0, 0,
	VAR_SMTPD_TLS_PROTO, DEF_SMTPD_TLS_PROTO, &var_smtpd_tls_proto, 0, 0,
	VAR_SMTPD_TLS_MAND_PROTO, DEF_SMTPD_TLS_MAND_PROTO, &var_smtpd_tls_mand_proto, 0, 0,
	VAR_SMTPD_TLS_512_FILE, DEF_SMTPD_TLS_512_FILE, &var_smtpd_tls_dh512_param_file, 0, 0,
	VAR_SMTPD_TLS_1024_FILE, DEF_SMTPD_TLS_1024_FILE, &var_smtpd_tls_dh1024_param_file, 0, 0,
	VAR_SMTPD_TLS_EECDH, DEF_SMTPD_TLS_EECDH, &var_smtpd_tls_eecdh, 1, 0,
	VAR_SMTPD_TLS_FPT_DGST, DEF_SMTPD_TLS_FPT_DGST, &var_smtpd_tls_fpt_dgst, 1, 0,
	VAR_SMTPD_TLS_LOGLEVEL, DEF_SMTPD_TLS_LOGLEVEL, &var_smtpd_tls_loglevel, 0, 0,
#endif
	VAR_SMTPD_TLS_LEVEL, DEF_SMTPD_TLS_LEVEL, &var_smtpd_tls_level, 0, 0,
	VAR_SMTPD_SASL_TYPE, DEF_SMTPD_SASL_TYPE, &var_smtpd_sasl_type, 1, 0,
	VAR_SMTPD_MILTERS, DEF_SMTPD_MILTERS, &var_smtpd_milters, 0, 0,
	VAR_MILT_CONN_MACROS, DEF_MILT_CONN_MACROS, &var_milt_conn_macros, 0, 0,
	VAR_MILT_HELO_MACROS, DEF_MILT_HELO_MACROS, &var_milt_helo_macros, 0, 0,
	VAR_MILT_MAIL_MACROS, DEF_MILT_MAIL_MACROS, &var_milt_mail_macros, 0, 0,
	VAR_MILT_RCPT_MACROS, DEF_MILT_RCPT_MACROS, &var_milt_rcpt_macros, 0, 0,
	VAR_MILT_DATA_MACROS, DEF_MILT_DATA_MACROS, &var_milt_data_macros, 0, 0,
	VAR_MILT_EOH_MACROS, DEF_MILT_EOH_MACROS, &var_milt_eoh_macros, 0, 0,
	VAR_MILT_EOD_MACROS, DEF_MILT_EOD_MACROS, &var_milt_eod_macros, 0, 0,
	VAR_MILT_UNK_MACROS, DEF_MILT_UNK_MACROS, &var_milt_unk_macros, 0, 0,
	VAR_MILT_PROTOCOL, DEF_MILT_PROTOCOL, &var_milt_protocol, 1, 0,
	VAR_MILT_DEF_ACTION, DEF_MILT_DEF_ACTION, &var_milt_def_action, 1, 0,
	VAR_MILT_DAEMON_NAME, DEF_MILT_DAEMON_NAME, &var_milt_daemon_name, 1, 0,
	VAR_MILT_V, DEF_MILT_V, &var_milt_v, 1, 0,
	VAR_STRESS, DEF_STRESS, &var_stress, 0, 0,
	VAR_UNV_FROM_WHY, DEF_UNV_FROM_WHY, &var_unv_from_why, 0, 0,
	VAR_UNV_RCPT_WHY, DEF_UNV_RCPT_WHY, &var_unv_rcpt_why, 0, 0,
	VAR_REJECT_TMPF_ACT, DEF_REJECT_TMPF_ACT, &var_reject_tmpf_act, 1, 0,
	VAR_UNK_NAME_TF_ACT, DEF_UNK_NAME_TF_ACT, &var_unk_name_tf_act, 1, 0,
	VAR_UNK_ADDR_TF_ACT, DEF_UNK_ADDR_TF_ACT, &var_unk_addr_tf_act, 1, 0,
	VAR_UNV_RCPT_TF_ACT, DEF_UNV_RCPT_TF_ACT, &var_unv_rcpt_tf_act, 1, 0,
	VAR_UNV_FROM_TF_ACT, DEF_UNV_FROM_TF_ACT, &var_unv_from_tf_act, 1, 0,
	VAR_SMTPD_CMD_FILTER, DEF_SMTPD_CMD_FILTER, &var_smtpd_cmd_filter, 0, 0,
#ifdef USE_TLSPROXY
	VAR_TLSPROXY_SERVICE, DEF_TLSPROXY_SERVICE, &var_tlsproxy_service, 1, 0,
#endif
	0,
    };
    static const CONFIG_RAW_TABLE raw_table[] = {
	VAR_SMTPD_EXP_FILTER, DEF_SMTPD_EXP_FILTER, &var_smtpd_exp_filter, 1, 0,
	VAR_DEF_RBL_REPLY, DEF_DEF_RBL_REPLY, &var_def_rbl_reply, 1, 0,
	VAR_SMTPD_REJ_FOOTER, DEF_SMTPD_REJ_FOOTER, &var_smtpd_rej_footer, 0, 0,
	0,
    };

    /*
     * Fingerprint executables and core dumps.
     */
    MAIL_VERSION_STAMP_ALLOCATE;

    /*
     * Pass control to the single-threaded service skeleton.
     */
    single_server_main(argc, argv, smtpd_service,
		       MAIL_SERVER_NINT_TABLE, nint_table,
		       MAIL_SERVER_INT_TABLE, int_table,
		       MAIL_SERVER_STR_TABLE, str_table,
		       MAIL_SERVER_RAW_TABLE, raw_table,
		       MAIL_SERVER_BOOL_TABLE, bool_table,
		       MAIL_SERVER_NBOOL_TABLE, nbool_table,
		       MAIL_SERVER_TIME_TABLE, time_table,
		       MAIL_SERVER_PRE_INIT, pre_jail_init,
		       MAIL_SERVER_PRE_ACCEPT, pre_accept,
		       MAIL_SERVER_POST_INIT, post_jail_init,
		       0);
}
