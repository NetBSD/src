/*++
/* NAME
/*	smtpd 8
/* SUMMARY
/*	Postfix SMTP server
/* SYNOPSIS
/*	\fBsmtpd\fR [generic Postfix daemon options]
/* DESCRIPTION
/*	The SMTP server accepts network connection requests
/*	and performs zero or more SMTP transactions per connection.
/*	Each received message is piped through the \fBcleanup\fR(8)
/*	daemon, and is placed into the \fBincoming\fR queue as one
/*	single queue file.  For this mode of operation, the program
/*	expects to be run from the \fBmaster\fR(8) process manager.
/*
/*	Alternatively, the SMTP server takes an established
/*	connection on standard input and deposits messages directly
/*	into the \fBmaildrop\fR queue. In this so-called stand-alone
/*	mode, the SMTP server can accept mail even while the mail
/*	system is not running.
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
/*	RFC 1870 (Message Size Declaration)
/*	RFC 1985 (ETRN command)
/*	RFC 2554 (AUTH command)
/*	RFC 2821 (SMTP protocol)
/*	RFC 2920 (SMTP Pipelining)
/* DIAGNOSTICS
/*	Problems and transactions are logged to \fBsyslogd\fR(8).
/*
/*	Depending on the setting of the \fBnotify_classes\fR parameter,
/*	the postmaster is notified of bounces, protocol problems,
/*	policy violations, and of other trouble.
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	Changes to \fBmain.cf\fR are picked up automatically, as smtpd(8)
/*	processes run for only a limited amount of time. Use the command
/*	"\fBpostfix reload\fR" to speed up a change.
/*
/*	The text below provides only a parameter summary. See
/*	postconf(5) for more details including examples.
/* COMPATIBILITY CONTROLS
/* .ad
/* .fi
/*	The following parameters work around implementation errors in other
/*	software, and/or allow you to override standards in order to prevent
/*	undesirable use.
/* .ad
/* .fi
/* .IP "\fBbroken_sasl_auth_clients (no)\fR"
/*	Enable inter-operability with SMTP clients that implement an obsolete
/*	version of the AUTH command (RFC 2554).
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
/*	What SMTP clients Postfix will not offer AUTH support to.
/* AFTER QUEUE EXTERNAL CONTENT INSPECTION CONTROLS
/* .ad
/* .fi
/*	As of version 1.0, Postfix can be configured to send new mail to
/*	an external content filter AFTER the mail is queued. This content
/*	filter is expected to inject mail back into a (Postfix or other)
/*	MTA for further delivery. See the FILTER_README document for details.
/* .IP "\fBcontent_filter (empty)\fR"
/*	The name of a mail delivery transport that filters mail after
/*	it is queued.
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
/* .IP "\fBsmtpd_proxy_timeout (100s)\fR"
/*	The time limit for connecting to a proxy filter and for sending or
/*	receiving information.
/* GENERAL CONTENT INSPECTION CONTROLS
/* .ad
/* .fi
/*	The following parameters are applicable for both built-in
/*	and external content filters.
/* .PP
/*	Available in Postfix version 2.1 and later:
/* .IP "\fBreceive_override_options (empty)\fR"
/*	Enable or disable recipient validation, built-in content
/*	filtering, or address rewriting.
/* EXTERNAL CONTENT INSPECTION CONTROLS
/* .ad
/* .fi
/*	The following parameters are applicable for both before-queue
/*	and after-queue content filtering.
/* .PP
/*	Available in Postfix version 2.1 and later:
/* .IP "\fBsmtpd_authorized_xforward_hosts (empty)\fR"
/*	What SMTP clients are allowed to use the XFORWARD feature.
/* SASL AUTHENTICATION CONTROLS
/* .ad
/* .fi
/*	Postfix SASL support (RFC 2554) can be used to authenticate remote
/*	SMTP clients to the Postfix SMTP server, and to authenticate the
/*	Postfix SMTP client to a remote SMTP server.
/*	See the SASL_README document for details.
/* .IP "\fBbroken_sasl_auth_clients (no)\fR"
/*	Enable inter-operability with SMTP clients that implement an obsolete
/*	version of the AUTH command (RFC 2554).
/* .IP "\fBsmtpd_sasl_auth_enable (no)\fR"
/*	Enable SASL authentication in the Postfix SMTP server.
/* .IP "\fBsmtpd_sasl_application_name (smtpd)\fR"
/*	The application name used for SASL server initialization.
/* .IP "\fBsmtpd_sasl_local_domain (empty)\fR"
/*	The name of the local SASL authentication realm.
/* .IP "\fBsmtpd_sasl_security_options (noanonymous)\fR"
/*	Restrict what authentication mechanisms the Postfix SMTP server
/*	will offer to the client.
/* .IP "\fBsmtpd_sender_login_maps (empty)\fR"
/*	Optional lookup table with the SASL login names that own sender
/*	(MAIL FROM) addresses.
/* .PP
/*	Available in Postfix version 2.1 and later:
/* .IP "\fBsmtpd_sasl_exceptions_networks (empty)\fR"
/*	What SMTP clients Postfix will not offer AUTH support to.
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
/*	Postfix sendmail(1) command line and in SMTP commands.
/* .PP
/*	Available in Postfix version 1.1 and 2.0:
/* .IP "\fBauthorized_verp_clients ($mynetworks)\fR"
/*	What SMTP clients are allowed to specify the XVERP command.
/* .PP
/*	Available in Postfix version 2.1 and later:
/* .IP "\fBsmtpd_authorized_verp_clients ($authorized_verp_clients)\fR"
/*	What SMTP clients are allowed to specify the XVERP command.
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
/* .IP "\fBnotify_classes (resource, software)\fR"
/*	The list of error classes that are reported to the postmaster.
/* .IP "\fBsoft_bounce (no)\fR"
/*	Safety net to keep mail queued that would otherwise be returned to
/*	the sender.
/* .PP
/*	Available in Postfix version 2.1 and later:
/* .IP "\fBsmtpd_authorized_xclient_hosts (empty)\fR"
/*	What SMTP clients are allowed to use the XCLIENT feature.
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
/*	The network interface addresses that this mail system receives mail
/*	on.
/* .IP "\fBproxy_interfaces (empty)\fR"
/*	The network interface addresses that this mail system receives mail
/*	on by way of a proxy or network address translation unit.
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
/*	Optional list of names of virtual alias domains, that is,
/*	domains for which all addresses are aliased to addresses in other
/*	local or remote domains.
/* .IP "\fBvirtual_alias_maps ($virtual_maps)\fR"
/*	Optional lookup tables that alias specific mail addresses or domains
/*	to other local or remote address.
/* .IP "\fBunknown_virtual_alias_reject_code (550)\fR"
/*	The SMTP server reply code when a recipient address matches
/*	$virtual_alias_domains, and $virtual_alias_maps specifies a list
/*	of lookup tables that does not match the recipient address.
/* .PP
/*	Parameters concerning known/unknown recipients in virtual mailbox
/*	domains:
/* .IP "\fBvirtual_mailbox_domains ($virtual_mailbox_maps)\fR"
/*	The list of domains that are delivered via the $virtual_transport
/*	mail delivery transport.
/* .IP "\fBvirtual_mailbox_maps (empty)\fR"
/*	Optional lookup tables with all valid addresses in the domains that
/*	match $virtual_mailbox_domains.
/* .IP "\fBunknown_virtual_mailbox_reject_code (550)\fR"
/*	The SMTP server reply code when a recipient address matches
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
/* .IP "\fBsmtpd_timeout (300s)\fR"
/*	The time limit for sending a Postfix SMTP server response and for
/*	receiving a remote SMTP client request.
/* .IP "\fBsmtpd_history_flush_threshold (100)\fR"
/*	The maximal number of lines in the Postfix SMTP server command history
/*	before it is flushed upon receipt of EHLO, RSET, or end of DATA.
/* .PP
/*	Not available in Postfix version 2.1:
/* .IP "\fBsmtpd_client_connection_count_limit (50)\fR"
/*	How many simultaneous connections any SMTP client is allowed to
/*	make to the SMTP service.
/* .IP "\fBsmtpd_client_connection_rate_limit (0)\fR"
/*	The maximal number of connection attempts any client is allowed to
/*	make to this service per time unit.
/* .IP "\fBsmtpd_client_connection_limit_exceptions ($mynetworks)\fR"
/*	Clients that are excluded from connection count or connection rate
/*	restrictions.
/* TARPIT CONTROLS
/* .ad
/* .fi
/*	When a remote SMTP client makes errors, the Postfix SMTP server
/*	can insert delays before responding. This can help to slow down
/*	run-away software.  The behavior is controlled by an error counter
/*	that counts the number of errors within an SMTP session that a
/*	client makes without delivering mail.
/* .IP "\fBsmtpd_error_sleep_time (1s)\fR"
/*	With Postfix 2.1 and later: the SMTP server response delay after
/*	a client has made more than $smtpd_soft_error_limit errors, and
/*	fewer than $smtpd_hard_error_limit errors, without delivering mail.
/* .IP "\fBsmtpd_soft_error_limit (10)\fR"
/*	The number of errors a remote SMTP client is allowed to make without
/*	delivering mail before the Postfix SMTP server slows down all its
/*	responses.
/* .IP "\fBsmtpd_hard_error_limit (20)\fR"
/*	The maximal number of errors a remote SMTP client is allowed to
/*	make without delivering mail.
/* .IP "\fBsmtpd_junk_command_limit (100)\fR"
/*	The number of junk commands (NOOP, VRFY, ETRN or RSET) that a remote
/*	SMTP client can send before the Postfix SMTP server starts to
/*	increment the error counter with each junk command.
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
/* .IP "\fBsmtpd_policy_service_timeout (100s)\fR"
/*	The time limit for connecting to, writing to or receiving from a
/*	delegated SMTPD policy server.
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
/*	Optional SMTP server access restrictions in the context of a client
/*	SMTP connection request.
/* .IP "\fBsmtpd_helo_required (no)\fR"
/*	Require that a remote SMTP client introduces itself at the beginning
/*	of an SMTP session with the HELO or EHLO command.
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
/*	The lookup key to be used in SMTP access(5) tables instead of the
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
/* SENDER AND RECIPIENT ADDRESS VERIFICATION CONTROLS
/* .ad
/* .fi
/*	Postfix version 2.1 introduces sender and address verification.
/*	This feature is implemented by sending probe email messages that
/*	are not actually delivered.
/*	This feature is requested via the reject_unverified_sender and
/*	reject_unverified_recipient access restrictions.  The status of
/*	verification probes is maintained by the verify(8) server.
/*	See the file ADDRESS_VERIFICATION_README for information
/*	about how to configure and operate the Postfix sender/recipient
/*	address verification service.
/* .IP "\fBaddress_verify_poll_count (3)\fR"
/*	How many times to query the verify(8) service for the completion
/*	of an address verification request in progress.
/* .IP "\fBaddress_verify_poll_delay (3s)\fR"
/*	The delay between queries for the completion of an address
/*	verification request in progress.
/* .IP "\fBaddress_verify_sender (postmaster)\fR"
/*	The sender address to use in address verification probes.
/* .IP "\fBunverified_sender_reject_code (450)\fR"
/*	The numerical Postfix SMTP server response code when a recipient
/*	address is rejected by the reject_unverified_sender restriction.
/* .IP "\fBunverified_recipient_reject_code (450)\fR"
/*	The numerical Postfix SMTP server response when a recipient address
/*	is rejected by the reject_unverified_recipient restriction.
/* ACCESS CONTROL RESPONSES
/* .ad
/* .fi
/*	The following parameters control numerical SMTP reply codes
/*	and/or text responses.
/* .IP "\fBaccess_map_reject_code (554)\fR"
/*	The numerical Postfix SMTP server response code when a client
/*	is rejected by an access(5) map restriction.
/* .IP "\fBdefer_code (450)\fR"
/*	The numerical Postfix SMTP server response code when a remote SMTP
/*	client request is rejected by the "defer" restriction.
/* .IP "\fBinvalid_hostname_reject_code (501)\fR"
/*	The numerical Postfix SMTP server response code when the client
/*	HELO or EHLO command parameter is rejected by the reject_invalid_hostname
/*	restriction.
/* .IP "\fBmaps_rbl_reject_code (554)\fR"
/*	The numerical Postfix SMTP server response code when a remote SMTP
/*	client request is blocked by the reject_rbl_client, reject_rhsbl_client,
/*	reject_rhsbl_sender or reject_rhsbl_recipient restriction.
/* .IP "\fBnon_fqdn_reject_code (504)\fR"
/*	The numerical Postfix SMTP server reply code when a client request
/*	is rejected by the reject_non_fqdn_hostname, reject_non_fqdn_sender
/*	or reject_non_fqdn_recipient restriction.
/* .IP "\fBreject_code (554)\fR"
/*	The numerical Postfix SMTP server response code when a remote SMTP
/*	client request is rejected by the "\fBreject\fR" restriction.
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
/*	reject_unknown_client restriction.
/* .IP "\fBunknown_hostname_reject_code (450)\fR"
/*	The numerical Postfix SMTP server response code when the hostname
/*	specified with the HELO or EHLO command is rejected by the
/*	reject_unknown_hostname restriction.
/* .PP
/*	Available in Postfix version 2.0 and later:
/* .IP "\fBdefault_rbl_reply (see 'postconf -d' output)\fR"
/*	The default SMTP server response template for a request that is
/*	rejected by an RBL-based restriction.
/* .IP "\fBmulti_recipient_bounce_reject_code (550)\fR"
/*	The numerical Postfix SMTP server response code when a remote SMTP
/*	client request is blocked by the reject_multi_recipient_bounce
/*	restriction.
/* .IP "\fBrbl_reply_maps (empty)\fR"
/*	Optional lookup tables with RBL response templates.
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
/*	The maximum amount of time that an idle Postfix daemon process
/*	waits for the next service request before exiting.
/* .IP "\fBmax_use (100)\fR"
/*	The maximal number of connection requests before a Postfix daemon
/*	process terminates.
/* .IP "\fBmyhostname (see 'postconf -d' output)\fR"
/*	The internet hostname of this mail system.
/* .IP "\fBmynetworks (see 'postconf -d' output)\fR"
/*	The list of "trusted" SMTP clients that have more privileges than
/*	"strangers".
/* .IP "\fBmyorigin ($myhostname)\fR"
/*	The default domain name that locally-posted mail appears to come
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
/* .IP "\fBsyslog_name (postfix)\fR"
/*	The mail system name that is prepended to the process name in syslog
/*	records, so that "smtpd" becomes, for example, "postfix/smtpd".
/* SEE ALSO
/*	cleanup(8), message canonicalization
/*	trivial-rewrite(8), address resolver
/*	verify(8), address verification service
/*	postconf(5), configuration parameters
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
/*	FILTER_README, external after-queue content filter
/*	LOCAL_RECIPIENT_README, blocking unknown local recipients
/*	SMTPD_ACCESS_README, built-in access policies
/*	SMTPD_POLICY_README, external policy server
/*	SMTPD_PROXY_README, external before-queue content filter
/*	SASL_README, Postfix SASL howto
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
#include <time.h>
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

/* Global library. */

