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
  	type static-stub;
  	allow-query { <address_match_element>; ... };
  	allow-query-on { <address_match_element>; ... };
  	forward ( first | only );
  	forwarders [ port <integer> ] [ dscp <integer> ] { ( <ipv4_address> | <ipv6_address> ) [ port <integer> ] [ dscp <integer> ]; ... };
  	max-records <integer>;
  	server-addresses { ( <ipv4_address> | <ipv6_address> ); ... };
  	server-names { <string>; ... };
  	zone-statistics ( full | terse | none | <boolean> );
  };
