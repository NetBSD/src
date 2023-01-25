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

  zone <string> [ <class> ] {
  	type ( slave | secondary );
  	allow-notify { <address_match_element>; ... };
  	allow-query { <address_match_element>; ... };
  	allow-query-on { <address_match_element>; ... };
  	allow-transfer { <address_match_element>; ... };
  	allow-update-forwarding { <address_match_element>; ... };
  	also-notify [ port <integer> ] [ dscp <integer> ] { ( <remote-servers> | <ipv4_address> [ port <integer> ] | <ipv6_address> [ port <integer> ] ) [ key <string> ]; ... };
  	alt-transfer-source ( <ipv4_address> | * ) [ port ( <integer> | * ) ] [ dscp <integer> ];
  	alt-transfer-source-v6 ( <ipv6_address> | * ) [ port ( <integer> | * ) ] [ dscp <integer> ];
  	auto-dnssec ( allow | maintain | off ); // deprecated
  	check-names ( fail | warn | ignore );
  	database <string>;
  	dialup ( notify | notify-passive | passive | refresh | <boolean> );
  	dlz <string>;
  	dnskey-sig-validity <integer>;
  	dnssec-dnskey-kskonly <boolean>;
  	dnssec-loadkeys-interval <integer>;
  	dnssec-policy <string>;
  	dnssec-update-mode ( maintain | no-resign );
  	file <quoted_string>;
  	forward ( first | only );
  	forwarders [ port <integer> ] [ dscp <integer> ] { ( <ipv4_address> | <ipv6_address> ) [ port <integer> ] [ dscp <integer> ]; ... };
  	inline-signing <boolean>;
  	ixfr-from-differences <boolean>;
  	journal <quoted_string>;
  	key-directory <quoted_string>;
  	masterfile-format ( map | raw | text );
  	masterfile-style ( full | relative );
  	masters [ port <integer> ] [ dscp <integer> ] { ( <remote-servers> | <ipv4_address> [ port <integer> ] | <ipv6_address> [ port <integer> ] ) [ key <string> ]; ... };
  	max-ixfr-ratio ( unlimited | <percentage> );
  	max-journal-size ( default | unlimited | <sizeval> );
  	max-records <integer>;
  	max-refresh-time <integer>;
  	max-retry-time <integer>;
  	max-transfer-idle-in <integer>;
  	max-transfer-idle-out <integer>;
  	max-transfer-time-in <integer>;
  	max-transfer-time-out <integer>;
  	min-refresh-time <integer>;
  	min-retry-time <integer>;
  	multi-master <boolean>;
  	notify ( explicit | master-only | primary-only | <boolean> );
  	notify-delay <integer>;
  	notify-source ( <ipv4_address> | * ) [ port ( <integer> | * ) ] [ dscp <integer> ];
  	notify-source-v6 ( <ipv6_address> | * ) [ port ( <integer> | * ) ] [ dscp <integer> ];
  	notify-to-soa <boolean>;
  	parental-agents [ port <integer> ] [ dscp <integer> ] { ( <remote-servers> | <ipv4_address> [ port <integer> ] | <ipv6_address> [ port <integer> ] ) [ key <string> ]; ... };
  	parental-source ( <ipv4_address> | * ) [ port ( <integer> | * ) ] [ dscp <integer> ];
  	parental-source-v6 ( <ipv6_address> | * ) [ port ( <integer> | * ) ] [ dscp <integer> ];
  	primaries [ port <integer> ] [ dscp <integer> ] { ( <remote-servers> | <ipv4_address> [ port <integer> ] | <ipv6_address> [ port <integer> ] ) [ key <string> ]; ... };
  	request-expire <boolean>;
  	request-ixfr <boolean>;
  	sig-signing-nodes <integer>;
  	sig-signing-signatures <integer>;
  	sig-signing-type <integer>;
  	sig-validity-interval <integer> [ <integer> ];
  	transfer-source ( <ipv4_address> | * ) [ port ( <integer> | * ) ] [ dscp <integer> ];
  	transfer-source-v6 ( <ipv6_address> | * ) [ port ( <integer> | * ) ] [ dscp <integer> ];
  	try-tcp-refresh <boolean>;
  	update-check-ksk <boolean>;
  	use-alt-transfer-source <boolean>;
  	zero-no-soa-ttl <boolean>;
  	zone-statistics ( full | terse | none | <boolean> );
  };
