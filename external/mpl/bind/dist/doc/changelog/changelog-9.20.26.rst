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

BIND 9.20.26
------------

Security Fixes
~~~~~~~~~~~~~~

- [CVE-2026-11331] Fix handling of rpz CNAME expansion that returns name
  too long. ``b950da5f625``

  Previously, if the expansion of a wildcard CNAME RPZ policy resulted
  in a name that exceeded the length limit, a self referential CNAME and
  the original address record were returned, allowing the policy to be
  bypassed.  In branches up to 9.20, this also left query processing in
  an inconsistent state which could trigger an assertion failure.  We
  now return a YXDOMAIN response, without the address.

  ISC would like to thank Laith Mash'al (0xmshal) for bringing this
  issue to our attention. :gl:`#5856`

- [CVE-2026-11721] Invalid signed wildcard records were being accepted.
  ``249752cd72d``

  Signed wildcard responses in which the Labels field in the `RRSIG`
  record was less than the number of labels in the Signer Name field
  were being incorrectly accepted. This in turn broke
  `synth-from-dnssec`, which depends on such records being correctly
  validated. This has been fixed.

  ISC thanks Qifan Zhang of Palo Alto Networks for bringing this issue
  to our attention. :gl:`#5871`

- [CVE-2026-13321] Fix DNSSEC validation bypass via out-of-zone NSEC
  Next Field. ``6fbc963d4af``

  A malicious zone with out-of-zone NSEC next owner names can cause a
  DNSSEC validating resolver to cache such record and, if
  `synth-from-dnssec` is enabled, to generate negative answers for any
  zone that is covered by the range.

  ISC would like to thank Qifan Zhang of Palo Alto Networks for
  reporting the issue. :gl:`#5873`

- [CVE-2026-10723] Correct verification of NSEC3 signer name.
  ``df3abfc3184``

  BIND 9 accepted child-zone NSEC3 records where the first label equals
  the hash of the parent zone as valid parent-zone closest encloser
  proofs. This has been fixed.

  ISC thanks Qifan Zhang of Palo Alto Networks for reporting the issue.
  :gl:`#5874`

- [CVE-2026-12617] Do no assert for some specifics CNAME and DNAME
  queries. ``d75d1e93958``

  A bug in the resolver's handling of certain cached DNAME and CNAME
  responses could cause named to trigger an assertion failure and exit.
  An attacker controlling a domain name and the authoritative DNS server
  it is hosted on could exploit this behavior to cause a
  denial-of-service. This vulnerability has been fixed.

  ISC thanks Qifan Zhang of Palo Alto Networks for bringing this issue
  to our attention. :gl:`#5946`

- [CVE-2026-10822] Malformed DNSKEY records could trigger an assertion.
  ``a1777ce7ee0``

  Previously, `dns_name_fromwire()` did not honor the record boundary
  when reading names from the wire, allowing malformed records to be
  accepted when they should not have been. In particular, malformed
  DNSKEY records could trigger an assertion failure when being printed.
  This has been fixed. :gl:`#6004`

- Reclaim memory promptly when DNSSEC validations are canceled.
  ``b8f9d7657ac``

  When a resolver is flooded with queries that require DNSSEC validation
  — for example during a random-subdomain attack — many of those
  validations are canceled before they complete. Previously a canceled
  validation still kept its place in the internal work queue and held
  the associated response in memory until that queued work eventually
  ran, so memory could climb sharply under sustained load. The canceled
  work is now dropped as soon as the validation is canceled, releasing
  the memory it was holding.

- [CVE-2026-11605] Prevent excessive validation work from crafted
  negative responses. ``5f1bb0aad09``

  A validating resolver could be made to perform a large amount of
  DNSSEC validation work in response to a single answer, consuming
  excessive CPU. A malicious authoritative server triggers this by
  returning a signed negative answer (NXDOMAIN or NODATA) padded with
  many denial-of-existence proof records, which the resolver continued
  to verify beyond its per-query validation limit. It now enforces that
  limit on negative answers and returns SERVFAIL once the limit is
  reached.

  Closes: https://gitlab.isc.org/isc-projects/bind9/-/work_items/4463

- [CVE-2026-11622] Prevent cache exhaustion under sustained attack.
  ``410aa215eab``

  The cache memory can become exhausted with expired entries whose
  memory is not released due to a sustained attack on the same DNS name
  that prevents the cleanup. This has been fixed.

  Closes: https://gitlab.isc.org/isc-projects/bind9/-/work_items/4760

