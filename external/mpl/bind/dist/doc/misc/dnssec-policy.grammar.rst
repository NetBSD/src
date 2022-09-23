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

  dnssec-policy <string> {
  	dnskey-ttl <duration>;
  	keys { ( csk | ksk | zsk ) [ ( key-directory ) ] lifetime
  	    <duration_or_unlimited> algorithm <string> [ <integer> ]; ... };
  	max-zone-ttl <duration>;
  	nsec3param [ iterations <integer> ] [ optout <boolean> ] [
  	    salt-length <integer> ];
  	parent-ds-ttl <duration>;
  	parent-propagation-delay <duration>;
  	publish-safety <duration>;
  	purge-keys <duration>;
  	retire-safety <duration>;
  	signatures-refresh <duration>;
  	signatures-validity <duration>;
  	signatures-validity-dnskey <duration>;
  	zone-propagation-delay <duration>;
  };
