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

BIND 9.20.23
------------

Security Fixes
~~~~~~~~~~~~~~

- Fix outgoing zone transfers' quota issue. ``1006b044b7``

  Unauthorized clients could consume outgoing zone transfers quota and
  block authorized zone transfer clients. This has been fixed.
  :gl:`#3589`

- [CVE-2026-3592] Limit resolver server list size. ``c3f3879560``

  When resolving a domain with many nameservers that share overlapping
  IP addresses (e.g., 10 NS records all pointing at the same set of
  addresses), BIND could previously waste time querying duplicate
  addresses and build up excessively large server lists. Deduplicate
  addresses in the resolver's server list so that each unique IP is only
  queried once per resolution attempt, regardless of how many NS records
  point to it and cap the number of addresses stored per nameserver name
  to 6 (combined A and AAAA), preventing memory and CPU overhead from
  domains with unusually large NS/glue sets. :gl:`#5641`

- [CVE-2026-3039] Fix GSS-API resource leak. ``92d5c60855``

  Fixed a memory leak where each GSS-API TKEY negotiation leaked a
  security context inside the GSS library. An unauthenticated attacker
  could exhaust server memory by sending repeated TKEY queries to a
  server with tkey-gssapi-keytab configured. The leaked memory was
  allocated by the GSS library, bypassing BIND's memory accounting.

  Multi-round GSS-API negotiation (GSS_S_CONTINUE_NEEDED) is now
  rejected, as BIND never supported it correctly and Kerberos/SPNEGO
  completes in a single round.

  Also implemented missing RFC 3645 requirement: the client now verifies
  that mutual authentication and integrity flags are granted by the
  GSS-API mechanism (Section 3.1.1). :gl:`#5752`

- [CVE-2026-5950] Avoid unbounded recursion loop. ``568be408bc``

  A bug during bad server handling could cause the resolver to enter an
  infinite loop, continuously sending queries to an upstream server with
  no exit condition, until the resolver query timeout was hit. This has
  been fixed.

  ISC would like to thank Billy Baraja (BielraX) for bringing this issue
  to our attention. :gl:`#5804`

- [CVE-2026-5947] Fix crash in resolver when SIG(0)-signed responses are
  received under load. ``9831f41894``

  A resolver could crash when handling a SIG(0)-signed response if the
  matching client query was cancelled while signature verification was
  still in progress — for example, when the recursive-clients quota was
  exhausted. This has been fixed. :gl:`#5819`

- [CVE-2026-3593] Add system test for HTTP/2 SETTINGS frame flood.
  ``3be272e26d``

  A use-after-free vulnerability in the DNS-over-HTTPS implementation
  could cause named to crash when a client sends a flood of HTTP/2
  SETTINGS frames while a DoH response is being written. This affects
  servers with DoH (DNS-over-HTTPS) enabled.

  ISC would like to thank Naresh Kandula Parmar (Nottiboy) for reporting
  this.

  For: #5755

- [CVE-2026-5946] Disable recursion, UPDATE, and NOTIFY for non-IN
  views. ``014be8be87``

  Recursion, dynamic updates (UPDATE), and zone change notifications
  (NOTIFY) are now disabled for views with a class other than IN (such
  as CHAOS or HESIOD); authoritative service for non-IN zones (e.g.
  version.bind in class CHAOS) continues to work as before. Servers
  configured with recursion yes in a non-IN view will log a warning at
  startup, and named-checkconf flags the same condition. UPDATE and
  NOTIFY messages that specify the meta-classes ANY or NONE in the
  question section are now rejected with FORMERR.

  This addresses a set of closely related security issues collectively
  identified as CVE-2026-5946. ISC would like to thank Mcsky23 for
  bringing these issues to our attention.

Removed Features
~~~~~~~~~~~~~~~~

- Remove obsolete KEY record EXTENDED flag deprecated by RFC 3445.
  ``99c226576a``

  KEY resource records originally defined EXTENDED flag that was removed
  by RFC 3445 back in 2002. BIND still carried code to parse and emit
  it, including the additional two-octet flags field that followed when
  the EXTENDED bit was set. That handling has been removed and the
  affected bit positions are now reserved.

  Dropping the extended-flags handling also eliminates a possible crash
  that could be reached when signing a zone containing an invalid key.
  :gl:`#5900`

  Partial backport of MR !11961 :gl:`!11962`

Feature Changes
~~~~~~~~~~~~~~~

- Revert isdelegation() to return boolean value again. ``6d89bfdf03``

  :gl:`#5838` :gl:`!11802`

- Fix CPU spikes and slow queries when cache approaches memory limit.
  ``e21ae6358a``

  When the cache grew close to the configured max-cache-size, every
  subsequent entry triggered all worker threads to run cache cleanup at
  once, causing CPU spikes and a drop in query throughput. Cleanup is
  now spread probabilistically across inserts as memory approaches the
  limit, so the work is distributed evenly instead of piling up at the
  threshold.

- Fix off by one error in dnssec-ksr sign. ``819df0d19e``

  If the inception time of the signature is exactly equal to the
  inactive time of the key, add the signature. :gl:`!11795`