#include <mail_params.h>
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
#ifdef SNAPSHOT
#include <anvil_clnt.h>
#endif
#include <flush_clnt.h>

/* Single-threaded server skeleton. */

#include <mail_server.h>

/* Application-specific */

#include <smtpd_token.h>
#include <smtpd.h>
#include <smtpd_check.h>
#include <smtpd_chat.h>
#include <smtpd_sasl_proto.h>
#include <smtpd_sasl_glue.h>
#include <smtpd_proxy.h>

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
int     var_unk_client_code;
int     var_bad_name_code;
int     var_unk_name_code;
int     var_unk_addr_code;
int     var_relay_code;
int     var_maps_rbl_code;
int     var_access_map_code;
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
char   *var_smtpd_sasl_opts;
char   *var_smtpd_sasl_appname;
char   *var_smtpd_sasl_realm;
char   *var_smtpd_sasl_exceptions_networks;
char   *var_filter_xport;
bool    var_broken_auth_clients;
char   *var_perm_mx_networks;
char   *var_smtpd_snd_auth_maps;
char   *var_smtpd_noop_cmds;
char   *var_smtpd_null_key;
int     var_smtpd_hist_thrsh;
char   *var_smtpd_exp_filter;
char   *var_def_rbl_reply;
int     var_unv_from_code;
int     var_unv_rcpt_code;
int     var_mul_rcpt_code;
char   *var_relay_rcpt_maps;
char   *var_verify_sender;
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
char   *var_input_transp;
int     var_smtpd_policy_tmout;
int     var_smtpd_policy_idle;
int     var_smtpd_policy_ttl;
char   *var_xclient_hosts;
char   *var_xforward_hosts;
bool    var_smtpd_rej_unl_from;
bool    var_smtpd_rej_unl_rcpt;

#ifdef SNAPSHOT
int     var_smtpd_crate_limit;
int     var_smtpd_cconn_limit;
char   *var_smtpd_hoggers;

#endif

 /*
  * Silly little macros.
  */
#define STR(x)	vstring_str(x)
#define LEN(x)	VSTRING_LEN(x)

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
static int xclient_allowed;

 /*
  * XFORWARD command. Access control is cached.
  */
static NAMADR_LIST *xforward_hosts;
static int xforward_allowed;

 /*
  * Client connection and rate limiting.
  */
#ifdef SNAPSHOT
ANVIL_CLNT *anvil_clnt;
static NAMADR_LIST *hogger_list;

#endif

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
static void chat_reset(SMTPD_STATE *, int);

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

    match = namadr_list_match(sasl_exceptions_networks,
			      state->name, state->addr);

    if (msg_verbose)
	msg_info("sasl_exceptions: %s[%s], match=%d",
		 state->name, state->addr, match);

    return (match);
}

#endif

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

/* helo_cmd - process HELO command */

static int helo_cmd(SMTPD_STATE *state, int argc, SMTPD_TOKEN *argv)
{
    char   *err;

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
    if (state->helo_name != 0)
	helo_reset(state);
    chat_reset(state, var_smtpd_hist_thrsh);
    mail_reset(state);
    rcpt_reset(state);
    state->helo_name = mystrdup(printable(argv[1].strval, '?'));
    neuter(state->helo_name, "<>()\\\";:@", '?');
    /* Downgrading the protocol name breaks the unauthorized pipelining test. */
    if (strcasecmp(state->protocol, MAIL_PROTO_ESMTP) != 0
	&& strcasecmp(state->protocol, MAIL_PROTO_SMTP) != 0) {
	myfree(state->protocol);
	state->protocol = mystrdup(MAIL_PROTO_SMTP);
    }
    smtpd_chat_reply(state, "250 %s", var_myhostname);
    return (0);
}

/* ehlo_cmd - process EHLO command */

