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

Notes for BIND 9.20.29
----------------------

Security Fixes
~~~~~~~~~~~~~~

- Prevent excessive CPU use validating crafted DNSSEC responses.
  :cve:`2026-19668`

  A malicious authoritative server could serve a securely delegated zone
  whose DS and DNSKEY records carried many distinct key tags but no valid
  match, forcing a validating resolver into excessive key-tag matching
  and high CPU use for every query. This work is now bounded by the
  per-query validation limit (:any:`max-validations-per-fetch`).

  ISC would like to thank Zuyao Xu and Xiang Li of the All-in-One
  Security and Privacy Laboratory, Nankai University, for bringing this
  vulnerability to our attention. :gl:`#5349`

- Require a TSIG on every message of incoming zone transfers.
  :cve:`2026-19033`

  Previously, :iscman:`named` accepted TSIG-signed zone transfers in
  which some messages were unsigned, and processed those messages
  before the next signature could vouch for them. It now requires a
  TSIG on every message of an incoming AXFR or IXFR; all modern
  nameservers already sign every message, so no change is expected in
  practice. :gl:`#6062`

- Prevent a DNSSEC downgrade of secure delegations via unrelated NSEC3
  records. :cve:`2026-77119`

  A validating resolver could be tricked into treating a secure
  delegation as unsigned and accepting forged answers for names beneath
  it, if an attacker could inject responses to its queries. Such forged
  proofs are now rejected. :gl:`#6234`

- Prevent forged DNSSEC-validated NXDOMAIN responses. :cve:`2026-19941`

  A validating resolver could accept a signed NSEC record from an
  unrelated zone as proof that a wildcard did not exist. An on-path
  attacker or malicious forwarder controlling a signed zone could
  therefore forge an authenticated NXDOMAIN response for a name that
  should resolve through a wildcard. The wildcard-denial and
  name-nonexistence proofs are now required to be signed by the same
  zone. :gl:`#6253`

- DNS64 with :any:`break-dnssec` could cause an assertion failure.
  :cve:`2026-19666`

  When a :any:`dns64` statement is configured with ``break-dnssec yes;``
  and its ``exclude`` list matches some but not all of the addresses in
  an AAAA RRset, :iscman:`named` removes the excluded addresses from the
  answer instead of synthesizing new ones. If the answer being filtered
  had been cached together with a proof that the queried name did not
  exist — which is what a wildcard match produces — :iscman:`named`
  terminated with an assertion failure.

  Only recursive resolvers are affected, and only when
  ``break-dnssec yes;`` is in use; the answer has to come from the
  cache, so a server that is only authoritative cannot reach this.

  ISC would like to thank Rintaro Kawasugi for bringing this
  vulnerability to our attention. :gl:`#6301`

- Reject oversized negative cache records. :cve:`2026-19667`

  A single crafted response from a server could make a resolver cache a
  malformed negative entry and then terminate with an assertion failure
  when reading it back. Only recursive resolvers are affected, on a
  default configuration.

  ISC would like to thank Rintaro Kawasugi for bringing this
  vulnerability to our attention. :gl:`#6302`

- Prevent resolver crash with cached DNSSEC proofs. :cve:`2026-19662`

  Under certain timing conditions, concurrent recursive queries could
  cause :iscman:`named` to crash when cached DNSSEC NOQNAME proof data
  was replaced while still in use. Cached proof data is now retained
  until all queries using it have completed.

  ISC would like to thank Samy Medjahed/Ap4sh for bringing this
  vulnerability to our attention. :gl:`#6333`

- Discard repeated SOA, CNAME, and DNAME records when parsing DNS
  messages. :cve:`2026-75029`

  A DNS message could carry the same SOA, CNAME, or DNAME record many
  times, and :iscman:`named` kept every copy while parsing it. With name
  compression those copies took up far more memory internally than in
  the message itself, and every later processing step had to handle all
  of them. Only the first copy of such a record is now kept; identical
  repeats are discarded. :gl:`#6335`

- Fix an unauthenticated crash on HTTPS using SIG(0). :cve:`2026-77692`

  A specifically crafted HTTPS query using SIG(0) as authentication
  could crash :iscman:`named` if the client closed the connection before
  :iscman:`named` actually verified the signature. This is now fixed.

  ISC would like to thank Vitaly Simonovich for bringing this
  vulnerability to our attention. :gl:`#6343`

- Cached HTTPS/SVCB aliases could exhaust resolver CPU.
  :cve:`2026-81736`

  A recursive resolver that had cached a large set of interlinked HTTPS
  or SVCB records in alias form could be driven to do an excessive
  amount of work assembling a single response, because it followed every
  cached alias target when building the additional section. A client
  permitted to use recursion, together with an attacker-controlled zone
  used to plant the records, could repeat small queries to consume
  enough CPU to delay or deny service to other clients. The amount of
  additional processing done for one query is now bounded.

  ISC would like to thank Henrique Pereira for bringing this
  vulnerability to our attention. :gl:`#6347`