New Features
~~~~~~~~~~~~

- Print OS platform in "named -V" ``5bb30b1f993``

  The "running on" line emitted by `named -V` (as well as the startup
  log and `rndc status`, which share the same source) now appends the
  PRETTY_NAME value from /etc/os-release in parentheses after the uname
  output, e.g.:

  running on Linux x86_64 6.19.14-... (Fedora Linux 42 (Workstation
  Edition))

  This helps disambiguate environments where the kernel string is not a
  reliable indicator of the userspace, such as RHEL clones and
  containers whose kernel does not match the host OS.

  When /etc/os-release is absent, /usr/lib/os-release is tried as a
  fallback per the systemd os-release(5) specification. When neither is
  available or no PRETTY_NAME is found, the output is unchanged.

  Assisted-by: Claude:claude-opus-4-7 :gl:`#5334` :gl:`!12319`

Removed Features
~~~~~~~~~~~~~~~~

- Remove the secondary validator in query.c. ``1687a3d0b85``

  Previously, when the additional section of a response was being
  populated, if cached data was found with pending trust, it would be
  opportunistically validated. The code implementing this validation was
  not quite formally correct. Rather than fixing it, the code has been
  removed: RRsets with pending trust are now omitted from responses.
  :gl:`#5966` :gl:`#5968` :gl:`#5972` :gl:`!12258`

Feature Changes
~~~~~~~~~~~~~~~

- Rework isc_work as per-loop, per-lane cancelable worker threads.
  ``51d67c173e9``

  Fold the libuv thread pool and the per-loop isc_helper threads into a
  single isc_work pool. Each (loop, lane) gets its own SPSC queue and
  worker, which drops the shared-queue contention, and the FAST/SLOW
  lanes keep short crypto tasks off the long blocking ones (zone
  dump/load, xfrin). isc_work jobs are now cancelable: isc_work_cancel()
  tombstones a still-queued job and its after_cb fires with
  ISC_R_CANCELED, so abandoned work can be dropped instead of run to
  completion. :gl:`!12267`

- Sync picohttpparser.c with upstream commit a875a01. ``7b249a11e34``

  Synced with the h2o/picohttpparser upstream repository up to commit
  f4d94b48b31e0abae029ebeafcfd9ca0680ede58.

  This commit is just hygiene and consistency by keeping the vendored
  copy current. :gl:`!12273`

Bug Fixes
~~~~~~~~~

- Fix a bug in DNS UPDATE processing with inline-signing enabled.
  ``ad4650c78fa``

  In rare cases the :iscman:`named` process could terminate unexpectedly
  when processing authorized DNS UPDATE messages in quick procession
  which are updating a zone with inline-signing enabled. This has been
  fixed. :gl:`#5816` :gl:`!12300`

- Properly detect private records before copying. ``5c153b85e54``

  We were triggering an assertion when trying to copy a private record
  to a buffer for modifying.  Extend the private type detection and copy
  the contents after we have rejected invalid private records.
  :gl:`#5857` :gl:`!12340`

- Tighten  referral DS acceptance. ``ed27fb9524f``

  Named was accepting DS records for sibling zones when it shouldn't
  have.  This has been fixed. :gl:`#5870` :gl:`!12325`

- Don't synthesize negative responses with pending NSEC. ``86b87dfd290``

  If an NSEC record has not yet been validated and is cached with trust
  pending, don't use it to synthesize negative responses. :gl:`#5872`
  :gl:`#5887`  :gl:`#5977` :gl:`!12368`

- Check that an NSEC signer is at or above the name to be validated.
  ``2afca834141``

  Add a check that an NSEC record being used as a proof of nonexistence
  for a given name is not signed by a name lower in the DNS hierarchy
  than the one in question. :gl:`#5876` :gl:`!12324`

- Don't evict DNSSEC-validated cache data on a CD=1 NXDOMAIN.
  ``3e6cef2ac66``

  When a client sent a query with the checking-disabled (CD) bit set and
  the answer was NXDOMAIN, the resolver cached that unvalidated negative
  response and discarded any DNSSEC-validated records it already held
  for the same name, even though the validated data was more
  trustworthy. A single such response - including a forged one - could
  flush validated records from the cache and force the resolver to fetch
  them again. The resolver now checks the trust level of the existing
  data first and leaves the cache unchanged when it is already
  validated. :gl:`#5877` :gl:`!12344`

