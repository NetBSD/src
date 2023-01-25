.. Copyright (C) Internet Systems Consortium, Inc. ("ISC")
..
.. SPDX-License-Identifier: MPL-2.0
..
.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0.  If a copy of the MPL was not distributed with this
.. file, you can obtain one at https://mozilla.org/MPL/2.0/.
..
.. See the COPYRIGHT file distributed with this work for additional
.. information regarding copyright ownership.

::

  options {
  	allow-new-zones <boolean>;
  	allow-notify { <address_match_element>; ... };
  	allow-query { <address_match_element>; ... };
  	allow-query-cache { <address_match_element>; ... };
  	allow-query-cache-on { <address_match_element>; ... };
  	allow-query-on { <address_match_element>; ... };
  	allow-recursion { <address_match_element>; ... };
  	allow-recursion-on { <address_match_element>; ... };
  	allow-transfer { <address_match_element>; ... };
  	allow-update { <address_match_element>; ... };
  	allow-update-forwarding { <address_match_element>; ... };
  	also-notify [ port <integer> ] [ dscp <integer> ] { (
  	    <remote-servers> | <ipv4_address> [ port <integer> ] |
  	    <ipv6_address> [ port <integer> ] ) [ key <string> ]; ... };
  	alt-transfer-source ( <ipv4_address> | * ) [ port ( <integer> | * )
  	    ] [ dscp <integer> ];
  	alt-transfer-source-v6 ( <ipv6_address> | * ) [ port ( <integer> |
  	    * ) ] [ dscp <integer> ];
  	answer-cookie <boolean>;
  	attach-cache <string>;
  	auth-nxdomain <boolean>; // default changed
  	auto-dnssec ( allow | maintain | off ); // deprecated
  	automatic-interface-scan <boolean>;
  	avoid-v4-udp-ports { <portrange>; ... };
  	avoid-v6-udp-ports { <portrange>; ... };
  	bindkeys-file <quoted_string>;
  	blackhole { <address_match_element>; ... };
  	cache-file <quoted_string>; // deprecated
  	catalog-zones { zone <string> [ default-masters [ port <integer> ]
  	    [ dscp <integer> ] { ( <remote-servers> | <ipv4_address> [ port
  	    <integer> ] | <ipv6_address> [ port <integer> ] ) [ key
  	    <string> ]; ... } ] [ zone-directory <quoted_string> ] [
  	    in-memory <boolean> ] [ min-update-interval <duration> ]; ... };
  	check-dup-records ( fail | warn | ignore );
  	check-integrity <boolean>;
  	check-mx ( fail | warn | ignore );
  	check-mx-cname ( fail | warn | ignore );
  	check-names ( primary | master |
  	    secondary | slave | response ) (
  	    fail | warn | ignore );
  	check-sibling <boolean>;
  	check-spf ( warn | ignore );
  	check-srv-cname ( fail | warn | ignore );
  	check-wildcard <boolean>;
  	clients-per-query <integer>;
  	cookie-algorithm ( aes | siphash24 );
  	cookie-secret <string>;
  	coresize ( default | unlimited | <sizeval> );
  	datasize ( default | unlimited | <sizeval> );
  	deny-answer-addresses { <address_match_element>; ... } [
  	    except-from { <string>; ... } ];
  	deny-answer-aliases { <string>; ... } [ except-from { <string>; ...
  	    } ];
  	dialup ( notify | notify-passive | passive | refresh | <boolean> );
  	directory <quoted_string>;
  	disable-algorithms <string> { <string>;
  	    ... };
  	disable-ds-digests <string> { <string>;
  	    ... };
  	disable-empty-zone <string>;
  	dns64 <netprefix> {
  		break-dnssec <boolean>;
  		clients { <address_match_element>; ... };
  		exclude { <address_match_element>; ... };
  		mapped { <address_match_element>; ... };
  		recursive-only <boolean>;
  		suffix <ipv6_address>;
  	};
  	dns64-contact <string>;
  	dns64-server <string>;
  	dnskey-sig-validity <integer>;
  	dnsrps-enable <boolean>;
  	dnsrps-options { <unspecified-text> };
  	dnssec-accept-expired <boolean>;
  	dnssec-dnskey-kskonly <boolean>;
  	dnssec-loadkeys-interval <integer>;
  	dnssec-must-be-secure <string> <boolean>;
  	dnssec-policy <string>;
  	dnssec-secure-to-insecure <boolean>;
  	dnssec-update-mode ( maintain | no-resign );
  	dnssec-validation ( yes | no | auto );
  	dnstap { ( all | auth | client | forwarder | resolver | update ) [
  	    ( query | response ) ]; ... };
  	dnstap-identity ( <quoted_string> | none | hostname );
  	dnstap-output ( file | unix ) <quoted_string> [ size ( unlimited |
  	    <size> ) ] [ versions ( unlimited | <integer> ) ] [ suffix (
  	    increment | timestamp ) ];
  	dnstap-version ( <quoted_string> | none );
  	dscp <integer>;
  	dual-stack-servers [ port <integer> ] { ( <quoted_string> [ port
  	    <integer> ] [ dscp <integer> ] | <ipv4_address> [ port
  	    <integer> ] [ dscp <integer> ] | <ipv6_address> [ port
  	    <integer> ] [ dscp <integer> ] ); ... };
  	dump-file <quoted_string>;
  	edns-udp-size <integer>;
  	empty-contact <string>;
  	empty-server <string>;
  	empty-zones-enable <boolean>;
  	fetch-quota-params <integer> <fixedpoint> <fixedpoint> <fixedpoint>;
  	fetches-per-server <integer> [ ( drop | fail ) ];
  	fetches-per-zone <integer> [ ( drop | fail ) ];
  	files ( default | unlimited | <sizeval> );
  	flush-zones-on-shutdown <boolean>;
  	forward ( first | only );
  	forwarders [ port <integer> ] [ dscp <integer> ] { ( <ipv4_address>
  	    | <ipv6_address> ) [ port <integer> ] [ dscp <integer> ]; ... };
  	fstrm-set-buffer-hint <integer>;
  	fstrm-set-flush-timeout <integer>;
  	fstrm-set-input-queue-size <integer>;
  	fstrm-set-output-notify-threshold <integer>;
  	fstrm-set-output-queue-model ( mpsc | spsc );
  	fstrm-set-output-queue-size <integer>;
  	fstrm-set-reopen-interval <duration>;
  	geoip-directory ( <quoted_string> | none );
  	glue-cache <boolean>;
  	heartbeat-interval <integer>;
  	hostname ( <quoted_string> | none );
  	interface-interval <duration>;
  	ixfr-from-differences ( primary | master | secondary | slave |
  	    <boolean> );
  	keep-response-order { <address_match_element>; ... };
  	key-directory <quoted_string>;
  	lame-ttl <duration>;
  	listen-on [ port <integer> ] [ dscp
  	    <integer> ] {
  	    <address_match_element>; ... };
  	listen-on-v6 [ port <integer> ] [ dscp
  	    <integer> ] {
  	    <address_match_element>; ... };
  	lmdb-mapsize <sizeval>;
  	lock-file ( <quoted_string> | none );
  	managed-keys-directory <quoted_string>;
  	masterfile-format ( map | raw | text );
  	masterfile-style ( full | relative );
  	match-mapped-addresses <boolean>;
  	max-cache-size ( default | unlimited | <sizeval> | <percentage> );
  	max-cache-ttl <duration>;
  	max-clients-per-query <integer>;
  	max-ixfr-ratio ( unlimited | <percentage> );
  	max-journal-size ( default | unlimited | <sizeval> );
  	max-ncache-ttl <duration>;
  	max-records <integer>;
  	max-recursion-depth <integer>;
  	max-recursion-queries <integer>;
  	max-refresh-time <integer>;
  	max-retry-time <integer>;
  	max-rsa-exponent-size <integer>;
  	max-stale-ttl <duration>;
  	max-transfer-idle-in <integer>;
  	max-transfer-idle-out <integer>;
  	max-transfer-time-in <integer>;
  	max-transfer-time-out <integer>;
  	max-udp-size <integer>;
  	max-zone-ttl ( unlimited | <duration> );
  	memstatistics <boolean>;
  	memstatistics-file <quoted_string>;
  	message-compression <boolean>;
  	min-cache-ttl <duration>;
  	min-ncache-ttl <duration>;
  	min-refresh-time <integer>;
  	min-retry-time <integer>;
  	minimal-any <boolean>;
  	minimal-responses ( no-auth | no-auth-recursive | <boolean> );
  	multi-master <boolean>;
  	new-zones-directory <quoted_string>;
  	no-case-compress { <address_match_element>; ... };
  	nocookie-udp-size <integer>;
  	notify ( explicit | master-only | primary-only | <boolean> );
  	notify-delay <integer>;
  	notify-rate <integer>;
  	notify-source ( <ipv4_address> | * ) [ port ( <integer> | * ) ] [
  	    dscp <integer> ];
  	notify-source-v6 ( <ipv6_address> | * ) [ port ( <integer> | * ) ]
  	    [ dscp <integer> ];
  	notify-to-soa <boolean>;
  	nta-lifetime <duration>;
  	nta-recheck <duration>;
  	nxdomain-redirect <string>;
  	parental-source ( <ipv4_address> | * ) [ port ( <integer> | * ) ] [
  	    dscp <integer> ];
  	parental-source-v6 ( <ipv6_address> | * ) [ port ( <integer> | * )
  	    ] [ dscp <integer> ];
  	pid-file ( <quoted_string> | none );
  	port <integer>;
  	preferred-glue <string>;
  	prefetch <integer> [ <integer> ];
  	provide-ixfr <boolean>;
  	qname-minimization ( strict | relaxed | disabled | off );
  	query-source ( ( [ address ] ( <ipv4_address> | * ) [ port (
  	    <integer> | * ) ] ) | ( [ [ address ] ( <ipv4_address> | * ) ]
  	    port ( <integer> | * ) ) ) [ dscp <integer> ];
  	query-source-v6 ( ( [ address ] ( <ipv6_address> | * ) [ port (
  	    <integer> | * ) ] ) | ( [ [ address ] ( <ipv6_address> | * ) ]
  	    port ( <integer> | * ) ) ) [ dscp <integer> ];
  	querylog <boolean>;
  	random-device ( <quoted_string> | none );
  	rate-limit {
  		all-per-second <integer>;
  		errors-per-second <integer>;
  		exempt-clients { <address_match_element>; ... };
  		ipv4-prefix-length <integer>;
  		ipv6-prefix-length <integer>;
  		log-only <boolean>;
  		max-table-size <integer>;
  		min-table-size <integer>;
  		nodata-per-second <integer>;
  		nxdomains-per-second <integer>;
  		qps-scale <integer>;
  		referrals-per-second <integer>;
  		responses-per-second <integer>;
  		slip <integer>;
  		window <integer>;
  	};
  	recursing-file <quoted_string>;
  	recursion <boolean>;
  	recursive-clients <integer>;
  	request-expire <boolean>;
  	request-ixfr <boolean>;
  	request-nsid <boolean>;
  	require-server-cookie <boolean>;
  	reserved-sockets <integer>;
  	resolver-nonbackoff-tries <integer>;
  	resolver-query-timeout <integer>;
  	resolver-retry-interval <integer>;
  	response-padding { <address_match_element>; ... } block-size
  	    <integer>;
  	response-policy { zone <string> [ add-soa <boolean> ] [ log
  	    <boolean> ] [ max-policy-ttl <duration> ] [ min-update-interval
  	    <duration> ] [ policy ( cname | disabled | drop | given | no-op
  	    | nodata | nxdomain | passthru | tcp-only <quoted_string> ) ] [
  	    recursive-only <boolean> ] [ nsip-enable <boolean> ] [
  	    nsdname-enable <boolean> ]; ... } [ add-soa <boolean> ] [
  	    break-dnssec <boolean> ] [ max-policy-ttl <duration> ] [
  	    min-update-interval <duration> ] [ min-ns-dots <integer> ] [
  	    nsip-wait-recurse <boolean> ] [ qname-wait-recurse <boolean> ]
  	    [ recursive-only <boolean> ] [ nsip-enable <boolean> ] [
  	    nsdname-enable <boolean> ] [ dnsrps-enable <boolean> ] [
  	    dnsrps-options { <unspecified-text> } ];
  	reuseport <boolean>;
  	root-delegation-only [ exclude { <string>; ... } ];
  	root-key-sentinel <boolean>;
  	rrset-order { [ class <string> ] [ type <string> ] [ name
  	    <quoted_string> ] <string> <string>; ... };
  	secroots-file <quoted_string>;
  	send-cookie <boolean>;
  	serial-query-rate <integer>;
  	serial-update-method ( date | increment | unixtime );
  	server-id ( <quoted_string> | none | hostname );
  	servfail-ttl <duration>;
  	session-keyalg <string>;
  	session-keyfile ( <quoted_string> | none );
  	session-keyname <string>;
  	sig-signing-nodes <integer>;
  	sig-signing-signatures <integer>;
  	sig-signing-type <integer>;
  	sig-validity-interval <integer> [ <integer> ];
  	sortlist { <address_match_element>; ... };
  	stacksize ( default | unlimited | <sizeval> );
  	stale-answer-client-timeout ( disabled | off | <integer> );
  	stale-answer-enable <boolean>;
  	stale-answer-ttl <duration>;
  	stale-cache-enable <boolean>;
  	stale-refresh-time <duration>;
  	startup-notify-rate <integer>;
  	statistics-file <quoted_string>;
  	synth-from-dnssec <boolean>;
  	tcp-advertised-timeout <integer>;
  	tcp-clients <integer>;
  	tcp-idle-timeout <integer>;
  	tcp-initial-timeout <integer>;
  	tcp-keepalive-timeout <integer>;
  	tcp-listen-queue <integer>;
  	tkey-dhkey <quoted_string> <integer>;
  	tkey-domain <quoted_string>;
  	tkey-gssapi-credential <quoted_string>;
  	tkey-gssapi-keytab <quoted_string>;
  	transfer-format ( many-answers | one-answer );
  	transfer-message-size <integer>;
  	transfer-source ( <ipv4_address> | * ) [ port ( <integer> | * ) ] [
  	    dscp <integer> ];
  	transfer-source-v6 ( <ipv6_address> | * ) [ port ( <integer> | * )
  	    ] [ dscp <integer> ];
  	transfers-in <integer>;
  	transfers-out <integer>;
  	transfers-per-ns <integer>;
  	trust-anchor-telemetry <boolean>; // experimental
  	try-tcp-refresh <boolean>;
  	update-check-ksk <boolean>;
  	update-quota <integer>;
  	use-alt-transfer-source <boolean>;
  	use-v4-udp-ports { <portrange>; ... };
  	use-v6-udp-ports { <portrange>; ... };
  	v6-bias <integer>;
  	validate-except { <string>; ... };
  	version ( <quoted_string> | none );
  	zero-no-soa-ttl <boolean>;
  	zero-no-soa-ttl-cache <boolean>;
  	zone-statistics ( full | terse | none | <boolean> );
  };