- Prevent TKEY queries from terminating :iscman:`named` without global
  options. :cve:`2026-76163`

  The :iscman:`named` process could terminate unexpectedly when a remote
  client sent a TKEY query and the configuration did not include a
  global :namedconf:ref:`options` statement. This has been fixed.

  ISC would like to thank Owais Lone (thesecguy) for bringing this
  vulnerability to our attention. :gl:`#6357`

- Out-of-zone records in a zone database could be served as
  authoritative. :cve:`2026-78301`

  When a zone database contained records for names outside the zone —
  such as a delegation above the zone apex, left behind by a secondary
  that had accepted out-of-zone data from its primary — the server could
  treat them as authoritative and answer queries for names inside the
  zone with that out-of-zone data instead of the zone's own. A server
  that was also a resolver could follow such a delegation and cache the
  answers of the server it named, affecting names outside the configured
  zone. Zone database lookups are now confined to names at or below the
  zone's origin.

  ISC would like to thank Henrique Pereira for bringing this
  vulnerability to our attention. :gl:`#6361`

- Fix crash on wildcard answers carrying both NSEC and NSEC3 proofs.
  :cve:`2026-80274`

  When a wildcard answer arrived with both NSEC and NSEC3 records at the
  name proving that the queried name did not exist, the resolver could
  pick different records when caching the answer and when retrieving the
  proof, depending on the order in which the authoritative server sent
  them. This could terminate :iscman:`named` with an assertion failure,
  fail the query with SERVFAIL, or serve a denial record other than the
  one that had been verified. The resolver now caches and serves the
  same denial record it accepted when the answer was received.

  ISC would like to thank hythyt for bringing this vulnerability to our
  attention. :gl:`#6369`

- Following HTTPS/SVCB aliases could leak resolver cache memory.
  :cve:`2026-81563`

  When a recursive server answered a query for an HTTPS or SVCB record
  in alias form and the alias target had more than 13 records, the
  target records were pinned in the cache permanently instead of being
  released once the answer was sent. A remote party who could make the
  server follow such aliases to a steady stream of fresh names could
  grow the cache beyond the configured :any:`max-cache-size` until the
  server was unable to resolve unrelated names. The records are now
  released correctly.

  ISC would like to thank Samy Medjahed/Ap4sh for bringing this
  vulnerability to our attention. :gl:`#6374`

Feature Changes
~~~~~~~~~~~~~~~

- Reject oversized and malformed DNSKEY records up front.

  Oversized RSA key material in a DNSKEY record was only rejected after
  it had been converted, allocating memory proportional to the record
  size. Such records are now rejected before conversion, as are Ed25519
  and Ed448 keys with trailing bytes that were previously silently
  ignored. :gl:`#4537`

Bug Fixes
~~~~~~~~~

- Prevent a crash when using both :any:`dns64` and ``filter-a``.

  An assertion failure was possible when using both :any:`dns64` and the
  ``filter-a`` plugin simultaneously; this has been fixed.
  :gl:`#5979`

- Stop passing UDP client addresses to :any:`update-policy` ``external``
  helpers.

  Dynamic update rules of type ``external`` delegate the authorization
  decision to an external helper daemon. The client address field in
  the helper request is only meant to carry TCP client addresses, which
  cannot easily be spoofed, but UDP client addresses were passed as
  well, so the helper could base its decision on an untrustworthy
  address. For updates arriving over UDP, the helper request no longer
  includes a client address. :gl:`#6061`

- Missing required NSEC3 for delegation not detected.

  A missing required NSEC3 record for an insecure delegation in a
  non-opt-out range was not being detected. This has been fixed.
  :gl:`#6063`

- Tighten EUI48 and EUI64 text parsing.

  Malformed EUI48 and EUI64 records could be accepted. This has been
  fixed. :gl:`#6082`

- GeoIP ACL state could be stale or wrong after reload.

  Previously, :iscman:`named` cached GeoIP information after looking it
  up, but the cached information was not invalidated when the GeoIP
  database was reloaded, so it could continue to be used. Existing
  cached GeoIP information is now invalidated as part of the reloading
  process. :gl:`#6083`

- Honor DNSSEC policy key tag ranges.

  When a :any:`dnssec-policy` configured a non-default ``tag-range``,
  :iscman:`dnssec-keygen` and :iscman:`dnssec-ksr` could accept
  generated keys outside that range. Both tools now honor the configured
  minimum and maximum key tags. :gl:`#6091`

- Fix a double free in :iscman:`mdig` when EDNS options are specified.

  The :iscman:`mdig` utility could terminate with a double free when
  EDNS options were specified on the command line. This has been fixed.
  :gl:`#6095`