Bug Fixes
~~~~~~~~~

- Check validator name when adding EDE text. ``b6c3390aea``

  When a validator is being shut down, the associated name `val->name`
  is set to NULL.  This could cause a crash if a worker thread
  subsequently added an EDE code with `val->name` in the extra text.

  `validator_addede()` now checks whether the name is NULL before trying
  to add it to the extra text. :gl:`#5613` :gl:`!11977`

- Use the zone file's basename as origin in DNSSEC tools. ``097c14da45``

  In `dnssec-signzone` and `dnssec-verify`, when the zone origin is not
  specified using the `-o` parameter, the default behavior is to try to
  sign using the zone's file name as the origin. So, for example,
  `dnssec-signzone -S example.com` will work, so long as the file name
  matches the zone name.

  This now also works if the zone is in a different directory. For
  example, `dnssec-signzone -S zones/example.com` will set the origin
  value to `example.com`. :gl:`#5678` :gl:`!11784`

- Fix a possible race condition during zone transfers. ``a48b287d9f``

  The :iscman:`named` process could terminate unexpectedly when
  processing an IXFR message during a zone transfer. This has been
  fixed. :gl:`#5767` :gl:`!11799`

- Make BIND9 compatible with OpenSSL 4. ``8242105d5d``

  OPENSSL_cleanup() in OpenSSL 4 doesn't free the memory, and that is
  not compatible with BIND 9's memory leak detection code. Don't use
  custom allocation/deallocation functions for OpenSSL's internal memory
  management.

  See https://github.com/openssl/openssl/pull/29721 :gl:`#5808`
  :gl:`!11896`

- Fix named crash when processing SIG records in dynamic updates.
  ``9e34ef0f7e``

  Previously, :iscman:`named` could abort if a client sent a dynamic
  update containing a SIG record (the legacy signature type) to a zone
  configured with an update-policy. The function `dns_db_findrdataset`
  had an incorrect requirements prerequisite that prevented SIG records
  being looked up, which was triggered as part of processing an UPDATE
  request and could be triggered remotely by any client permitted to
  send updates. This has been fixed by ensuring that SIG records are
  handled consistently with RRSIG records during update processing.
  :gl:`#5818` :gl:`!11876`

- Fix crash in resolver when SIG(0)-signed responses are received under
  load. ``bbe0b9b8f6``

  A resolver could crash when handling a SIG(0)-signed response if the
  matching client query was cancelled while signature verification was
  still in progress — for example, when the recursive-clients quota was
  exhausted. This has been fixed. :gl:`#5819`

- Fix zone verification of NSEC3 signed zones. ``de4a9b4fa6``

  Previously, when computing the compressed bitmap during verification
  of an NSEC3-signed zone, an undersized buffer was used that resulted
  in an out-of-bounds write if there were too many active windows in the
  bitmap. This impacted mirror zones which are NSEC3-signed,
  `dnssec-signzone` and `dnssec-verifyzone`. This has been fixed.
  :gl:`#5834` :gl:`!11833`

- Prevent a crash when using both dns64 and filter-aaaa. ``ddcacbc5a8``

  An assertion failure could be triggered if both `dns64` and the
  `filter-aaaa` plugin were in use simultaneously. This happened if the
  plugin triggered a second recursion process, which then attempted to
  store DNS64 state information in a pointer that had already been set
  by the original recursion process. This has been fixed. :gl:`#5854`
  :gl:`!11967`

- Remove unnecessary dns_name_free call. ``35d94fffb0``

  When processing a catalog zone member's primaries definition and there
  is a TXT record containing an invalid name TSIG key name,
  dns_name_free was incorrectly called triggering an assertion. This has
  been fixed. :gl:`#5858` :gl:`!11848`

- Tidy up the cleanup path in check_signer() ``cf517f73d5``

  When check_signer() processed a DNSKEY whose public-key data could not
  be parsed, the early return on the parse error skipped the cleanup of
  the cloned signature rdataset.  In every code path that currently
  reaches this function the cloned rdataset holds no resources, so no
  memory was actually leaked, but the cleanup is restructured so the
  parse and the iteration cannot diverge again. :gl:`#5869` :gl:`!11957`

- Prevent malicious DNSSEC zones from exhausting validator CPU.
  ``c425827743``

  A DNSSEC-signed zone could publish a DNSKEY with an unusually large
  RSA public exponent and force any validator resolving names in that
  zone to spend disproportionate CPU verifying signatures.  The
  validator now rejects such DNSKEYs, matching the limit already applied
  to keys read from files or HSMs. :gl:`#5881` :gl:`!11923`

- Fix inverted gethostname() check in rndc status. ``5ede4a87eb``

  The replacement of named_os_gethostname() with raw gethostname()
  inverted the success check: the "localhost" fallback runs on success,
  and on failure the uninitialized hostname buffer is read by
  snprintf(), leaking stack memory via the rndc status reply.
  :gl:`#5889` :gl:`!11881`

