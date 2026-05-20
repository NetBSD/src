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

Notes for BIND 9.20.23
----------------------

Security Fixes
~~~~~~~~~~~~~~

- Limit resolver server list size. :cve:`2026-3592`

  When resolving a domain with many nameservers that shared overlapping
  IP addresses (e.g., 10 NS records all pointing at the same set of
  addresses), BIND could previously waste time querying duplicate
  addresses and build up excessively large server lists. Addresses in
  the resolver's server list are now deduplicated so that each unique IP is only
  queried once per resolution attempt, regardless of how many NS records
  point to it. The number of addresses stored per nameserver name
  is also now capped at six (combined A and AAAA), preventing memory and CPU overhead from
  domains with unusually large NS/glue sets.

  ISC would like to thank Shuhan Zhang from Tsinghua University for
  reporting this issue. :gl:`#5641`

- Fix GSS-API resource leak. :cve:`2026-3039`

  A memory leak was fixed where each GSS-API TKEY negotiation leaked a
  security context inside the GSS library. An unauthenticated attacker
  could exhaust server memory by sending repeated TKEY queries to a
  server with :any:`tkey-gssapi-keytab` configured. The leaked memory was
  allocated by the GSS library, bypassing BIND's memory accounting.

  Multi-round GSS-API negotiation (GSS_S_CONTINUE_NEEDED) is now
  rejected, as BIND never supported it correctly and Kerberos/SPNEGO
  completes in a single round.

  ISC would like to thank Vitaly Simonovich for bringing this
  vulnerability to our attention. :gl:`#5752`

- Disable recursion, UPDATE, and NOTIFY for non-IN views.
  :cve:`2026-5946`

  Recursion, dynamic updates (UPDATE), and zone change notifications
  (NOTIFY) are now disabled for views with a class other than IN (such
  as CHAOS or HESIOD); authoritative service for non-IN zones (e.g.
  version.bind in class CHAOS) continues to work as before. Servers
  configured with :namedconf:ref:`recursion yes; <recursion>`
  in a non-IN view log a warning at
  startup, and :iscman:`named-checkconf` flags the same condition. UPDATE and
  NOTIFY messages that specify the meta-classes ANY or NONE in the
  question section are now rejected with FORMERR.

  This addresses a set of closely related security issues collectively
  identified as CVE-2026-5946. ISC would like to thank Mcsky23 for
  bringing these issues to our attention. :gl:`#5784`

- Avoid unbounded recursion loop. :cve:`2026-5950`

  A bug during bad server handling could cause the resolver to enter an
  infinite loop, continuously sending queries to an upstream server with
  no exit condition, until the resolver query timeout was hit. This has
  been fixed.

  ISC would like to thank Billy Baraja (BielraX) for bringing this issue
  to our attention. :gl:`#5804`

- Fix crash in resolver when SIG(0)-signed responses are received under
  load. :cve:`2026-5947`

  A resolver could crash when handling a SIG(0)-signed response if the
  matching client query was cancelled while signature verification was
  still in progress — for example, when the recursive-clients quota was
  exhausted. This has been fixed.

  ISC would like to thank Naoki Wakamatsu for bringing this
  vulnerability to our attention. :gl:`#5819`

- Fix use-after-free error in DNS-over-HTTPS when processing HTTP/2
  SETTINGS frames. :cve:`2026-3593`

  Previously, a use-after-free vulnerability in the DNS-over-HTTPS implementation
  could cause :iscman:`named` to crash when a client sent a flood of HTTP/2
  SETTINGS frames while a DoH response was being written. This affected
  servers with DoH (DNS-over-HTTPS) enabled and has been fixed.

  ISC would like to thank Naresh Kandula Parmar (Nottiboy) for reporting
  this. :gl:`#5755`

- Fix outgoing zone transfers' quota issue.

  Unauthorized clients could consume the entire outgoing zone-transfer quota and
  block authorized zone transfer clients. This has been fixed.
  :gl:`#3589`

Feature Changes
~~~~~~~~~~~~~~~

- Fix CPU spikes and slow queries when cache approaches memory limit.

  Cache cleanup is now spread probabilistically to avoid CPU usage spikes and a
  drop in query throughput. :gl:`#5891`

Bug Fixes
~~~~~~~~~

- Use the zone file's basename as origin in DNSSEC tools.

  In :iscman:`dnssec-signzone` and :iscman:`dnssec-verify`, when the zone origin is not
  specified using the ``-o`` parameter, the default behavior is to try to
  sign using the zone's file name as the origin. So, for example,
  ``dnssec-signzone -S example.com`` will work, so long as the file name
  matches the zone name.

  This now also works if the zone is in a different directory. For
  example, ``dnssec-signzone -S zones/example.com`` will set the origin
  value to ``example.com``. :gl:`#5678`

- Fix a possible race condition during zone transfers.

  The :iscman:`named` process could terminate unexpectedly when
  processing an IXFR message during a zone transfer. This has been
  fixed. :gl:`#5767`

- Fix :iscman:`named` crash when processing SIG records in dynamic updates.

  Previously, :iscman:`named` could abort if a client sent a dynamic
  update containing a SIG record (the legacy signature type) to a zone
  configured with an update-policy. The function `dns_db_findrdataset`
  had an incorrect requirements prerequisite that prevented SIG records
  from being looked up, which was triggered as part of processing an UPDATE
  request and could be triggered remotely by any client permitted to
  send updates. This has been fixed by ensuring that SIG records are
  handled consistently with RRSIG records during update processing.
  :gl:`#5818`

