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

BIND 9.20.29
------------

Security Fixes
~~~~~~~~~~~~~~

- [CVE-2026-19668] Prevent excessive CPU use validating crafted DNSSEC
  responses. ``a0a61dba9e``

  A malicious authoritative server could serve a securely delegated zone
  whose DS and DNSKEY records carry many distinct key tags but no valid
  match, forcing a validating resolver into excessive key-tag matching
  and high CPU use for every query. BIND now bounds this work with the
  per-query validation limit (max-validations-per-fetch). :gl:`#5349`

- [CVE-2026-19033] Require a TSIG on every message of incoming zone
  transfers. ``9404cd2b8c``

  BIND 9 used to accept TSIG-signed zone transfers in which some
  messages were unsigned, and processed those messages before the next
  signature could vouch for them. It now requires a TSIG on every
  message of an incoming AXFR or IXFR; all modern nameserver already
  sign every message, so no change is expected in practice. :gl:`#6062`

- [CVE-2026-77119] Prevent a DNSSEC downgrade of secure delegations via
  unrelated NSEC3. ``3bed9c8e9e``

  A validating resolver could be tricked into treating a secure
  delegation as unsigned and accepting forged answers for names beneath
  it, if an attacker could inject responses to its queries. Such forged
  proofs are now rejected. :gl:`#6234`

- [CVE-2026-19941] Prevent forged DNSSEC-validated NXDOMAIN responses.
  ``a36bf58daf``

  A validating resolver could accept a signed NSEC record from an
  unrelated zone as proof that a wildcard did not exist. An on-path
  attacker or malicious forwarder controlling a signed zone could
  therefore forge an authenticated NXDOMAIN response for a name that
  should resolve through a wildcard. BIND now requires the
  wildcard-denial and name-nonexistence proofs to be signed by the same
  zone. :gl:`#6253`

- [CVE-2026-19666] DNS64 with break-dnssec could cause an assertion
  failure. ``4cec4965c4``

  When a "dns64" statement is configured with "break-dnssec yes" and its
  "exclude" list matches some but not all of the addresses in an AAAA
  RRset, named removes the excluded addresses from the answer instead of
  synthesizing new ones. If the answer being filtered had been cached
  together with a proof that the queried name does not exist -- which is
  what a wildcard match produces -- named terminated with an assertion
  failure.

  Only recursive resolvers are affected, and only when "break-dnssec
  yes" is in use; the answer has to come from the cache, so a server
  that is only authoritative cannot reach this. :gl:`#6301`

- [CVE-2026-19667] Reject negative cache records that do not fit in a
  dns_rdata_t. ``dbf08c8581``

  A single crafted response from a server could make a resolver cache a
  malformed negative entry and then terminate with an assertion failure
  when reading it back. Only recursive resolvers are affected, on a
  default configuration. :gl:`#6302`

- [CVE-2026-19662] Prevent resolver crash with cached DNSSEC proofs.
  ``c884cc1ba0``

  Under certain timing conditions, concurrent recursive queries could
  cause named to crash when cached DNSSEC NOQNAME proof data was
  replaced while still in use. Cached proof data is now retained until
  all queries using it have completed. :gl:`#6333`

- [CVE-2026-75029] Discard repeated SOA, CNAME, and DNAME records when
  parsing DNS messages. ``0d630758c2``

  A DNS message could carry the same SOA, CNAME, or DNAME record many
  times, and named kept every copy while parsing it. With name
  compression those copies took up far more memory internally than in
  the message itself, and every later processing step had to handle all
  of them. named now keeps the first copy of such a record and discards
  identical repeats. :gl:`#6335`

- [CVE-2026-77692] Fix an unauthenticated crash on HTTPS using SIG(0)
  ``5a24401c5c``

  A specifically crafted HTTPS query using SIG(0) as authentication
  could crash named if the client closes the connection before named
  actually verifies the signature. This is now fixed. :gl:`#6343`

- [CVE-2026-81736] Cached HTTPS/SVCB aliases could exhaust resolver CPU.
  ``20bbb1639a``

  A recursive resolver that had cached a large set of interlinked HTTPS
  or SVCB records in alias form could be driven to do an excessive
  amount of work assembling a single response, because it followed every
  cached alias target when building the additional section. A client
  permitted to use recursion, together with an attacker-controlled zone
  used to plant the records, could repeat small queries to consume
  enough CPU to delay or deny service to other clients. The amount of
  additional processing done for one query is now bounded. :gl:`#6347`