static int ehlo_cmd(SMTPD_STATE *state, int argc, SMTPD_TOKEN *argv)
{
    char   *err;

    /*
     * XXX 2821 new feature: Section 4.1.4 specifies that a server must clear
     * all buffers and reset the state exactly as if a RSET command had been
     * issued.
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
    if (state->helo_name != 0)
	helo_reset(state);
    chat_reset(state, var_smtpd_hist_thrsh);
    mail_reset(state);
    rcpt_reset(state);
    state->helo_name = mystrdup(printable(argv[1].strval, '?'));
    neuter(state->helo_name, "<>()\\\";:@", '?');
    if (strcasecmp(state->protocol, MAIL_PROTO_ESMTP) != 0) {
	myfree(state->protocol);
	state->protocol = mystrdup(MAIL_PROTO_ESMTP);
    }
    smtpd_chat_reply(state, "250-%s", var_myhostname);
    smtpd_chat_reply(state, "250-PIPELINING");
    if (var_message_limit)
	smtpd_chat_reply(state, "250-SIZE %lu",
			 (unsigned long) var_message_limit);	/* XXX */
    else
	smtpd_chat_reply(state, "250-SIZE");
    if (var_disable_vrfy_cmd == 0)
	smtpd_chat_reply(state, "250-VRFY");
    smtpd_chat_reply(state, "250-ETRN");
#ifdef USE_SASL_AUTH
    if (var_smtpd_sasl_enable && !sasl_client_exception(state)) {
	smtpd_chat_reply(state, "250-AUTH %s", state->sasl_mechanism_list);
	if (var_broken_auth_clients)
	    smtpd_chat_reply(state, "250-AUTH=%s", state->sasl_mechanism_list);
    }
#endif
    if (namadr_list_match(verp_clients, state->name, state->addr))
	smtpd_chat_reply(state, "250-%s", VERP_CMD);
    /* XCLIENT must not override its own access control. */
    if (xclient_allowed)
	smtpd_chat_reply(state, "250-" XCLIENT_CMD
			 " " XCLIENT_NAME " " XCLIENT_ADDR
			 " " XCLIENT_PROTO " " XCLIENT_HELO);
    if (xforward_allowed)
	smtpd_chat_reply(state, "250-" XFORWARD_CMD
			 " " XFORWARD_NAME " " XFORWARD_ADDR
			 " " XFORWARD_PROTO " " XFORWARD_HELO);
    smtpd_chat_reply(state, "250 8BITMIME");
    return (0);
}

/* helo_reset - reset HELO/EHLO command stuff */

static void helo_reset(SMTPD_STATE *state)
{
    if (state->helo_name)
	myfree(state->helo_name);
    state->helo_name = 0;
}

/* mail_open_stream - open mail queue file or IPC stream */

static void mail_open_stream(SMTPD_STATE *state)
{
    char   *postdrop_command;
    int     cleanup_flags;

    /*
     * XXX 2821: An SMTP server is not allowed to "clean up" mail except in
     * the case of original submissions. Presently, Postfix always runs all
     * mail through the cleanup server.
     * 
     * We could approximate the RFC as follows: Postfix rewrites mail if it
     * comes from a source that we are willing to relay for. This way, we
     * avoid rewriting most mail that comes from elsewhere. However, that
     * requires moving functionality away from the cleanup daemon elsewhere,
     * such as virtual address expansion, and header/body pattern matching.
     */

    /*
     * If running from the master or from inetd, connect to the cleanup
     * service.
     */
    cleanup_flags = CLEANUP_FLAG_MASK_EXTERNAL;
    if (smtpd_input_transp_mask & INPUT_TRANSP_ADDRESS_MAPPING)
	cleanup_flags &= ~(CLEANUP_FLAG_BCC_OK | CLEANUP_FLAG_MAP_OK);
    if (smtpd_input_transp_mask & INPUT_TRANSP_HEADER_BODY)
	cleanup_flags &= ~CLEANUP_FLAG_FILTER;

    if (SMTPD_STAND_ALONE(state) == 0) {
	state->dest = mail_stream_service(MAIL_CLASS_PUBLIC,
					  var_cleanup_service);
	if (state->dest == 0
	    || attr_print(state->dest->stream, ATTR_FLAG_NONE,
			  ATTR_TYPE_NUM, MAIL_ATTR_FLAGS, cleanup_flags,
			  ATTR_TYPE_END) != 0)
	    msg_fatal("unable to connect to the %s %s service",
		      MAIL_CLASS_PUBLIC, var_cleanup_service);
    }

    /*
     * Otherwise, pipe the message through the privileged postdrop helper.
     * XXX Make postdrop a manifest constant.
     */
    else {
	postdrop_command = concatenate(var_command_dir, "/postdrop",
			      msg_verbose ? " -v" : (char *) 0, (char *) 0);
	state->dest = mail_stream_command(postdrop_command);
	if (state->dest == 0)
	    msg_fatal("unable to execute %s", postdrop_command);
	myfree(postdrop_command);
    }
    state->cleanup = state->dest->stream;
    state->queue_id = mystrdup(state->dest->id);

    /*
     * Record the time of arrival, the sender envelope address, some session
     * information, and some additional attributes.
     */
    if (SMTPD_STAND_ALONE(state) == 0) {
	rec_fprintf(state->cleanup, REC_TYPE_TIME, "%ld", state->time);
	if (*var_filter_xport)
	    rec_fprintf(state->cleanup, REC_TYPE_FILT, "%s", var_filter_xport);
    }
    rec_fputs(state->cleanup, REC_TYPE_FROM, state->sender);
    if (state->encoding != 0)
	rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%s",
		    MAIL_ATTR_ENCODING, state->encoding);

    /*
     * Store the client attributes for logging purposes.
     */
    if (SMTPD_STAND_ALONE(state) == 0) {
	if (IS_AVAIL_CLIENT_NAME(FORWARD_NAME(state)))
	    rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%s",
			MAIL_ATTR_CLIENT_NAME, FORWARD_NAME(state));
	if (IS_AVAIL_CLIENT_ADDR(FORWARD_ADDR(state)))
	    rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%s",
			MAIL_ATTR_CLIENT_ADDR, FORWARD_ADDR(state));
	if (IS_AVAIL_CLIENT_NAMADDR(FORWARD_NAMADDR(state)))
	    rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%s",
			MAIL_ATTR_ORIGIN, FORWARD_NAMADDR(state));
	if (IS_AVAIL_CLIENT_HELO(FORWARD_HELO(state)))
	    rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%s",
			MAIL_ATTR_HELO_NAME, FORWARD_HELO(state));
	if (IS_AVAIL_CLIENT_PROTO(FORWARD_PROTO(state)))
	    rec_fprintf(state->cleanup, REC_TYPE_ATTR, "%s=%s",
			MAIL_ATTR_PROTO_NAME, FORWARD_PROTO(state));
    }
    if (state->verp_delims)
	rec_fputs(state->cleanup, REC_TYPE_VERP, state->verp_delims);
}

/* extract_addr - extract address from rubble */

static char *extract_addr(SMTPD_STATE *state, SMTPD_TOKEN *arg,
			          int allow_empty_addr, int strict_rfc821)
{
    char   *myname = "extract_addr";
    TOK822 *tree;
    TOK822 *tp;
    TOK822 *addr = 0;
    int     naddr;
    int     non_addr;
    char   *err = 0;
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
     * Report trouble. Log a warning only if we are going to sleep+reject so
     * that attackers can't flood our logfiles.
     */
    if (naddr > 1
	|| (strict_rfc821 && (non_addr || *STR(arg->vstrval) != '<'))) {
	msg_warn("Illegal address syntax from %s in %s command: %s",
		 state->namaddr, state->where, STR(arg->vstrval));
	err = "501 Bad address syntax";
    }

    /*
     * Overwrite the input with the extracted address. This seems bad design,
     * but we really are not going to use the original data anymore. What we
     * start with is quoted (external) form, and what we need is unquoted
     * (internal form).
     */
    if (addr)
	tok822_internalize(arg->vstrval, addr->head, TOK822_STR_DEFL);
    else
	vstring_strcpy(arg->vstrval, "");
    arg->strval = STR(arg->vstrval);

    /*
     * Report trouble. Log a warning only if we are going to sleep+reject so
     * that attackers can't flood our logfiles.
     */
    if (err == 0)
	if ((arg->strval[0] == 0 && !allow_empty_addr)
	    || (strict_rfc821 && arg->strval[0] == '@')
	    || (SMTPD_STAND_ALONE(state) == 0
		&& smtpd_check_addr(STR(arg->vstrval)) != 0)) {
	    msg_warn("Illegal address syntax from %s in %s command: %s",
		     state->namaddr, state->where, STR(arg->vstrval));
	    err = "501 Bad address syntax";
	}

    /*
     * Cleanup.
     */
    tok822_free_tree(tree);
    if (msg_verbose)
	msg_info("%s: result: %s", myname, STR(arg->vstrval));
    return (err);
}

/* mail_cmd - process MAIL command */