- Fix a 'deny-answer-aliases' configuration bypass issue.
  ``47d375b9e63``

  It was possible to use a maliciously crafted authoritative zone to
  make :iscman:`named` resolver synthesize a ``DNAME`` "alias" that
  should have been rejected by the configured :any:`deny-answer-aliases`
  option. This has been fixed. :gl:`#5930` :gl:`!12245`

- Reject external referrals from forwarders. ``cdbc5be8b0c``

  Under `forward-first` policy in a forwarding zone BIND could accept NS
  above the forward zone apex from negative responses. This has been
  fixed.

  ISC would like to thank Qifan Zhang, of Palo Alto Networks, for the
  report. :gl:`#5937` :gl:`!12278`

- Fix a zone transfer over TLS (XoT) issue when using the opportunistic
  TLS mode. ``40b64d13862``

  The :iscman:`named` process, running as secondary DNS server,
  configured to transfer a zone from a primary server using an encrypted
  XoT transport in opportunistic TLS mode (i.e. without peer
  certificate/hostname validation) could terminate unexpectedly when the
  TLS ALPN negotiation with primary server was unsuccessful. This has
  been fixed. :gl:`#5957` :gl:`!12242`

- Unvalidated opt-out NSEC3 could be accepted in insecurity proof.
  ``c3733e12f9e``

  When determining whether an insecure delegation is legitimate, NSEC3
  opt-out records which had not yet passed validation could be used.
  This has been fixed. :gl:`#5970` :gl:`!12358`

- Check wildcard signer and NOQNAME signer match. ``3621c8169bf``

  A positive wildcard answer, and the NSEC3 proof that the requested
  name doesn't exist in the zone, must both be from the same zone.
  Otherwise, an NSEC3 from an ancestor zone could be used to interfere
  with validation.

  We now retrieve the signer name from a wildcard response's signature.
  An NSEC3 record cannot be used as a NOQNAME proof for the wildcard
  unless it exactly matches the name one level above the NSEC3.
  :gl:`#5971` :gl:`!12279`