- [CVE-2026-76163] Prevent TKEY queries from terminating named without
  global options. ``7645138538``

  named could terminate unexpectedly when a remote client sent a TKEY
  query if the configuration did not include a global options statement.
  This has been fixed.

  ISC thanks Owais Lone (thesecguy) for reporting the issue. :gl:`#6357`

- [CVE-2026-78301] Out-of-zone records in a zone database could be
  served as authoritative. ``72a10c3a0b``

  When a zone database contained records for names outside the zone —
  such as a delegation above the zone apex, left behind by a secondary
  that had accepted out-of-zone data from its primary — the server could
  treat them as authoritative and answer queries for names inside the
  zone with that out-of-zone data instead of the zone's own. A server
  that was also a resolver could follow such a delegation and cache the
  answers of the server it named, affecting names outside the configured
  zone. Zone database lookups are now confined to names at or below the
  zone's origin.

  ISC would like to thank Henrique Pereira for reporting the issue.
  :gl:`#6361`

- [CVE-2026-80274] Crash on wildcard answers carrying both NSEC and
  NSEC3 proofs. ``0e44451b1a``

  When a wildcard answer arrived with both NSEC and NSEC3 records at the
  name proving that the queried name does not exist, the resolver could
  pick different records when caching the answer and when retrieving the
  proof, depending on the order in which the authoritative server sent
  them. This could terminate named with an assertion failure, fail the
  query with SERVFAIL, or serve a denial record other than the one that
  had been verified. The resolver now caches and serves the same denial
  record it accepted when the answer was received.

  ISC would like to thank hythyt for reporting the issue. :gl:`#6369`

- [CVE-2026-81563] Following HTTPS/SVCB aliases could leak resolver
  cache memory. ``3162df369e``

  When a recursive server answered a query for an HTTPS or SVCB record
  in alias form and the alias target had more than 13 records, the
  target records were pinned in the cache permanently instead of being
  released once the answer was sent. A remote party who could make the
  server follow such aliases to a steady stream of fresh names could
  grow the cache beyond the configured max-cache-size until the server
  was unable to resolve unrelated names. The records are now released
  correctly.

  ISC would like to thank Samy Medjahed/Ap4sh for reporting the issue.
  :gl:`#6374`

New Features
~~~~~~~~~~~~

- Add an agent skill for the isc_job/isc_async/isc_work APIs.
  ``fe32990b06``

  Documents when to use isc_job_run(), isc_async_run() or
  isc_work_enqueue(), and the contract each one imposes. No functional
  change. :gl:`!12561`

Removed Features
~~~~~~~~~~~~~~~~

- Remove unused closest encloser proof caching. ``abd8b5bfd8``

  BIND used to cache an NSEC3 closest encloser proof alongside positive
  wildcard answers so that a resolver could re-send it when answering
  from its cache. That stopped being used in BIND 9.9 (2011), when
  positive wildcard responses were changed to omit that NSEC3 record —
  RFC 5155 requires only the next closer name proof — and the closest
  encloser came to be  derived during validation instead. The caching
  code has been unreachable ever since, so this removes it with no
  change in behaviour. :gl:`#5803` :gl:`!12660`

Feature Changes
~~~~~~~~~~~~~~~

- Reject oversized and malformed DNSKEY records up front. ``6c22109924``

  Oversized RSA key material in a DNSKEY record was only rejected after
  it had been converted, allocating memory proportional to the record
  size. Such records are now rejected before conversion, as are Ed25519
  and Ed448 keys with trailing bytes that were previously silently
  ignored. :gl:`#4537` :gl:`!12560`

Bug Fixes
~~~~~~~~~

- Prevent a crash when using both dns64 and filter-a. ``bce5d10d18``

  An assertion failure was possible when using both `dns64` and the
  `filter-a` plugin simultaneously; this has been fixed. :gl:`#5979`
  :gl:`!12663`

- Fix update-policy grant external address passing. ``b1e955c326``

  Only TCP client addresses are supposed to be passed to an `external`
  handler for the associated `update-policy` rule, but UDP client
  addresses were also being passed.  This could have caused the external
  handler to return a result it otherwise wouldn't. This has been fixed.
  :gl:`#6061` :gl:`!12555`

- Missing required NSEC3 for delegation not detected. ``e84ed2e9d7``

  A missing required NSEC3 record for an insecure delegation in a non
  OPTOUT range was not being detected.  This has been fixed. :gl:`#6063`
  :gl:`!12611`

- Tighten EUI48 and EUI48 text parsing. ``ff50f2cdf1``

  Malformed EUI48 and EUI64 records could be accepted.  This has been
  fixed. :gl:`#6082` :gl:`!12521`