- Fix a crash when an IXFR falls back to AXFR with updates still
  pending.

  When a secondary zone received an incremental transfer (IXFR) and the
  primary then caused :iscman:`named` to fall back to a full transfer
  (AXFR) while some of the already-received incremental changes were
  still waiting to be applied, :iscman:`named` could later crash when
  that transfer finished. The pending changes are now discarded
  correctly before the AXFR retry. :gl:`#6114`

- Fix DS requests to parental agents over TLS.

  TLS configuration for parental agents was being ignored when sending
  DS requests. This has been fixed. :gl:`#6135`

- Fix the :option:`rndc-confgen -q` (quiet) option.

  The command-line parsing in :iscman:`rndc-confgen` was broken, so
  :option:`rndc-confgen -q` did not work. This has been fixed.
  :gl:`#6187`

- Enforce query ACLs for redirect zones and searched DLZs.

  Queries answered from redirect zones or searched DLZ databases did not
  consistently honor :any:`allow-query` and :any:`allow-query-on`,
  potentially exposing restricted DNS data to excluded clients or
  through excluded listening addresses. These ACLs are now enforced
  before redirect or DLZ data is returned. :gl:`#6251` :gl:`#6252`

- Check ``asnum`` validity in GeoIP ACLs.

  The validity of autonomous system (AS) numbers is now checked at
  configuration time when parsing GeoIP ACLs that use ``asnum``
  elements.

  ``asnum`` values start with an optional case-insensitive ``AS``
  prefix, followed only by decimal digits, with no spaces or other
  extraneous characters. The value represented cannot exceed 2^32.
  :gl:`#6255`

- Fix a crash on :any:`remote-servers` lists that reference themselves.

  Since 9.21.16 and 9.20.17, a :any:`remote-servers`, :any:`primaries`,
  ``masters``, or :any:`parental-agents` list that referenced itself,
  directly or through another list, made :iscman:`named` crash on
  startup or reconfiguration. Such references are again skipped and the
  remaining entries in the list are used, as in earlier versions.
  :gl:`#6287`

- A record from outside a response policy zone could crash
  :iscman:`named`.

  A response policy zone transferred from a primary can contain a record
  whose name lies outside the zone. Such a record could terminate
  :iscman:`named` with an assertion failure, both when it arrived and
  again at every startup afterwards, because a secondary keeps it in its
  own copy of the zone. Records like this are now rejected and logged;
  previously one could also silently create a policy entry for an
  unrelated name. :gl:`#6304`

- Invalid :any:`key-store` configuration could abort the DNSSEC tools.

  An invalid :any:`key-store` block named ``key-directory`` in a
  configuration file could abort the DNSSEC tools. This has been fixed.
  :gl:`#6313`

- NSEC signature set could bypass the secure-delegation check.

  When proving that a delegation was insecure, the validator bounded an
  NSEC record's authority by the signer of whichever RRSIG happened to
  come first in the record's signature set, rather than the signature
  that actually verified. A grandparent NSEC padded with an extra,
  unverifiable signature could therefore pass the check that kept such
  proofs from reaching below a signed child zone. The validator now
  requires every signature on the NSEC to name the same signer and
  refuses proofs whose signature set is malformed or larger than
  :any:`max-validations-per-fetch` allows. :gl:`#6321`

- Fix a possible :iscman:`nsupdate` issue when using GSS-TSIG.

  The :iscman:`nsupdate` process could terminate unexpectedly when using
  the GSS-TSIG mode executed with the :option:`nsupdate -g` option. This
  has been fixed. :gl:`#6325`

- Fix a crash with a single-element ``geoip`` sortlist.

  If :iscman:`named` was configured with a single-element
  :any:`sortlist` containing a ``geoip`` ACL element, any matching query
  triggered an assertion failure. This has been fixed. :gl:`#6342`

- Prevent out-of-bailiwick CNAMEs from evicting cached records.

  A recursive resolver could remove valid cached records when a DNS
  response contained an out-of-bailiwick CNAME with the same owner name.
  Out-of-bailiwick data is now discarded before it can modify the cache.
  :gl:`#6345`

- Restore periodic cleanup of stale resolver address data.

  Stale resolver address data could remain cached until memory pressure
  or an explicit flush. The cleanup interval has been corrected, so
  stale entries are removed periodically again. :gl:`#6346`

- Fix :iscman:`named-checkconf`/:iscman:`named` crash with malformed
  key name.

  When a primary/remote-server key name was malformed,
  :iscman:`named-checkconf` and :iscman:`named` were both crashing
  (after warning about the invalid key name). This is now fixed.
  :gl:`#6362`

- Prevent resolver crashes while processing DNS over TCP.

  Recursive resolvers could terminate with an assertion failure while
  processing DNS responses over TCP under sustained traffic. The failure
  was observed on resolvers configured globally with ``forward only;``,
  but the same transport path is also used by iterative resolution.
  This has been fixed. :gl:`!12537`