- Require secure trust for covering NSEC in the RBT cache.
  ``b6bfa444e5b``

  The guard against synthesizing negative answers from a pending NSEC
  (#5977) was only added to the QP cache's find_coveringnsec(); the RBT
  cache database kept binding any covering NSEC regardless of trust.  On
  builds configured with --with-cachedb=rbt this lets a piggy-backed,
  unvalidated NSEC drive a synthesized NXDOMAIN, reviving the cache
  poisoning that was fixed for the default cache.

  query_coveringnsec() only verifies the covering NSEC's trust on its
  NODATA path; the NXDOMAIN path relies on the database returning a
  secure record.  Require the NSEC and its RRSIG to be dns_trust_secure
  in find_coveringnsec(), matching the QP cache. :gl:`#5977`
  :gl:`!12374`

- Check dns_rdata_fromstruct() return values. ``416b0817d1b``

  In some functions implementing RFC 5011 key maintenance, the results
  of `dns_rdata_fromstruct()` were not checked. This has been fixed.
  :gl:`#5982` :gl:`!12183`

- Fix CNAME resolution failure caused by a cached SERVFAIL response.
  ``b1e0a58f4f2``

  Under certain circumstances, a cached SERVFAIL response could
  incorrectly prevent successful resolution of a CNAME target. This
  could cause resolution failures to persist until the cached SERVFAIL
  entry expired, even when the CNAME target itself was otherwise
  resolvable. This issue has been fixed. :gl:`#5983` :gl:`!12259`

- [CVE-2026-13204] Prevent crash from malformed NSEC/NSEC3 response.
  ``ad393b09232``

  An assertion could be triggered by an improperly signed NOQNAME proof.
  This has been fixed.

  ISC thanks Qifan Zhang of Palo Alto Networks for reporting the issue.
  :gl:`#5985`

- Reject unsupported RSA DNSKEY shapes during DNSSEC validation.
  ``ea91570492c``

  An authoritative server publishing an RSA DNSKEY with an unusually
  large modulus or an exotic public exponent could make each DNSSEC
  signature check on a validating recursive resolver noticeably more
  expensive than for a normally sized key.  Such DNSKEYs are now treated
  as invalid. :gl:`#6008` :gl:`!12207`

- Fix a bug in GeoIP2 string matching. ``f8f5e6a9d77``

  When using GeoIP2 ACLs (see :any:`acl`), :iscman:`named` could
  incorrectly match a name using a sub-string instead of the full name
  match. This has been fixed. :gl:`#6019` :gl:`!12235`

- Fix DNS-over-HTTPS (DoH) quota configuration issue. ``72d538fe269``

  The :any:`http-listener-clients` and
  :any:`http-streams-per-connection` configuration options could be
  truncated to smaller values (or to ``0``, which means unlimited) when
  very big configuration values were used, which exceeded ``65535``. As
  a note - it is very unlikely that such big values are used in
  production, and the default values for the affected options are
  ``300`` and ``100``, correspondingly. This has been fixed. :gl:`#6021`
  :gl:`!12231`

- Fix invalid pointer release in JSON statistics-channel response.
  ``5fe742e3fd1``

  Each response served on a JSON statistics endpoint released the wrong
  pointer to the JSON library after the response was sent: the response
  body string instead of the JSON document.  With the current responses
  this does not crash named in practice, but the call is incorrect and
  can in principle corrupt memory.  XML responses are not affected.
  :gl:`#6024` :gl:`!12274`

- Truncated reply to a TSIG query no longer stalls the resolver.
  ``c922cabd46d``

  When an upstream server returned a truncated reply to a query that
  BIND had signed with TSIG, the resolver could keep waiting for a
  follow-up UDP packet that never arrived, so the query stalled until it
  hit resolver-query-timeout and the client received no answer. BIND now
  treats any reply it cannot authenticate as an immediate failure and
  returns SERVFAIL right away as a defense in depth. :gl:`#6028`
  :gl:`!12310`

- Ignore updates removing DNSKEY RRset with class ANY. ``6a533ec622c``

  When a Dynamic Update is received that removes the ``DNSKEY`` (or
  ``CDNSKEY``, or ``CDS``) RRset, remove all records except the ones
  that are in use for signing for the zone. :gl:`#6045` :gl:`!12230`

- Fix a memory leak when updating a zone with more than 32 DNSSEC keys.
  ``73440816e41``

  Applying changes to a signed zone — via DNS UPDATE or the
  inline-signing raw-to-secure sync — leaked the surplus keys when the
  zone's key directory held more than 32, slowly growing named's memory
  use. :gl:`#6051` :gl:`!12332`

- Fix the memory ordering in the adaptive read-write lock.
  ``eff9a306d88``

  On hardware with a weak memory model, the internal read-write lock
  could briefly admit a reader and a writer at the same time, risking
  sporadic crashes or incorrect data. The reader/writer handshake now
  uses sequentially consistent ordering so the two can no longer
  overlap. :gl:`#6060` :gl:`!12205`

- Print the full OID in PRIVATEOID key comments. ``87397c823dc``

  The OID in the "; alg = ..." comment of a PRIVATEOID key was truncated
  to sixteen characters. :gl:`#6092` :gl:`!12378`

- Correct locator decoding for NID, L64, and L32 records.
  ``8a8cc34a504``

  NID, L64, and L32 records were decoded incorrectly when converted into
  their parsed structures, because the preference field was not skipped
  before the locator. :gl:`#6097` :gl:`!12349`

- Fix a possible crash when cleaning up a view's caches. ``51d7456399d``

  In rare cases named could crash while a view was being removed, for
  example during reconfiguration or shutdown, as its internal caches
  were torn down. This has been fixed. :gl:`#6119` :gl:`!12190`

- Preserve the request message across async SIG(0) processing.
  ``d6a4aa5ce50``

  For SIG(0)-signed requests, view matching is offloaded and the request
  is finished asynchronously from ns_client_request_continue(), which
  passes client->inner.buffer to dns_dt_send().  That buffer aliases the
  network manager's receive buffer, only valid during the read callback,
  so it may already be freed and reused, producing garbage dnstap frames
  (e.g. the "upforwd" sig0-over-DoT test fails with UQ=0).

  Copy the request message when entering async mode and reference the
  copy, freeing it in ns__client_reset_cb().

  Assisted-by: Claude:claude-opus-4-8 :gl:`#6139` :gl:`!12200`

- Ignore 0-byte reads in the TCP read callback. ``8588ffec7de``

  Callbacks for libuv stream reads do not signal zero-length reads as a
  failure signal but rather as EAGAIN/EWOULDBLOCK. This can trigger an
  assertion when a zero-length read is pushed onto a PROXYv2 endpoint
  that has not yet processed the headers as it expects a non-NULL region
  of positive length. :gl:`#6140` :gl:`!12291`

- Include the IPv6 address's brackets in the parsed URL/URI host.
  ``9baad48be06``

  The brackets are not included in the host component of the parsed
  URL/URI. Change the parser to include the brackets. :gl:`#6147`
  :gl:`!12362`

- Improve the input validation of the isc_url_parse() function.
  ``f4c8f08042a``

  The isc_url_parse() function failed to check the input buffer's length
  and assumed that it can't be bigger than UINT16_MAX, because both the
  'off' and 'len' fields of the 'isc_url_parser_t' structure are
  uint16_t.

  Add a check to not accept a buffer longer than 8192 octets.
  :gl:`#6150` :gl:`!12361`

- Only print per zone glue stats when query statistics is full.
  ``097e530348f``

  The code printing query statistics was ignoring the zone-statistics
  option. This has been fixed. :gl:`#6164` :gl:`!12263`

- CDS/CDNSKEY records were not removed when re-configuring the server.
  ``20db02121f7``

  When on an ``rndc reconfig`` the DNSSEC policy changes such that it
  changes the expected ``CDNSKEY`` and/or ``CDS`` records in the zone,
  the RRset should be updated accordingly. This did not happen when
  removing digests from the configuration, or setting `cdnskey no;`.
  This has been fixed. :gl:`#6166` :gl:`!12284`

- Fix a crash when querying an empty non-terminal in a wildcard zone in
  RBTDB. ``b8e5801ec5f``

  A query for an empty non-terminal in a wildcard zone served from the
  RBT zone database could abort named with an assertion failure.  It now
  returns the correct NODATA answer. :gl:`#6170` :gl:`!12299`

- Stop reusing outgoing TCP connections the peer has already closed.
  ``fb0ae8a5d6b``

  ``named`` could hand a new query an idle forwarder/upstream TCP or TLS
  connection that the peer had already closed, causing the query to fail
  (and CLOSE-WAIT sockets to pile up). Idle reused connections are now
  watched, so a close is noticed and the connection is dropped instead
  of reused. A new ``tcp-reuse-timeout`` option controls how long an
  idle outgoing connection is kept open for reuse (default 5 seconds).
  :gl:`#6171` :gl:`!12317`

- Fix DNSSEC validation failures for names under an apex DNAME.
  ``cd006ecf235``

  DNSSEC validation could fail with SERVFAIL for names covered by a
  DNAME at the apex of a signed zone, unless the zone's keys were
  already validated in the cache. This regression was introduced by the
  recent fix for resolver stalls on CNAME responses to DS queries.
  :gl:`#6176` :gl:`!12356`

- Avoid needless queries when the DNSSEC validation limit is reached.
  ``45a3e5b8561``

  A validating resolver could send one needless upstream query after
  reaching its per-fetch DNSSEC validation limit. It now stops
  immediately.

- Avoid writing through a const pointer in render_xsl() ``348e5a9ee54``

  render_xsl() served the static XSL stylesheet by casting away the
  const qualifier of xslmsg and handing the pointer to
  isc_buffer_reinit():

  p = UNCONST(xslmsg);     isc_buffer_reinit(b, p, strlen(xslmsg));

  isc_buffer_reinit() copies any pre-existing buffer content into the
  new base with memmove(), so the call would write into xslmsg, which is
  a 'const char[]' living in read-only memory.  This is safe today only
  because the supplied bodybuffer is always freshly initialized with
  length 0, so the memmove() never runs -- a fragile,
  action-at-a-distance invariant that GCC's -fanalyzer flags as a write
  to a const object (-Wanalyzer-write-to-const).

  Use isc_buffer_constinit(), the primitive intended for pointing a
  buffer at constant data: it goes through isc_buffer_init() and never
  writes to the base.  This drops the UNCONST cast, keeps xslmsg in
  read-only memory, and silences the analyzer warning.

  Assisted-by: Claude:claude-opus-4-8 :gl:`!12309`

- Don't serve a stale CNAME or record when fresh data of the other
  exists. ``1cf6838c271``

  When a cached name held both a CNAME and records of another type — one
  stale, the other still fresh — named with serve-stale could return the
  expired set instead of the fresh one, in either direction. It now
  prefers whichever is fresh. :gl:`!12305`

- Fix compatibility with OpenSSL 1.0.2u. ``11fcd8206ab``

  BIND failed to build with OpenSSL 1.0.2u. Add the missing header file
  includes. :gl:`!11897`

- Only update the global tid_count once. ``746d9bfd3a8``

  Skip updating the `tid_count` value on repeated calls to prevent
  ThreadSanitizer 'data race'. :gl:`!12188`