- GeoIP ACL state can be stale or wrong after reload. ``63baf425b3``

  `named` caches GeoIP information after looking it up, but the cached
  information was not invalidated when the GeoIP database was reloaded,
  so it could continue to be used.  We now invalidate existing cached
  GeoIP information as part of the reloading process. :gl:`#6083`
  :gl:`!12662`

- Honor DNSSEC policy key tag ranges. ``b82e5834b7``

  When a DNSSEC policy configured a non-default tag-range, dnssec-keygen
  and dnssec-ksr could accept generated keys outside that range. Both
  tools now honor the configured minimum and maximum key tags.
  :gl:`#6091` :gl:`!12549`

- Fix double free in mdig when EDNS options are specified.
  ``af5bd0b0ff``

  When the default_query is cloned the EDNS options need to be cloned
  rather than the pointer copied.  The old behaviour results in a double
  free of the options.  This has been fixed. :gl:`#6095` :gl:`!12661`

- Fix a crash when an IXFR falls back to AXFR with updates still
  pending. ``e34062bc7e``

  When a secondary zone received an incremental transfer (IXFR) and the
  primary then caused named to fall back to a full transfer (AXFR) while
  some of the already-received incremental changes were still waiting to
  be applied, named could later crash when that transfer finished. The
  pending changes are now discarded correctly before the AXFR retry.
  :gl:`#6114` :gl:`!12624`

- Fix DS requests to parental agents over TLS. ``55830d30f6``

  TLS configuration for parental agents was being ignored when sending
  DS requests. This has been fixed. :gl:`#6135` :gl:`!12613`

- Fix a crash when resolving names below a cached DNAME. ``b94e940f52``

  A recursive resolver could crash when it answered a query for a name
  beneath a cached DNAME while that same DNAME record was concurrently
  refreshed or evicted from the cache. :gl:`#6182` :gl:`!12593`

- Rndc-confgen `-q` (quiet) option is documented but doesn't work.
  ``7e4a7ca1a7``

  The command line parsing in rndc-confgen was broken so  `rndc-confgen
  -q` did not work.  This has been fixed. :gl:`#6187` :gl:`!12575`

- Enforce query ACLs for redirect zones and searched DLZs.
  ``bc69876b2e``

  Queries answered from redirect zones or searched DLZ databases did not
  consistently honor `allow-query` and `allow-query-on`, potentially
  exposing restricted DNS data to excluded clients or through excluded
  listening addresses. These ACLs are now enforced before redirect or
  DLZ data is returned. :gl:`#6251`, #6252 :gl:`!12646`

- Check "asnum" validity in GeoIP ACLs. ``28c2bfdc7b``

  We now check the validity of autonomous system (AS) numbers when
  parsing GeoIP ACLs that use `asnum` elements at configuration time.

  `asnum` values start with an optional case-insensitive "AS" prefix,
  followed only by decimal digits, with no spaces or other extraneous
  characters. The value represented cannot exceed 2^32. :gl:`#6255`
  :gl:`!12511`

- Prevent crashes while reporting DNSSEC signing statistics.
  ``c190514f0a``

  Servers with zone-statistics full could terminate while reporting
  DNSSEC signing statistics for a zone tracking adding more than four
  signing keys. :gl:`#6256` :gl:`!12674`

- Fix various nits in the netmgr code. ``c28cdad51b``

  The MR consists of couple of small fixes and uncaught errors in the
  Network Manager. :gl:`#6257` :gl:`!12576`

- Fix a crash on remote-servers lists that reference themselves.
  ``aaae614f9d``

  Since 9.21.16 and 9.20.17, a remote-servers, primaries, masters, or
  parental-agents list that referenced itself, directly or through
  another list, made named crash on startup or reconfiguration. Such
  references are again skipped and the remaining entries in the list are
  used, as in earlier versions. :gl:`#6287` :gl:`!12604`

- A record from outside a response policy zone could stop named.
  ``d135513b37``

  A response policy zone transferred from a primary can contain a record
  whose name lies outside the zone. Such a record could stop named, both
  when it arrived and again at every startup afterwards, because a
  secondary keeps it in its own copy of the zone. Records like this are
  now rejected and logged; previously one could also silently create a
  policy entry for an unrelated name. :gl:`#6304` :gl:`!12543`

- "rndc flushtree ." failed to flush the cache. ``96e8b585ed``

  `rndc flushtree` flushes cache data below a specified name. If the
  name specified is the DNS root, it should fully empty the cache, the
  same as `rndc flush`.  However, there was a bug causing the command,
  in that case, to have no effect on the cache at all; this has been
  fixed. :gl:`#6308` :gl:`!12582`

