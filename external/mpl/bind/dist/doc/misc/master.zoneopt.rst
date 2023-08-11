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
  	type ( master | primary );
  	allow-query { <address_match_element>; ... };
  	allow-query-on { <address_match_element>; ... };
  	allow-transfer { <address_match_element>; ... };
  	allow-update { <address_match_element>; ... };
  	also-notify [ port <integer> ] [ dscp <integer> ] { ( <remote-servers> | <ipv4_address> [ port <integer> ] | <ipv6_address> [ port <integer> ] ) [ key <string> ]; ... };
  	alt-transfer-source ( <ipv4_address> | * ) [ port ( <integer> | * ) ] [ dscp <integer> ];
  	alt-transfer-source-v6 ( <ipv6_address> | * ) [ port ( <integer> | * ) ] [ dscp <integer> ];
  	auto-dnssec ( allow | maintain | off ); // deprecated
  	check-dup-records ( fail | warn | ignore );
  	check-integrity <boolean>;
  	check-mx ( fail | warn | ignore );
  	check-mx-cname ( fail | warn | ignore );
  	check-names ( fail | warn | ignore );
  	check-sibling <boolean>;
  	check-spf ( warn | ignore );
  	check-srv-cname ( fail | warn | ignore );
  	check-wildcard <boolean>;
  	database <string>;
  	dialup ( notify | notify-passive | passive | refresh | <boolean> );
  	dlz <string>;
  	dnskey-sig-validity <integer>;
  	dnssec-dnskey-kskonly <boolean>;
  	dnssec-loadkeys-interval <integer>;
  	dnssec-policy <string>;
  	dnssec-secure-to-insecure <boolean>;
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
  	max-ixfr-ratio ( unlimited | <percentage> );
  	max-journal-size ( default | unlimited | <sizeval> );
  	max-records <integer>;
  	max-transfer-idle-out <integer>;
  	max-transfer-time-out <integer>;
  	max-zone-ttl ( unlimited | <duration> );
  	notify ( explicit | master-only | primary-only | <boolean> );
  	notify-delay <integer>;
  	notify-source ( <ipv4_address> | * ) [ port ( <integer> | * ) ] [ dscp <integer> ];
  	notify-source-v6 ( <ipv6_address> | * ) [ port ( <integer> | * ) ] [ dscp <integer> ];
  	notify-to-soa <boolean>;
  	parental-agents [ port <integer> ] [ dscp <integer> ] { ( <remote-servers> | <ipv4_address> [ port <integer> ] | <ipv6_address> [ port <integer> ] ) [ key <string> ]; ... };
  	parental-source ( <ipv4_address> | * ) [ port ( <integer> | * ) ] [ dscp <integer> ];
  	parental-source-v6 ( <ipv6_address> | * ) [ port ( <integer> | * ) ] [ dscp <integer> ];
  	serial-update-method ( date | increment | unixtime );
  	sig-signing-nodes <integer>;
  	sig-signing-signatures <integer>;
  	sig-signing-type <integer>;
  	sig-validity-interval <integer> [ <integer> ];
  	update-check-ksk <boolean>;
  	update-policy ( local | { ( deny | grant ) <string> ( 6to4-self | external | krb5-self | krb5-selfsub | krb5-subdomain | ms-self | ms-selfsub | ms-subdomain | name | self | selfsub | selfwild | subdomain | tcp-self | wildcard | zonesub ) [ <string> ] <rrtypelist>; ... } );
  	zero-no-soa-ttl <boolean>;
  	zone-statistics ( full | terse | none | <boolean> );
  };