- Fix rndc-confgen aborting on HMAC-SHA-384/512 keys above 512 bits.
  ``7e1eace6cd``

  `rndc-confgen -A hmac-sha384` and `-A hmac-sha512` documented a `-b`
  range of 1..1024, but any value above 512 aborted on hardened builds
  instead of producing a key. The full advertised range now works.
  :gl:`#5903` :gl:`!11910`

- Prevent crafted queries from degrading RRL performance. ``bf4cdca7e9``

  With response rate limiting enabled, an attacker sending queries from
  many spoofed source addresses could steer entries into the same slot
  of the internal rate-limit table and slow down query processing on the
  affected server. The table now uses a per-process keyed hash so the
  placement of entries cannot be predicted or influenced from the
  network. :gl:`#5906` :gl:`!11952`

- Fix swapped arguments in redirect2() single-label branch.
  ``3728b405ea``

  On a recursive resolver with nxdomain-redirect configured, an NXDOMAIN
  result for a query whose qname is the root could corrupt the view's
  nxdomain-redirect target, after which the redirect feature stopped
  working for every subsequent query in that view until named was
  restarted. :gl:`#5908` :gl:`!11913`

- Free per-command rndc state when response serialisation fails.
  ``070b394f53``

  When isccc_cc_towire failed while building an rndc reply,
  control_respond returned without releasing the per-command request,
  response, HMAC secret copy, and text buffer.  They were eventually
  freed when the connection closed, but until then the HMAC key copy
  stayed in named's memory.  The failure path now goes through the same
  cleanup label as every other error. :gl:`#5913` :gl:`!11919`

- Prevent rare named crash when notifies are cancelled. ``49509dcbae``

  Under heavy load, named could occasionally crash when a queued
  outbound notify or zone refresh was cancelled at the moment it was
  being sent — for example, while a zone was being reloaded or removed.
  The race that caused the crash is now prevented. :gl:`#5915`
  :gl:`!11922`

- Stop delv from aborting on a malformed query name. ``ca8315bb4d``

  delv aborts with SIGABRT instead of exiting cleanly when given a query
  name that fails wire-format conversion (e.g. a label longer than 63
  octets). After this change delv prints the parse error and exits with
  a normal failure code. :gl:`#5916` :gl:`!11927`

- Fix a crash when reconfiguring while an NTA is being rechecked.
  ``971ca4df1a``

  When named was reconfigured or shut down while a negative trust anchor
  was being rechecked against authoritative servers, the in-flight
  recheck could outlive the view that owned it and cause `named` to
  crash.  This has been fixed. :gl:`#5938` :gl:`!11966`

- Fix a bug in allow-query/allow-transfer catalog zone custom
  properties. ``e962fd459e``

  The :iscman:`named` process could terminate unexpectedly when
  processing a catalog zone with an invalid ``allow-query`` or
  ``allow-transfer`` custom property (i.e. having a non-APL type)
  coexisting with the valid property. This has been fixed. :gl:`#5941`
  :gl:`!11975`

- Fix a stack use-after-free in qpzone. ``ddea991c07``

  In previous_closest_nsec(), a new qpreader was opened to search the
  NSEC tree. It was possible for that to be used to update a QP iterator
  object owned by the caller, and then be destroyed when the function
  returned.

  This has been addressed by having the caller open the NSEC qpreader
  instead. :gl:`#5942` :gl:`!11956`

- Fix a memory leak issue in the catalog zones. ``5fcb6d8809``

  The :iscman:`named` process could leak small amounts of memory when
  processing a catalog zone entry which had defined custom primary
  servers with TSIG keys using both the regular ``primaries`` custom
  property syntax and the legacy alternative syntax (``masters``) at the
  same time. This has been fixed. :gl:`#5943` :gl:`!11973`

- Fix suppressed missing-glue check in named-checkzone. ``dc5eb3fe25``

  named-checkzone and named-checkconf -z silently skipped the
  missing-glue check for any NS name that had already triggered an
  extra-AAAA-glue warning, so zones missing required A glue could pass
  validation and be deployed with broken delegations. :gl:`!11905`

- Implement seamless outgoing TCP connection reuse. ``eb117e16b9``

  The resolver can and will reuse outgoing TCP connections to the same
  host, as recommended by RFC 7766. This prevents a whole class of
  attacks that abuse the fact that establishing a TCP connection is
  expensive and it is fairly easy to deplete the outgoing TCP ports by
  putting them into TIME_WAIT state.

  The number of pipelined queries per connection is capped at 256 to
  limit the impact of a connection drop. :gl:`!11846`

- Pass empty string instead of NULL to ns_client_dumpmessage()
  ``24cdf8c096``

  Pass "" instead of NULL to ns_client_dumpmessage() to get the log
  message printed.

- Reject record sets too large to serve in DNS. ``933a8de056``

  When BIND was asked to store a record set whose total size exceeds
  what fits in a DNS message, it would allocate memory and build the
  structure, then fail later at response time. Such oversized record
  sets are now rejected at the time of storage with an error, avoiding
  wasted work on data that can never be served. :gl:`!11964`