- Invalid key-store configuration could abort the DNSSEC tools.
  ``1d796ab072``

  Invalid configured key-stores named "key-directory" in configuration
  files could abort the DNSSEC tools. This has been fixed. :gl:`#6313`
  :gl:`!12653`

- NSEC signature set could bypass the secure-delegation check.
  ``c966177f6c``

  When proving that a delegation is insecure, the validator bounded an
  NSEC record's authority by the signer of whichever RRSIG happened to
  come first in the record's signature set, rather than the signature
  that actually verified. A grandparent NSEC padded with an extra,
  unverifiable signature could therefore pass the check that keeps such
  proofs from reaching below a signed child zone. The validator now
  requires every signature on the NSEC to name the same signer and
  refuses proofs whose signature set is malformed or larger than
  max-validations-per-fetch allows. :gl:`#6321`

- Fix a possible nsupdate issue when using GSS-TSIG. ``4ddcab2d3c``

  The :iscman:`nsupdate` process could terminate unexpectedly when using
  the GSS-TSIG mode executed with the :option:`nsupdate -g` option. This
  has been fixed. :gl:`#6325` :gl:`!12588`

- Fix isccc_alist_define error paths. ``af1349552a``

  If there is an out of memory error in isccc_alist_define a memory leak
  (the sexpr holding the key name) or a double free (value) could occur.
  This has been fixed. :gl:`#6329` :gl:`!12636`

- Check for empty 'endpoints' list. ``23f58af443``

  Configuring an `http` block with `endpoints {};` previously caused a
  crash in `named`. This is now rejected earlier by the configuration
  check. :gl:`#6330` :gl:`!12552`

- Named could crash with a single-element geoip sortlist. ``0e996a4d3b``

  If `named` was configured with a single-element sortlist containing a
  `geoip` ACL element, any matching query triggered an assertion
  failure. This has been fixed. :gl:`#6342` :gl:`!12583`

- Prevent out-of-bailiwick CNAMEs from evicting cached records.
  ``cdedd4acd5``

  A recursive resolver could remove valid cached records when a DNS
  response contained an out-of-bailiwick CNAME with the same owner name.
  Out-of-bailiwick data is now discarded before it can modify the cache.
  :gl:`#6345` :gl:`!12651`

- Restore periodic cleanup of stale resolver address data.
  ``356f4013f8``

  Stale resolver address data could remain cached until memory pressure
  or an explicit flush. Correct the cleanup interval so it is removed
  periodically. :gl:`#6346` :gl:`!12589`

- Fix named-checkconf/named crash with malformed key name.
  ``9f218f6aaf``

  When a primary/remote-server key name was malformed, named-checkconf
  and named were both crashing (after warning about the invalid key
  name). This is now fixed. :gl:`#6362` :gl:`!12639`

- Fix -Wformat-truncation warning in totext_in_wks() ``f97c2bea40``

  BIND 9 failed to build with GCC 16 at -O3: rendering a WKS record as
  text triggered a -Wformat-truncation error, which is fatal in
  developer builds. The port number is now printed with a 16-bit format
  specifier, so the compiler can see it always fits the output buffer.
  :gl:`!12542`

- Fix off-by-one errors caused by magic hardcoded values. ``726c6cb795``

  Fix off-by-one comparinson errors: "named -p http=" dropped the first
  digit of the given port (for example, "http=8080" selected port 80)
  and now uses the port as given, and "named-rrchecker -C" compared only
  part of the "CLASS" prefix when filtering generic class names, which
  was harmless in practice but is now corrected. :gl:`!12616`

- Hmac_verify() now accepts truncated HMACs only when requested.
  ``c81b111496``

  The hmac_verify() function incorrectly compares only up to
  'sig->length' bytes, but the signature and its length should not be
  trusted, e.g. in case if it comes from a user query.

  Don't accept signatures which length isn't equal to the expected
  calculated HMAC length unless it is explicitly requested by the
  caller, e.g. for truncated TSIG [1] support.

  [1] https://datatracker.ietf.org/doc/html/rfc8945#name-tsig-truncation
  -policy :gl:`!12629`

- Prevent resolver crashes while processing DNS over TCP. ``81b3b6d89f``

  Recursive resolvers could terminate with an assertion failure while
  processing DNS responses over TCP under sustained traffic. The failure
  was observed on resolvers configured globally with forward only; the
  same transport path is also used by iterative resolution.  This has
  been fixed. :gl:`!12537`