- Fix :option:`rndc modzone` behavior for a zone in named.conf.

  If a zone was present in the configuration file and not originally
  added by :option:`rndc addzone`, :option:`rndc modzone` for that zone would succeed
  once but subsequent :option:`rndc modzone` attempts would fail. This has been
  fixed. :gl:`#5826`

- Fix zone verification of NSEC3 signed zones.

  Previously, when computing the compressed bitmap during verification
  of an NSEC3-signed zone, an undersized buffer was used that resulted
  in an out-of-bounds write if there were too many active windows in the
  bitmap. This impacted the mirror zones which are NSEC3-signed,
  :iscman:`dnssec-signzone` and :iscman:`dnssec-verify`. This has been fixed.
  :gl:`#5834`

- Prevent a crash when using both :any:`dns64` and :any:`filter-aaaa`.

  An assertion failure could be triggered if both :any:`dns64` and the
  :any:`filter-aaaa` plugin were in use simultaneously. This happened if the
  plugin triggered a second recursion process, which then attempted to
  store DNS64 state information in a pointer that had already been set
  by the original recursion process. This has been fixed. :gl:`#5854`

- Fixed an assertion failure when processing catalog zones.

  If a TXT record containing an invalid name TSIG key name was found
  when processing a catalog zone member's primaries definition,
  ``dns_name_free`` was incorrectly called, triggering an assertion. This has
  been fixed. :gl:`#5858`

- Prevent malicious DNSSEC zones from exhausting validator CPU.

  A DNSSEC-signed zone could publish a DNSKEY with an unusually large
  RSA public exponent and force any validator resolving names in that
  zone to spend disproportionate CPU verifying signatures.  The
  validator now rejects such DNSKEYs, matching the limit already applied
  to keys read from files or HSMs. :gl:`#5881`

- Fix :iscman:`rndc-confgen` aborting on HMAC-SHA-384/512 keys above 512 bits.

  :iscman:`rndc-confgen` (with either ``-A hmac-sha384`` or
  ``-A hmac-sha512``) previously documented a ``-b``
  range of 1..1024, but any value above 512 aborted on hardened builds
  instead of producing a key. The full advertised range now works.
  :gl:`#5903`

- Prevent crafted queries from degrading RRL performance.

  With response rate limiting enabled, an attacker sending queries from
  many spoofed source addresses could steer entries into the same slot
  of the internal rate-limit table and slow down query processing on the
  affected server. The table now uses a per-process keyed hash so the
  placement of entries cannot be predicted or influenced from the
  network. :gl:`#5906`

- Prevent rare :iscman:`named` crash when notifies are cancelled.

  Under heavy load, :iscman:`named` could occasionally crash when a queued
  outbound notify or zone refresh was cancelled at the moment it was
  being sent — for example, while a zone was being reloaded or removed.
  The race that caused the crash is now prevented. :gl:`#5915`

- Stop :iscman:`delv` from aborting on a malformed query name.

  :iscman:`delv` previously aborted with SIGABRT instead of exiting cleanly when given a query
  name that failed wire-format conversion (e.g. a label longer than 63
  octets). After this change :iscman:`delv` prints the parse error and exits with
  a normal failure code. :gl:`#5916`

- Fix a crash when reconfiguring while an NTA is being rechecked.

  Previously, if :iscman:`named` was reconfigured or shut down while a negative trust anchor
  was being rechecked against authoritative servers, the in-flight
  recheck could outlive the view that owned it and cause :iscman:`named` to
  crash.  This has been fixed. :gl:`#5938`

- Fix a bug in :any:`allow-query`/:any:`allow-transfer` catalog zone custom
  properties.

  The :iscman:`named` process could terminate unexpectedly when
  processing a catalog zone with an invalid :any:`allow-query` or
  :any:`allow-transfer` custom property (i.e. having a non-APL type)
  coexisting with the valid property. This has been fixed. :gl:`#5941`

- Fix a memory leak issue in catalog zones.

  The :iscman:`named` process could leak small amounts of memory when
  processing a catalog zone entry which had defined custom primary
  servers with TSIG keys, if both the regular ``primaries`` custom
  property syntax and the legacy alternative syntax (``masters``) were used at the
  same time. This has been fixed. :gl:`#5943`

- Fix suppressed missing-glue check in :iscman:`named-checkzone`.

  :iscman:`named-checkzone` and :option:`named-checkconf -z` silently
  skipped the missing-glue check for any NS name that had already
  triggered an extra-AAAA-glue warning, so zones missing required A glue
  could pass validation and be deployed with broken delegations.
  :gl:`!11899`

- Implement seamless outgoing TCP connection reuse.

  The resolver can and will reuse outgoing TCP connections to the same
  host, as recommended by :rfc:`7766`. This prevents a whole class of
  attacks that abuse the fact that establishing a TCP connection is
  expensive and it is fairly easy to deplete the outgoing TCP ports by
  putting them into ``TIME_WAIT`` state.

  The number of pipelined queries per connection is capped at 256 to
  limit the impact of a connection drop. :gl:`!11845`

- Reject record sets too large to serve in DNS.

  When BIND was asked to store a record set whose total size exceeded
  what fit in a DNS message, it would allocate memory and build the
  structure, then fail later at response time. Such oversized record
  sets are now rejected at the time of storage with an error, avoiding
  wasted work on data that can never be served. :gl:`!11963`