static int mail_cmd(SMTPD_STATE *state, int argc, SMTPD_TOKEN *argv)
{
    char   *err;
    int     narg;
    char   *arg;
    char   *verp_delims = 0;

    state->encoding = 0;

    /*
     * Sanity checks.
     * 
     * XXX 2821 pedantism: Section 4.1.2 says that SMTP servers that receive a
     * command in which invalid character codes have been employed, and for
     * which there are no other reasons for rejection, MUST reject that
     * command with a 501 response. So much for the principle of "be liberal
     * in what you accept, be strict in what you send".
     */
    if (var_helo_required && state->helo_name == 0) {
	state->error_mask |= MAIL_ERROR_POLICY;
	smtpd_chat_reply(state, "503 Error: send HELO/EHLO first");
	return (-1);
    }
#define IN_MAIL_TRANSACTION(state) ((state)->sender != 0)

    if (IN_MAIL_TRANSACTION(state)) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "503 Error: nested MAIL command");
	return (-1);
    }
    if (argc < 3
	|| strcasecmp(argv[1].strval, "from:") != 0) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "501 Syntax: MAIL FROM: <address>");
	return (-1);
    }
    if (argv[2].tokval == SMTPD_TOK_ERROR) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "501 Bad sender address syntax");
	return (-1);
    }
    if ((err = extract_addr(state, argv + 2, PERMIT_EMPTY_ADDR, var_strict_rfc821_env)) != 0) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "%s", err);
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
		smtpd_chat_reply(state, "501 Bad message size syntax");
		return (-1);
	    }
	    /* Reject size overflow. */
	    if ((state->msg_size = off_cvt_string(arg + 5)) < 0) {
		smtpd_chat_reply(state, "552 Message size exceeds file system imposed limit");
		state->error_mask |= MAIL_ERROR_POLICY;
		return (-1);
	    }
#ifdef USE_SASL_AUTH
	} else if (var_smtpd_sasl_enable && strncasecmp(arg, "AUTH=", 5) == 0) {
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
		    smtpd_chat_reply(state, "501 Error: %s needs two characters from %s",
				     VERP_CMD, var_verp_filter);
		    return (-1);
		}
	    }
	} else {
	    state->error_mask |= MAIL_ERROR_PROTOCOL;
	    smtpd_chat_reply(state, "555 Unsupported option: %s", arg);
	    return (-1);
	}
    }
    if (verp_delims && argv[2].strval[0] == 0) {
	smtpd_chat_reply(state, "503 Error: %s requires non-null sender",
			 VERP_CMD);
	return (-1);
    }
    if (SMTPD_STAND_ALONE(state) == 0
	&& var_smtpd_delay_reject == 0
	&& (err = smtpd_check_mail(state, argv[2].strval)) != 0) {
	smtpd_chat_reply(state, "%s", err);
	/* XXX Reset access map side effects. */
	mail_reset(state);
	return (-1);
    }

    /*
     * Check the queue file space, if applicable.
     */
    if (!USE_SMTPD_PROXY(state)) {
	if ((err = smtpd_check_size(state, state->msg_size)) != 0) {
	    smtpd_chat_reply(state, "%s", err);
	    return (-1);
	}
    }

    /*
     * No more early returns. The mail transaction is in progress.
     */
    state->time = time((time_t *) 0);
    state->sender = mystrdup(argv[2].strval);
    vstring_sprintf(state->instance, "%x.%lx.%x",
		    var_pid, (unsigned long) state->time, state->seqno++);
    if (verp_delims)
	state->verp_delims = mystrdup(verp_delims);
    if (USE_SMTPD_PROXY(state))
	state->proxy_mail = mystrdup(STR(state->buffer));
    smtpd_chat_reply(state, "250 Ok");
    return (0);
}

/* mail_reset - reset MAIL command stuff */

static void mail_reset(SMTPD_STATE *state)
{
    state->msg_size = 0;

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
    state->saved_flags = 0;
#ifdef USE_SASL_AUTH
    if (var_smtpd_sasl_enable)
	smtpd_sasl_mail_reset(state);
#endif
    state->discard = 0;
    VSTRING_RESET(state->instance);
    VSTRING_TERMINATE(state->instance);

    /*
     * Try to be nice. Don't bother when we lost the connection. Don't bother
     * waiting for a reply, it just increases latency.
     */
    if (state->proxy) {
	(void) smtpd_proxy_cmd(state, SMTPD_PROX_WANT_NONE, "QUIT");
	smtpd_proxy_close(state);
    }
    if (state->xforward.flags)
	smtpd_xforward_reset(state);
    if (state->prepend)
	state->prepend = argv_free(state->prepend);
}

/* rcpt_cmd - process RCPT TO command */

static int rcpt_cmd(SMTPD_STATE *state, int argc, SMTPD_TOKEN *argv)
{
    char   *err;
    int     narg;
    char   *arg;

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
	smtpd_chat_reply(state, "503 Error: need MAIL command");
	return (-1);
    }
    if (argc < 3
	|| strcasecmp(argv[1].strval, "to:") != 0) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "501 Syntax: RCPT TO: <address>");
	return (-1);
    }
    if (argv[2].tokval == SMTPD_TOK_ERROR) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "501 Bad recipient address syntax");
	return (-1);
    }
    if ((err = extract_addr(state, argv + 2, REJECT_EMPTY_ADDR, var_strict_rfc821_env)) != 0) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "%s", err);
	return (-1);
    }
    for (narg = 3; narg < argc; narg++) {
	arg = argv[narg].strval;
	if (1) {
	    state->error_mask |= MAIL_ERROR_PROTOCOL;
	    smtpd_chat_reply(state, "555 Unsupported option: %s", arg);
	    return (-1);
	}
    }
    if (var_smtpd_rcpt_limit && state->rcpt_count >= var_smtpd_rcpt_limit) {
	smtpd_chat_reply(state, "452 Error: too many recipients");
	if (state->rcpt_overshoot++ < var_smtpd_rcpt_overlim)
	    return (0);
	state->error_mask |= MAIL_ERROR_POLICY;
	return (-1);
    }
    if (SMTPD_STAND_ALONE(state) == 0) {
	if ((err = smtpd_check_rcpt(state, argv[2].strval)) != 0) {
	    smtpd_chat_reply(state, "%s", err);
	    return (-1);
	}
    }

    /*
     * Don't access the proxy, queue file, or queue file writer process until
     * we have a valid recipient address.
     */
    if (state->proxy == 0 && state->cleanup == 0) {
	if (state->proxy_mail) {
	    if (smtpd_proxy_open(state, var_smtpd_proxy_filt,
				 var_smtpd_proxy_tmout, var_smtpd_proxy_ehlo,
				 state->proxy_mail) != 0) {
		smtpd_chat_reply(state, "%s", STR(state->proxy_buffer));
		return (-1);
	    }
	} else {
	    mail_open_stream(state);
	}

	/*
	 * Log the queue ID with the message origin.
	 */
#ifdef USE_SASL_AUTH
	if (var_smtpd_sasl_enable)
	    smtpd_sasl_mail_log(state);
	else
#endif
	    msg_info("%s: client=%s", state->queue_id ?
		     state->queue_id : "NOQUEUE", FORWARD_NAMADDR(state));
    }

    /*
     * Proxy the recipient. OK, so we lied. If the real-time proxy rejects
     * the recipient then we can have a proxy connection without having
     * accepted a recipient.
     */
    if (state->proxy && smtpd_proxy_cmd(state, SMTPD_PROX_WANT_OK,
					"%s", STR(state->buffer)) != 0) {
	smtpd_chat_reply(state, "%s", STR(state->proxy_buffer));
	return (-1);
    }

    /*
     * Store the recipient. Remember the first one.
     * 
     * Flush recipients to maintain a stiffer coupling with the next stage and
     * to better utilize parallelism.
     */
    state->rcpt_count++;
    if (state->recipient == 0)
	state->recipient = mystrdup(argv[2].strval);
    if (state->cleanup) {
	rec_fputs(state->cleanup, REC_TYPE_RCPT, argv[2].strval);
	vstream_fflush(state->cleanup);
    }
    smtpd_chat_reply(state, "250 Ok");
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

/* data_cmd - process DATA command */

static int data_cmd(SMTPD_STATE *state, int argc, SMTPD_TOKEN *unused_argv)
{
    char   *err;
    char   *start;
    int     len;
    int     curr_rec_type;
    int     prev_rec_type;
    int     first = 1;
    VSTRING *why = 0;
    int     saved_err;
    int     (*out_record) (VSTREAM *, int, const char *, int);
    int     (*out_fprintf) (VSTREAM *, int, const char *,...);
    VSTREAM *out_stream;
    int     out_error;
    char  **cpp;

    /*
     * Sanity checks. With ESMTP command pipelining the client can send DATA
     * before all recipients are rejected, so don't report that as a protocol
     * error.
     */
    if (state->rcpt_count == 0) {
	if (!IN_MAIL_TRANSACTION(state)) {
	    state->error_mask |= MAIL_ERROR_PROTOCOL;
	    smtpd_chat_reply(state, "503 Error: need RCPT command");
	} else {
	    smtpd_chat_reply(state, "554 Error: no valid recipients");
	}
	return (-1);
    }
    if (argc != 1) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "501 Syntax: DATA");
	return (-1);
    }
    if (SMTPD_STAND_ALONE(state) == 0 && (err = smtpd_check_data(state)) != 0) {
	smtpd_chat_reply(state, "%s", err);
	return (-1);
    }
    if (state->proxy && smtpd_proxy_cmd(state, SMTPD_PROX_WANT_MORE,
					"%s", STR(state->buffer)) != 0) {
	smtpd_chat_reply(state, "%s", STR(state->proxy_buffer));
	return (-1);
    }

    /*
     * One level of indirection to choose between normal or proxied
     * operation. We want to avoid massive code duplication within tons of
     * if-else clauses.
     */
    if (state->proxy) {
	out_stream = state->proxy;
	out_record = smtpd_proxy_rec_put;
	out_fprintf = smtpd_proxy_rec_fprintf;
	out_error = CLEANUP_STAT_PROXY;
    } else {
	out_stream = state->cleanup;
	out_record = rec_put;
	out_fprintf = rec_fprintf;
	out_error = CLEANUP_STAT_WRITE;
    }

    /*
     * Flush out any access table actions that are delegated to the cleanup
     * server, and that may trigger before we accept the first valid
     * recipient.
     * 
     * Terminate the message envelope segment. Start the message content
     * segment, and prepend our own Received: header. If there is only one
     * recipient, list the recipient address.
     */
    if (state->cleanup) {
	if (state->saved_filter)
	    rec_fprintf(state->cleanup, REC_TYPE_FILT, "%s", state->saved_filter);
	if (state->saved_redirect)
	    rec_fprintf(state->cleanup, REC_TYPE_RDR, "%s", state->saved_redirect);
	if (state->saved_flags)
	    rec_fprintf(state->cleanup, REC_TYPE_FLGS, "%d", state->saved_flags);
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
    if (!state->proxy || state->xforward.flags == 0) {
	out_fprintf(out_stream, REC_TYPE_NORM,
		    "Received: from %s (%s [%s])",
		    state->helo_name ? state->helo_name : state->name,
		    state->name, state->addr);
	if (state->rcpt_count == 1 && state->recipient) {
	    out_fprintf(out_stream, REC_TYPE_NORM,
			state->cleanup ? "\tby %s (%s) with %s id %s" :
			"\tby %s (%s) with %s",
			var_myhostname, var_mail_name,
			state->protocol, state->queue_id);
	    quote_822_local(state->buffer, state->recipient);
	    out_fprintf(out_stream, REC_TYPE_NORM,
	      "\tfor <%s>; %s", STR(state->buffer), mail_date(state->time));
	} else {
	    out_fprintf(out_stream, REC_TYPE_NORM,
			state->cleanup ? "\tby %s (%s) with %s id %s;" :
			"\tby %s (%s) with %s;",
			var_myhostname, var_mail_name,
			state->protocol, state->queue_id);
	    out_fprintf(out_stream, REC_TYPE_NORM,
			"\t%s", mail_date(state->time));
	}
#ifdef RECEIVED_ENVELOPE_FROM
	quote_822_local(state->buffer, state->sender);
	out_fprintf(out_stream, REC_TYPE_NORM,
		    "\t(envelope-from %s)", STR(state->buffer));
#endif
    }
    smtpd_chat_reply(state, "354 End data with <CR><LF>.<CR><LF>");

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
	if (smtp_get(state->buffer, state->client, var_line_limit) == '\n')
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
	    && (state->proxy == 0 ? (++start, --len) == 0 : len == 1))
	    break;
	if (state->err == CLEANUP_STAT_OK
	    && out_record(out_stream, curr_rec_type, start, len) < 0)
	    state->err = out_error;
    }

    /*
     * Send the end of DATA and finish the proxy connection. Set the
     * CLEANUP_STAT_PROXY error flag in case of trouble.
     * 
     * XXX The low-level proxy output routines should set "state" error
     * attributes. This requires making "state" a context attribute of the
     * VSTREAM.
     */
    if (state->proxy) {
	if (state->err == CLEANUP_STAT_OK) {
	    (void) smtpd_proxy_cmd(state, SMTPD_PROX_WANT_ANY, ".");
	    if (state->err == CLEANUP_STAT_OK &&
		*STR(state->proxy_buffer) != '2')
		state->err = CLEANUP_STAT_CONT;
	} else {
	    state->error_mask |= MAIL_ERROR_SOFTWARE;
	    state->err |= CLEANUP_STAT_PROXY;
	    vstring_sprintf(state->proxy_buffer,
			    "451 Error: queue file write error");
	}
    }

    /*
     * Send the end-of-segment markers and finish the queue file record
     * stream.
     */
    else {
	if (state->err == CLEANUP_STAT_OK)
	    if (rec_fputs(state->cleanup, REC_TYPE_XTRA, "") < 0
		|| rec_fputs(state->cleanup, REC_TYPE_END, "") < 0
		|| vstream_fflush(state->cleanup))
		state->err = CLEANUP_STAT_WRITE;
	if (state->err == 0) {
	    why = vstring_alloc(10);
	    state->err = mail_stream_finish(state->dest, why);
	} else
	    mail_stream_cleanup(state->dest);
	state->dest = 0;
	state->cleanup = 0;
    }

    /*
     * Handle any errors. One message may suffer from multiple errors, so
     * complain only about the most severe error. Forgive any previous client
     * errors when a message was received successfully.
     * 
     * See also: qmqpd.c
     */
    if (state->err == CLEANUP_STAT_OK) {
	state->error_count = 0;
	state->error_mask = 0;
	state->junk_cmds = 0;
	if (state->queue_id)
	    smtpd_chat_reply(state, "250 Ok: queued as %s", state->queue_id);
	else
	    smtpd_chat_reply(state, "%s", STR(state->proxy_buffer));
    } else if ((state->err & CLEANUP_STAT_BAD) != 0) {
	state->error_mask |= MAIL_ERROR_SOFTWARE;
	smtpd_chat_reply(state, "451 Error: internal error %d", state->err);
    } else if ((state->err & CLEANUP_STAT_SIZE) != 0) {
	state->error_mask |= MAIL_ERROR_BOUNCE;
	smtpd_chat_reply(state, "552 Error: message too large");
    } else if ((state->err & CLEANUP_STAT_HOPS) != 0) {
	state->error_mask |= MAIL_ERROR_BOUNCE;
	smtpd_chat_reply(state, "554 Error: too many hops");
    } else if ((state->err & CLEANUP_STAT_CONT) != 0) {
	state->error_mask |= MAIL_ERROR_POLICY;
	if (state->proxy_buffer)
	    smtpd_chat_reply(state, "%s", STR(state->proxy_buffer));
	else
	    smtpd_chat_reply(state, "550 Error: %s", LEN(why) ?
			     STR(why) : "content rejected");
    } else if ((state->err & CLEANUP_STAT_WRITE) != 0) {
	state->error_mask |= MAIL_ERROR_RESOURCE;
	smtpd_chat_reply(state, "451 Error: queue file write error");
    } else if ((state->err & CLEANUP_STAT_PROXY) != 0) {
	state->error_mask |= MAIL_ERROR_SOFTWARE;
	smtpd_chat_reply(state, "%s", STR(state->proxy_buffer));
    } else {
	state->error_mask |= MAIL_ERROR_SOFTWARE;
	smtpd_chat_reply(state, "451 Error: internal error %d", state->err);
    }

    /*
     * Disconnect after transmission must not be treated as "lost connection
     * after DATA".
     */
    state->where = SMTPD_AFTER_DOT;

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
	smtpd_chat_reply(state, "501 Syntax: RSET");
	return (-1);
    }

    /*
     * Restore state to right after HELO/EHLO command.
     */
    chat_reset(state, var_smtpd_hist_thrsh);
    mail_reset(state);
    rcpt_reset(state);
    smtpd_chat_reply(state, "250 Ok");
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
	smtpd_chat_reply(state, "501 Syntax: NOOP");
	return (-1);
    }
#endif
    smtpd_chat_reply(state, "250 Ok");
    return (0);
}

/* vrfy_cmd - process VRFY */

static int vrfy_cmd(SMTPD_STATE *state, int argc, SMTPD_TOKEN *argv)
{
    char   *err = 0;

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
	smtpd_chat_reply(state, "502 VRFY command is disabled");
	return (-1);
    }
    if (argc < 2) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "501 Syntax: VRFY address");
	return (-1);
    }
    if (argc > 2)
	collapse_args(argc - 1, argv + 1);
    if ((err = extract_addr(state, argv + 1, REJECT_EMPTY_ADDR, SLOPPY)) != 0) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "%s", err);
	return (-1);
    }
    if (SMTPD_STAND_ALONE(state) == 0
	&& (err = smtpd_check_rcpt(state, argv[1].strval)) != 0) {
	smtpd_chat_reply(state, "%s", err);
	return (-1);
    }

    /*
     * XXX 2821 new feature: Section 3.5.1 requires that the VRFY response is
     * either "full name <user@domain>" or "user@domain". Postfix replies
     * with the address that was provided by the client, whether or not it is
     * in fully qualified domain form or not.
     * 
     * Reply code 250 is reserved for the case where the address is verified;
     * reply code 252 should be used when no definitive certainty exists.
     */
    smtpd_chat_reply(state, "252 %s", argv[1].strval);
    return (0);
}

/* etrn_cmd - process ETRN command */

static int etrn_cmd(SMTPD_STATE *state, int argc, SMTPD_TOKEN *argv)
{
    char   *err;

    /*
     * Sanity checks.
     */
    if (var_helo_required && state->helo_name == 0) {
	state->error_mask |= MAIL_ERROR_POLICY;
	smtpd_chat_reply(state, "503 Error: send HELO/EHLO first");
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
    if (!ISALNUM(argv[1].strval[0]))
	argv[1].strval++;
    if (!valid_hostname(argv[1].strval, DONT_GRIPE)) {
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
    switch (flush_send(argv[1].strval)) {
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

    /*
     * Don't bother checking the syntax.
     */
    smtpd_chat_reply(state, "221 Bye");

    /*
     * When the "." and quit replies are pipelined, make sure they are
     * flushed now, to avoid repeated mail deliveries in case of a crash in
     * the "clean up before disconnect" code.
     */
    vstream_fflush(state->client);
    return (0);
}

/* xclient_cmd - override SMTP client attributes */

static int xclient_cmd(SMTPD_STATE *state, int argc, SMTPD_TOKEN *argv)
{
    SMTPD_TOKEN *argp;
    char   *attr_value;
    char   *attr_name;
    int     update_namaddr = 0;
    int     peer_code;
    static NAME_CODE peer_codes[] = {
	XCLIENT_UNAVAILABLE, SMTPD_PEER_CODE_PERM,
	XCLIENT_TEMPORARY, SMTPD_PEER_CODE_TEMP,
	0, SMTPD_PEER_CODE_OK,
    };
    static NAME_CODE proto_names[] = {
	MAIL_PROTO_SMTP, 1,
	MAIL_PROTO_ESMTP, 2,
	0, -1,
    };

    /*
     * Sanity checks. The XCLIENT command does not override its own access
     * control.
     */
    if (IN_MAIL_TRANSACTION(state)) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "503 Error: MAIL transaction in progress");
	return (-1);
    }
    if (argc < 2) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "501 Syntax: %s attribute=value...",
			 XCLIENT_CMD);
	return (-1);
    }
    if (!xclient_allowed) {
	state->error_mask |= MAIL_ERROR_POLICY;
	smtpd_chat_reply(state, "554 Error: insufficient authorization");
	return (-1);
    }
#define STREQ(x,y)	(strcasecmp((x), (y)) == 0)
#define UPDATE_STR(s, v) do { \
	    if (s) myfree(s); \
	    s = (v) ? mystrdup(v) : 0; \
	} while(0)
#define NEUTER_CHARACTERS "<>()\\\";:@"

    /*
     * Iterate over all attribute=value elements.
     */
    for (argp = argv + 1; argp < argv + argc; argp++) {
	attr_name = argp->strval;

	/*
	 * For safety's sake mask non-printable characters. We'll do more
	 * specific censoring later.
	 */
	if ((attr_value = split_at(attr_name, '=')) == 0 || *attr_value == 0) {
	    state->error_mask |= MAIL_ERROR_PROTOCOL;
	    smtpd_chat_reply(state, "501 Error: attribute=value expected");
	    return (-1);
	}
	printable(attr_value, '?');

	/*
	 * NAME=substitute SMTP client hostname. Also updates the client
	 * hostname lookup status code.
	 */
	if (STREQ(attr_name, XCLIENT_NAME)) {
	    peer_code = name_code(peer_codes, NAME_CODE_FLAG_NONE, attr_value);
	    if (peer_code != SMTPD_PEER_CODE_OK) {
		attr_value = CLIENT_NAME_UNKNOWN;
	    } else {
		if (!valid_hostname(attr_value, DONT_GRIPE)
		    || valid_hostaddr(attr_value, DONT_GRIPE)) {
		    state->error_mask |= MAIL_ERROR_PROTOCOL;
		    smtpd_chat_reply(state, "501 Bad %s syntax: %s",
				     XCLIENT_NAME, attr_value);
		    return (-1);
		}
	    }
	    state->peer_code = peer_code;
	    UPDATE_STR(state->name, attr_value);
	    update_namaddr = 1;
	}

	/*
	 * ADDR=substitute SMTP client network address.
	 */
	else if (STREQ(attr_name, XCLIENT_ADDR)) {
	    if (STREQ(attr_value, XCLIENT_UNAVAILABLE)) {
		attr_value = CLIENT_ADDR_UNKNOWN;
	    } else {
		if (!valid_hostaddr(attr_value, DONT_GRIPE)) {
		    state->error_mask |= MAIL_ERROR_PROTOCOL;
		    smtpd_chat_reply(state, "501 Bad %s syntax: %s",
				     XCLIENT_ADDR, attr_value);
		    return (-1);
		}
	    }
	    UPDATE_STR(state->addr, attr_value);
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
		    smtpd_chat_reply(state, "501 Bad %s syntax: %s",
				     XCLIENT_HELO, attr_value);
		    return (-1);
		}
		neuter(attr_value, NEUTER_CHARACTERS, '?');
	    }
	    UPDATE_STR(state->helo_name, attr_value);
	}

	/*
	 * PROTO=SMTP protocol name.
	 */
	else if (STREQ(attr_name, XCLIENT_PROTO)) {
	    if (name_code(proto_names, NAME_CODE_FLAG_NONE, attr_value) < 0) {
		state->error_mask |= MAIL_ERROR_PROTOCOL;
		smtpd_chat_reply(state, "501 Bad %s syntax: %s",
				 XCLIENT_PROTO, attr_value);
		return (-1);
	    }
	    UPDATE_STR(state->protocol, uppercase(attr_value));
	}

	/*
	 * Unknown attribute name. Complain.
	 */
	else {
	    state->error_mask |= MAIL_ERROR_PROTOCOL;
	    smtpd_chat_reply(state, "501 Bad %s attribute name: %s",
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
	    concatenate(state->name, "[", state->addr, "]", (char *) 0);
    }
    smtpd_chat_reply(state, "250 Ok");
    return (0);
}

/* xforward_cmd - forward logging attributes */

static int xforward_cmd(SMTPD_STATE *state, int argc, SMTPD_TOKEN *argv)
{
    SMTPD_TOKEN *argp;
    char   *attr_value;
    char   *attr_name;
    int     updated = 0;
    static NAME_CODE xforward_flags[] = {
	XFORWARD_NAME, SMTPD_STATE_XFORWARD_NAME,
	XFORWARD_ADDR, SMTPD_STATE_XFORWARD_ADDR,
	XFORWARD_PROTO, SMTPD_STATE_XFORWARD_PROTO,
	XFORWARD_HELO, SMTPD_STATE_XFORWARD_HELO,
	0, 0,
    };
    int     flag;

    /*
     * Sanity checks.
     */
    if (IN_MAIL_TRANSACTION(state)) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "503 Error: MAIL transaction in progress");
	return (-1);
    }
    if (argc < 2) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "501 Syntax: %s attribute=value...",
			 XFORWARD_CMD);
	return (-1);
    }
    if (!xforward_allowed) {
	state->error_mask |= MAIL_ERROR_POLICY;
	smtpd_chat_reply(state, "554 Error: insufficient authorization");
	return (-1);
    }

    /*
     * Initialize.
     */
    if (state->xforward.flags == 0)
	smtpd_xforward_preset(state);

    /*
     * Iterate over all attribute=value elements.
     */
    for (argp = argv + 1; argp < argv + argc; argp++) {
	attr_name = argp->strval;

	/*
	 * For safety's sake mask non-printable characters. We'll do more
	 * specific censoring later.
	 */
	if ((attr_value = split_at(attr_name, '=')) == 0 || *attr_value == 0) {
	    state->error_mask |= MAIL_ERROR_PROTOCOL;
	    smtpd_chat_reply(state, "501 Error: attribute=value expected");
	    return (-1);
	}
	if (strlen(attr_value) > 255) {
	    state->error_mask |= MAIL_ERROR_PROTOCOL;
	    smtpd_chat_reply(state, "501 Error: attribute value too long");
	    return (-1);
	}
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
	    } else {
		neuter(attr_value, NEUTER_CHARACTERS, '?');
	    }
	    UPDATE_STR(state->xforward.addr, attr_value);
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
		    smtpd_chat_reply(state, "501 Bad %s syntax: %s",
				     XFORWARD_PROTO, attr_value);
		    return (-1);
		}
		neuter(attr_value, NEUTER_CHARACTERS, '?');
	    }
	    UPDATE_STR(state->xforward.protocol, attr_value);
	    break;

	    /*
	     * Unknown attribute name. Complain.
	     */
	default:
	    state->error_mask |= MAIL_ERROR_PROTOCOL;
	    smtpd_chat_reply(state, "501 Bad %s attribute name: %s",
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
	    concatenate(state->xforward.name, "[",
			state->xforward.addr, "]",
			(char *) 0) : mystrdup(state->xforward.name);
    }
    smtpd_chat_reply(state, "250 Ok");
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

#define SMTPD_CMD_FLAG_LIMIT    (1<<0)	/* limit usage */
#define SMTPD_CMD_FLAG_FORBID	(1<<1)	/* RFC 2822 mail header */

static SMTPD_CMD smtpd_cmd_table[] = {
    "HELO", helo_cmd, SMTPD_CMD_FLAG_LIMIT,
    "EHLO", ehlo_cmd, SMTPD_CMD_FLAG_LIMIT,

#ifdef USE_SASL_AUTH
    "AUTH", smtpd_sasl_auth_cmd, 0,
#endif

    "MAIL", mail_cmd, 0,
    "RCPT", rcpt_cmd, 0,
    "DATA", data_cmd, 0,
    "RSET", rset_cmd, SMTPD_CMD_FLAG_LIMIT,
    "NOOP", noop_cmd, SMTPD_CMD_FLAG_LIMIT,
    "VRFY", vrfy_cmd, SMTPD_CMD_FLAG_LIMIT,
    "ETRN", etrn_cmd, SMTPD_CMD_FLAG_LIMIT,
    "QUIT", quit_cmd, 0,
    "XCLIENT", xclient_cmd, SMTPD_CMD_FLAG_LIMIT,
    "XFORWARD", xforward_cmd, SMTPD_CMD_FLAG_LIMIT,
    "Received:", 0, SMTPD_CMD_FLAG_FORBID,
    "Reply-To:", 0, SMTPD_CMD_FLAG_FORBID,
    "Message-ID:", 0, SMTPD_CMD_FLAG_FORBID,
    "Subject:", 0, SMTPD_CMD_FLAG_FORBID,
    "From:", 0, SMTPD_CMD_FLAG_FORBID,
    "CONNECT", 0, SMTPD_CMD_FLAG_FORBID,
    "User-Agent:", 0, SMTPD_CMD_FLAG_FORBID,
    0,
};

static STRING_LIST *smtpd_noop_cmds;

/* smtpd_proto - talk the SMTP protocol */

static void smtpd_proto(SMTPD_STATE *state, const char *service)
{
    int     argc;
    SMTPD_TOKEN *argv;
    SMTPD_CMD *cmdp;
    int     count;
    int     crate;

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
    smtp_timeout_setup(state->client, var_smtpd_tmout);

    switch (vstream_setjmp(state->client)) {

    default:
	msg_panic("smtpd_proto: unknown error reading from %s[%s]",
		  state->name, state->addr);
	break;

    case SMTP_ERR_TIME:
	state->reason = "timeout";
	smtpd_chat_reply(state, "421 %s Error: timeout exceeded",
			 var_myhostname);
	break;

    case SMTP_ERR_EOF:
	state->reason = "lost connection";
	break;

    case 0:

	/*
	 * XXX The client connection count/rate control must be consistent in
	 * its use of client address information in connect and disconnect
	 * events. For now we exclude xclient authorized hosts from
	 * connection count/rate control.
	 */
#ifdef SNAPSHOT
	if (SMTPD_STAND_ALONE(state) == 0
	    && !xclient_allowed
	    && anvil_clnt
	    && !namadr_list_match(hogger_list, state->name, state->addr)
	    && anvil_clnt_connect(anvil_clnt, service, state->addr,
				  &count, &crate) == ANVIL_STAT_OK) {
	    if (var_smtpd_cconn_limit > 0 && count > var_smtpd_cconn_limit) {
		smtpd_chat_reply(state, "421 %s Error: too many connections from %s",
				 var_myhostname, state->addr);
		msg_warn("Too many connections: %d from %s for service %s",
			 count, state->namaddr, service);
		break;
	    }
	    if (var_smtpd_crate_limit > 0 && crate > var_smtpd_crate_limit) {
		smtpd_chat_reply(state, "421 %s Error: too many connections from %s",
				 var_myhostname, state->addr);
		msg_warn("Too frequent connections: %d from %s for service %s",
			 crate, state->namaddr, service);
		break;
	    }
	}
#endif
	/* XXX We use the real client for connect access control. */
	if (SMTPD_STAND_ALONE(state) == 0
	    && var_smtpd_delay_reject == 0
	    && (state->access_denied = smtpd_check_client(state)) != 0) {
	    smtpd_chat_reply(state, "%s", state->access_denied);
	} else {
	    smtpd_chat_reply(state, "220 %s", var_smtpd_banner);
	}

	for (;;) {
	    if (state->error_count >= var_smtpd_hard_erlim) {
		state->reason = "too many errors";
		state->error_mask |= MAIL_ERROR_PROTOCOL;
		smtpd_chat_reply(state, "421 %s Error: too many errors",
				 var_myhostname);
		break;
	    }
	    watchdog_pat();
	    smtpd_chat_query(state);
	    if ((argc = smtpd_token(vstring_str(state->buffer), &argv)) == 0) {
		state->error_mask |= MAIL_ERROR_PROTOCOL;
		smtpd_chat_reply(state, "500 Error: bad syntax");
		state->error_count++;
		continue;
	    }
	    if (*var_smtpd_noop_cmds
		&& string_list_match(smtpd_noop_cmds, argv[0].strval)) {
		smtpd_chat_reply(state, "250 Ok");
		if (state->junk_cmds++ > var_smtpd_junk_cmd_limit)
		    state->error_count++;
		continue;
	    }
	    for (cmdp = smtpd_cmd_table; cmdp->name != 0; cmdp++)
		if (strcasecmp(argv[0].strval, cmdp->name) == 0)
		    break;
	    if (cmdp->name == 0) {
		smtpd_chat_reply(state, "502 Error: command not implemented");
		state->error_mask |= MAIL_ERROR_PROTOCOL;
		state->error_count++;
		continue;
	    }
	    if (cmdp->flags & SMTPD_CMD_FLAG_FORBID) {
		msg_warn("%s sent non-SMTP command: %.100s",
			 state->namaddr, vstring_str(state->buffer));
		smtpd_chat_reply(state, "221 Error: I can break rules, too. Goodbye.");
		break;
	    }
	    /* XXX We use the real client for connect access control. */
	    if (state->access_denied && cmdp->action != quit_cmd) {
		smtpd_chat_reply(state, "503 Error: access denied for %s",
				 state->namaddr);	/* RFC 2821 Sec 3.1 */
		state->error_count++;
		continue;
	    }
	    state->where = cmdp->name;
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
     */
#ifdef SNAPSHOT
    if (SMTPD_STAND_ALONE(state) == 0
	&& !xclient_allowed
	&& anvil_clnt
	&& !namadr_list_match(hogger_list, state->name, state->addr))
	anvil_clnt_disconnect(anvil_clnt, service, state->addr);
#endif

    /*
     * Log abnormal session termination, in case postmaster notification has
     * been turned off. In the log, indicate the last recognized state before
     * things went wrong. Don't complain about clients that go away without
     * sending QUIT.
     */
    if (state->reason && state->where
	&& (strcmp(state->where, SMTPD_AFTER_DOT)
	    || strcmp(state->reason, "lost connection")))
	msg_info("%s after %s from %s[%s]",
		 state->reason, state->where, state->name, state->addr);

    /*
     * Cleanup whatever information the client gave us during the SMTP
     * dialog.
     */
    helo_reset(state);
#ifdef USE_SASL_AUTH
    if (var_smtpd_sasl_enable)
	smtpd_sasl_auth_reset(state);
#endif
    chat_reset(state, 0);
    mail_reset(state);
    rcpt_reset(state);
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
     * This routine runs when a client has connected to our network port, or
     * when the smtp server is run in stand-alone mode (input from pipe).
     * 
     * Look up and sanitize the peer name, then initialize some connection-
     * specific state. When the name service is hosed, hostname lookup will
     * take a while. This is why I always run a local name server on critical
     * machines.
     */
    smtpd_state_init(&state, stream);
    msg_info("connect from %s[%s]", state.name, state.addr);

    /*
     * XCLIENT must not override its own access control.
     */
    xclient_allowed =
	namadr_list_match(xclient_hosts, state.name, state.addr);

    /*
     * Overriding XFORWARD access control makes no sense, either.
     */
    xforward_allowed =
	namadr_list_match(xforward_hosts, state.name, state.addr);

    /*
     * See if we need to turn on verbose logging for this client.
     */
    debug_peer_check(state.name, state.addr);

    /*
     * Provide the SMTP service.
     */
    smtpd_proto(&state, service);

    /*
     * After the client has gone away, clean up whatever we have set up at
     * connection time.
     */
    msg_info("disconnect from %s[%s]", state.name, state.addr);
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
    smtpd_noop_cmds = string_list_init(MATCH_FLAG_NONE, var_smtpd_noop_cmds);
    verp_clients = namadr_list_init(MATCH_FLAG_NONE, var_verp_clients);
    xclient_hosts = namadr_list_init(MATCH_FLAG_NONE, var_xclient_hosts);
    xforward_hosts = namadr_list_init(MATCH_FLAG_NONE, var_xforward_hosts);
#ifdef SNAPSHOT
    hogger_list = namadr_list_init(MATCH_FLAG_NONE, var_smtpd_hoggers);
#endif
    if (getuid() == 0 || getuid() == var_owner_uid)
	smtpd_check_init();
    debug_peer_init();

    if (var_smtpd_sasl_enable)
#ifdef USE_SASL_AUTH
	smtpd_sasl_initialize();

    if (*var_smtpd_sasl_exceptions_networks)
	sasl_exceptions_networks =
	    namadr_list_init(MATCH_FLAG_NONE,
			     var_smtpd_sasl_exceptions_networks);
#else
	msg_warn("%s is true, but SASL support is not compiled in",
		 VAR_SMTPD_SASL_ENABLE);
#endif

    /*
     * flush client.
     */
    flush_init();
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
     * Sanity checks. The queue_minfree value should be at least as large as
     * (process_limit * message_size_limit) but that is unpractical, so we
     * arbitrarily pick a number and require twice the message size limit.
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
#ifdef SNAPSHOT
    if (var_smtpd_crate_limit || var_smtpd_cconn_limit)
	anvil_clnt = anvil_clnt_create();
#endif
}

/* main - the main program */

int     main(int argc, char **argv)
{
    static CONFIG_INT_TABLE int_table[] = {
	VAR_SMTPD_RCPT_LIMIT, DEF_SMTPD_RCPT_LIMIT, &var_smtpd_rcpt_limit, 1, 0,
	VAR_SMTPD_SOFT_ERLIM, DEF_SMTPD_SOFT_ERLIM, &var_smtpd_soft_erlim, 1, 0,
	VAR_SMTPD_HARD_ERLIM, DEF_SMTPD_HARD_ERLIM, &var_smtpd_hard_erlim, 1, 0,
	VAR_QUEUE_MINFREE, DEF_QUEUE_MINFREE, &var_queue_minfree, 0, 0,
	VAR_UNK_CLIENT_CODE, DEF_UNK_CLIENT_CODE, &var_unk_client_code, 0, 0,
	VAR_BAD_NAME_CODE, DEF_BAD_NAME_CODE, &var_bad_name_code, 0, 0,
	VAR_UNK_NAME_CODE, DEF_UNK_NAME_CODE, &var_unk_name_code, 0, 0,
	VAR_UNK_ADDR_CODE, DEF_UNK_ADDR_CODE, &var_unk_addr_code, 0, 0,
	VAR_RELAY_CODE, DEF_RELAY_CODE, &var_relay_code, 0, 0,
	VAR_MAPS_RBL_CODE, DEF_MAPS_RBL_CODE, &var_maps_rbl_code, 0, 0,
	VAR_ACCESS_MAP_CODE, DEF_ACCESS_MAP_CODE, &var_access_map_code, 0, 0,
	VAR_REJECT_CODE, DEF_REJECT_CODE, &var_reject_code, 0, 0,
	VAR_DEFER_CODE, DEF_DEFER_CODE, &var_defer_code, 0, 0,
	VAR_NON_FQDN_CODE, DEF_NON_FQDN_CODE, &var_non_fqdn_code, 0, 0,
	VAR_SMTPD_JUNK_CMD, DEF_SMTPD_JUNK_CMD, &var_smtpd_junk_cmd_limit, 1, 0,
	VAR_SMTPD_RCPT_OVERLIM, DEF_SMTPD_RCPT_OVERLIM, &var_smtpd_rcpt_overlim, 1, 0,
	VAR_SMTPD_HIST_THRSH, DEF_SMTPD_HIST_THRSH, &var_smtpd_hist_thrsh, 1, 0,
	VAR_UNV_FROM_CODE, DEF_UNV_FROM_CODE, &var_unv_from_code, 0, 0,
	VAR_UNV_RCPT_CODE, DEF_UNV_RCPT_CODE, &var_unv_rcpt_code, 0, 0,
	VAR_MUL_RCPT_CODE, DEF_MUL_RCPT_CODE, &var_mul_rcpt_code, 0, 0,
	VAR_LOCAL_RCPT_CODE, DEF_LOCAL_RCPT_CODE, &var_local_rcpt_code, 0, 0,
	VAR_VIRT_ALIAS_CODE, DEF_VIRT_ALIAS_CODE, &var_virt_alias_code, 0, 0,
	VAR_VIRT_MAILBOX_CODE, DEF_VIRT_MAILBOX_CODE, &var_virt_mailbox_code, 0, 0,
	VAR_RELAY_RCPT_CODE, DEF_RELAY_RCPT_CODE, &var_relay_rcpt_code, 0, 0,
	VAR_VERIFY_POLL_COUNT, DEF_VERIFY_POLL_COUNT, &var_verify_poll_count, 1, 0,
#ifdef SNAPSHOT
	VAR_SMTPD_CRATE_LIMIT, DEF_SMTPD_CRATE_LIMIT, &var_smtpd_crate_limit, 0, 0,
	VAR_SMTPD_CCONN_LIMIT, DEF_SMTPD_CCONN_LIMIT, &var_smtpd_cconn_limit, 0, 0,
#endif
	0,
    };
    static CONFIG_TIME_TABLE time_table[] = {
	VAR_SMTPD_TMOUT, DEF_SMTPD_TMOUT, &var_smtpd_tmout, 1, 0,
	VAR_SMTPD_ERR_SLEEP, DEF_SMTPD_ERR_SLEEP, &var_smtpd_err_sleep, 0, 0,
	VAR_SMTPD_PROXY_TMOUT, DEF_SMTPD_PROXY_TMOUT, &var_smtpd_proxy_tmout, 1, 0,
	VAR_VERIFY_POLL_DELAY, DEF_VERIFY_POLL_DELAY, &var_verify_poll_delay, 1, 0,
	VAR_SMTPD_POLICY_TMOUT, DEF_SMTPD_POLICY_TMOUT, &var_smtpd_policy_tmout, 1, 0,
	VAR_SMTPD_POLICY_IDLE, DEF_SMTPD_POLICY_IDLE, &var_smtpd_policy_idle, 1, 0,
	VAR_SMTPD_POLICY_TTL, DEF_SMTPD_POLICY_TTL, &var_smtpd_policy_ttl, 1, 0,
	0,
    };
    static CONFIG_BOOL_TABLE bool_table[] = {
	VAR_HELO_REQUIRED, DEF_HELO_REQUIRED, &var_helo_required,
	VAR_SMTPD_DELAY_REJECT, DEF_SMTPD_DELAY_REJECT, &var_smtpd_delay_reject,
	VAR_STRICT_RFC821_ENV, DEF_STRICT_RFC821_ENV, &var_strict_rfc821_env,
	VAR_DISABLE_VRFY_CMD, DEF_DISABLE_VRFY_CMD, &var_disable_vrfy_cmd,
	VAR_ALLOW_UNTRUST_ROUTE, DEF_ALLOW_UNTRUST_ROUTE, &var_allow_untrust_route,
	VAR_SMTPD_SASL_ENABLE, DEF_SMTPD_SASL_ENABLE, &var_smtpd_sasl_enable,
	VAR_BROKEN_AUTH_CLNTS, DEF_BROKEN_AUTH_CLNTS, &var_broken_auth_clients,
	VAR_SHOW_UNK_RCPT_TABLE, DEF_SHOW_UNK_RCPT_TABLE, &var_show_unk_rcpt_table,
	VAR_SMTPD_REJ_UNL_FROM, DEF_SMTPD_REJ_UNL_FROM, &var_smtpd_rej_unl_from,
	VAR_SMTPD_REJ_UNL_RCPT, DEF_SMTPD_REJ_UNL_RCPT, &var_smtpd_rej_unl_rcpt,
	0,
    };
    static CONFIG_STR_TABLE str_table[] = {
	VAR_SMTPD_BANNER, DEF_SMTPD_BANNER, &var_smtpd_banner, 1, 0,
	VAR_NOTIFY_CLASSES, DEF_NOTIFY_CLASSES, &var_notify_classes, 0, 0,
	VAR_CLIENT_CHECKS, DEF_CLIENT_CHECKS, &var_client_checks, 0, 0,
	VAR_HELO_CHECKS, DEF_HELO_CHECKS, &var_helo_checks, 0, 0,
	VAR_MAIL_CHECKS, DEF_MAIL_CHECKS, &var_mail_checks, 0, 0,
	VAR_RCPT_CHECKS, DEF_RCPT_CHECKS, &var_rcpt_checks, 0, 0,
	VAR_ETRN_CHECKS, DEF_ETRN_CHECKS, &var_etrn_checks, 0, 0,
	VAR_DATA_CHECKS, DEF_DATA_CHECKS, &var_data_checks, 0, 0,
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
	VAR_SMTPD_SASL_APPNAME, DEF_SMTPD_SASL_APPNAME, &var_smtpd_sasl_appname, 1, 0,
	VAR_SMTPD_SASL_REALM, DEF_SMTPD_SASL_REALM, &var_smtpd_sasl_realm, 0, 0,
	VAR_SMTPD_SASL_EXCEPTIONS_NETWORKS, DEF_SMTPD_SASL_EXCEPTIONS_NETWORKS, &var_smtpd_sasl_exceptions_networks, 0, 0,
	VAR_FILTER_XPORT, DEF_FILTER_XPORT, &var_filter_xport, 0, 0,
	VAR_PERM_MX_NETWORKS, DEF_PERM_MX_NETWORKS, &var_perm_mx_networks, 0, 0,
	VAR_SMTPD_SND_AUTH_MAPS, DEF_SMTPD_SND_AUTH_MAPS, &var_smtpd_snd_auth_maps, 0, 0,
	VAR_SMTPD_NOOP_CMDS, DEF_SMTPD_NOOP_CMDS, &var_smtpd_noop_cmds, 0, 0,
	VAR_SMTPD_NULL_KEY, DEF_SMTPD_NULL_KEY, &var_smtpd_null_key, 0, 0,
	VAR_RELAY_RCPT_MAPS, DEF_RELAY_RCPT_MAPS, &var_relay_rcpt_maps, 0, 0,
	VAR_VERIFY_SENDER, DEF_VERIFY_SENDER, &var_verify_sender, 0, 0,
	VAR_VERP_CLIENTS, DEF_VERP_CLIENTS, &var_verp_clients, 0, 0,
	VAR_SMTPD_PROXY_FILT, DEF_SMTPD_PROXY_FILT, &var_smtpd_proxy_filt, 0, 0,
	VAR_SMTPD_PROXY_EHLO, DEF_SMTPD_PROXY_EHLO, &var_smtpd_proxy_ehlo, 0, 0,
	VAR_INPUT_TRANSP, DEF_INPUT_TRANSP, &var_input_transp, 0, 0,
	VAR_XCLIENT_HOSTS, DEF_XCLIENT_HOSTS, &var_xclient_hosts, 0, 0,
	VAR_XFORWARD_HOSTS, DEF_XFORWARD_HOSTS, &var_xforward_hosts, 0, 0,
#ifdef SNAPSHOT
	VAR_SMTPD_HOGGERS, DEF_SMTPD_HOGGERS, &var_smtpd_hoggers, 0, 0,
#endif
	0,
    };
    static CONFIG_RAW_TABLE raw_table[] = {
	VAR_SMTPD_EXP_FILTER, DEF_SMTPD_EXP_FILTER, &var_smtpd_exp_filter, 1, 0,
	VAR_DEF_RBL_REPLY, DEF_DEF_RBL_REPLY, &var_def_rbl_reply, 1, 0,
	0,
    };

    /*
     * Pass control to the single-threaded service skeleton.
     */
    single_server_main(argc, argv, smtpd_service,
		       MAIL_SERVER_INT_TABLE, int_table,
		       MAIL_SERVER_STR_TABLE, str_table,
		       MAIL_SERVER_RAW_TABLE, raw_table,
		       MAIL_SERVER_BOOL_TABLE, bool_table,
		       MAIL_SERVER_TIME_TABLE, time_table,
		       MAIL_SERVER_PRE_INIT, pre_jail_init,
		       MAIL_SERVER_PRE_ACCEPT, pre_accept,
		       MAIL_SERVER_POST_INIT, post_jail_init,
		       0);
}
