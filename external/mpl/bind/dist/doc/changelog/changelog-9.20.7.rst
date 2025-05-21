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

BIND 9.20.7
-----------

New Features
~~~~~~~~~~~~

- Implement the min-transfer-rate-in configuration option.
  ``4a5a9c8256``

  A new option 'min-transfer-rate-in <bytes> <minutes>' has been added
  to the view and zone configurations. It can abort incoming zone
  transfers which run very slowly due to network related issues, for
  example. The default value is set to 10240 bytes in 5 minutes.
  :gl:`#3914` :gl:`!10137`

- Add digest methods for SIG and RRSIG. ``6d8c513986``

  ZONEMD digests RRSIG records and potentially digests SIG record. Add
  digests methods for both record types. :gl:`#5219` :gl:`!10218`

- Add HTTPS record query to host command line tool. ``2ddfb57b45``

  The host command was extended to also query for the HTTPS RR type by
  default. :gl:`!10123`

Bug Fixes
~~~~~~~~~

- Prevent a reference leak when using plugins. ``0201e3eacb``

  The `NS_QUERY_DONE_BEGIN` and `NS_QUERY_DONE_SEND` plugin hooks could
  cause a reference leak if they returned `NS_HOOK_RETURN` without
  cleaning up the query context properly. :gl:`#2094` :gl:`!10170`

- Fix isc_quota bug. ``dbc635c148``

  Running jobs which were entered into the isc_quota queue is the
  responsibility of the isc_quota_release() function, which, when
  releasing a previously acquired quota, checks whether the queue is
  empty, and if it's not, it runs a job from the queue without touching
  the 'quota->used' counter. This mechanism is susceptible to a possible
  hangup of a newly queued job in case when between the time a decision
  has been made to queue it (because used >= max) and the time it was
  actually queued, the last quota was released. Since there is no more
  quotas to be released (unless arriving in the future), the newly
  entered job will be stuck in the queue.

  Fix the issue by adding checks in both isc_quota_release() and
  isc_quota_acquire_cb() to make sure that the described hangup does not
  happen. Also see code comments. :gl:`#4965` :gl:`!10139`

- Fix dual-stack-servers configuration option. ``a47dab2c5e``

  The dual-stack-servers configuration option was not working as
  expected; the specified servers were not being used when they should
  have been, leading to resolution failures. This has been fixed.
  :gl:`#5019` :gl:`!10174`

- Implement sig0key-checks-limit and sig0message-checks-limit.
  ``95af81b674``

  Previously a hard-coded limitation of maximum two key or message
  verification checks were introduced when checking the message's SIG(0)
  signature. It was done in order to protect against possible DoS
  attacks. The logic behind choosing the number 2 was that more than a
  single key should only be required during key rotations, and in that
  case two keys are enough. But later it became apparent that there are
  other use cases too where even more keys are required, see issue
  number #5050 in GitLab.

  This change introduces two new configuration options for the views,
  `sig0key-checks-limit` and `sig0message-checks-limit`, which define
  how many keys are allowed to be checked to find a matching key, and
  how many message verifications are allowed to take place once a
  matching key has been found. The latter protects against expensive
  cryptographic operations when there are keys with colliding tags and
  algorithm numbers, with default being 2, and the former protects
  against a bit less expensive key parsing operations and defaults to
  16. :gl:`#5050` :gl:`!10141`

- Fix the data race causing a permanent active client increase.
  ``20cf51dfc5``

  Previously, a data race could cause a newly created fetch context for
  a new client to be used before it had been fully initialized, which
  would cause the query to become stuck; queries for the same data would
  be either paused indefinitely or dropped because of the
  `clients-per-query` limit. This has been fixed. :gl:`#5053`
  :gl:`!10147`

- Fix deferred validation of unsigned DS and DNSKEY records.
  ``ba5fe2dd12``

  When processing a query with the "checking disabled" bit set (CD=1),
  `named` stores the unvalidated result in the cache, marked "pending".
  When the same query is sent with CD=0, the cached data is validated,
  and either accepted as an answer, or ejected from the cache as
  invalid. This deferred validation was not attempted for DS and DNSKEY
  records if they had no cached signatures, causing spurious validation
  failures. We now complete the deferred validation in this scenario.

  Also, if deferred validation fails, we now re-query the data to find
  out whether the zone has been corrected since the invalid data was
  cached. :gl:`#5066` :gl:`!10105`

- When recording an rr trace, use libtool. ``17ca2fbbdc``

  When a system test is run with the `USE_RR` environment variable set
  to 1, an `rr` trace is now correctly generated for each instance of
  `named`. :gl:`#5079` :gl:`!10207`

- Do not cache signatures for rejected data. ``9b3e1facf6``

  The cache has been updated so that if new data is rejected - for
  example, because there was already existing data at a higher trust
  level - then its covering RRSIG will also be rejected. :gl:`#5132`
  :gl:`!10134`

- Fix RPZ race condition during a reconfiguration. ``eca9a3279e``

  With RPZ in use, `named` could terminate unexpectedly because of a
  race condition when a reconfiguration command was received using
  `rndc`. This has been fixed. :gl:`#5146` :gl:`!10144`

- "CNAME and other data check" not applied to all types. ``a68f5dd74b``

  An incorrect optimization caused "CNAME and other data" errors not to
  be detected if certain types were at the same node as a CNAME.  This
  has been fixed. :gl:`#5150` :gl:`!10100`

- Relax private DNSKEY and RRSIG constraints. ``455080866c``

  DNSKEY, KEY, RRSIG and SIG constraints have been relaxed to allow
  empty key and signature material after the algorithm identifier for
  PRIVATEOID and PRIVATEDNS. It is arguable whether this falls within
  the expected use of these types as no key material is shared and the
  signatures are ineffective but these are private algorithms and they
  can be totally insecure. :gl:`#5167` :gl:`!10173`

- Delete dead nodes when committing a new version. ``0682684028``

  In the qpzone implementation of `dns_db_closeversion()`, if there are
  changed nodes that have no remaining data, delete them. :gl:`#5169`
  :gl:`!10124`

- Revert "Delete dead nodes when committing a new version"
  ``d2ec6d1db4``

  This reverts commit 67255da4b376f65138b299dcd5eb6a3b7f9735a9,
  reversing changes made to 74c9ff384e695d1b27fa365d1fee84576f869d4c.
  :gl:`#5169` :gl:`!10226`

- Fix dns_qp_insert() checks in qpzone. ``11cc40ebf6``

  Remove code in the QP zone database to handle failures of
  `dns_qp_insert()` which can't actually happen. :gl:`#5171`
  :gl:`!10114`

- Remove NSEC/DS/NSEC3 RRSIG check from dns_message_parse.
  ``b752db0c3f``

  Previously, when parsing responses, named incorrectly rejected
  responses without matching RRSIG records for NSEC/DS/NSEC3 records in
  the authority section. This rejection, if appropriate, should have
  been left for the validator to determine and has been fixed.
  :gl:`#5185` :gl:`!10142`

- Fix TTL issue with ANY queries processed through RPZ "passthru"
  ``b1bf17096a``

  Answers to an "ANY" query which were processed by the RPZ "passthru"
  policy had the response-policy's `max-policy-ttl` value unexpectedly
  applied. This has been fixed. :gl:`#5187` :gl:`!10180`

- Dnssec-signzone needs to check for a NULL key when setting offline.
  ``2d4b4fe15e``

  dnssec-signzone could dereference a NULL key pointer when resigning a
  zone.  This has been fixed. :gl:`#5192` :gl:`!10169`

- Acquire the database reference before possibly last node release.
  ``2b5b4e9dd1``

  Acquire the database reference in the detachnode() to prevent the last
  reference to be release while the NODE_LOCK being locked.  The
  NODE_LOCK is locked/unlocked inside the RCU critical section, thus it
  is most probably this should not pose a problem as the database uses
  call_rcu memory reclamation, but this it is still safer to acquire the
  reference before releasing the node. :gl:`#5194` :gl:`!10156`

- Fix a logic error in cache_name() ``b8bd65763c``

  A change in 6aba56ae8 (checking whether a rejected RRset was identical
  to the data it would have replaced, so that we could still cache a
  signature) inadvertently introduced cases where processing of a
  response would continue when previously it would have been skipped.
  :gl:`#5197` :gl:`!10158`

- Fix a bug in the statistics channel when querying zone transfers
  information. ``b50d9b601d``

  When querying zone transfers information from the statistics channel
  there was a rare possibility that `named` could terminate unexpectedly
  if a zone transfer was in a state when transferring from all the
  available primary servers had failed earlier. This has been fixed.
  :gl:`#5198` :gl:`!10194`

- Fix assertion failure when dumping recursing clients. ``5d913c3383``

  Previously, if a new counter was added to the hashtable while dumping
  recursing clients via the `rndc recursing` command, and
  `fetches-per-zone` was enabled, an assertion failure could occur. This
  has been fixed. :gl:`#5200` :gl:`!10168`

- Call isc__iterated_hash_initialize in isc__work_cb. ``693a1d41ed``

  isc_iterated_hash didn't work in offloaded threads as the per thread
  initialisation has not been done.  This has been fixed. :gl:`#5214`
  :gl:`!10210`

- Fix a bug in get_request_transport_type() ``aa3c6584c6``

  When `dns_remote_done()` is true, calling `dns_remote_curraddr()`
  asserts. Add a `dns_remote_curraddr()` check before calling
  `dns_remote_curraddr()`. :gl:`#5215` :gl:`!10223`

- Dump the active resolver fetches from dns_resolver_dumpfetches()
  ``b2033b7e4c``

  Previously, active resolver fetches were only dumped when the
  `fetches-per-zone` configuration option was enabled. Now, active
  resolver fetches are dumped along with the number of
  `clients-per-server` counters per resolver fetch. :gl:`!10148`

- Fix wrong logging severity in do_nsfetch() ``fd623c6ecc``

  :gl:`!10118`

- Post [CVE-2024-12705] Performance Drop Fixes, Part 2. ``8cc425a5bb``

  This merge request addresses several key performance bottlenecks in
  the DoH (DNS over HTTPS) implementation by introducing significant
  optimizations and improvements.

  ### Key Improvements

  1. **Simplification and Optimisation of `http_do_bio()` Function**:
  - The code flow in the `http_do_bio()` function has been significantly
  simplified. 2. **Flushing HTTP Write Buffer on Outgoing DNS
  Messages**:    - The buffer is flushed and a send operation is
  performed when there is an outgoing DNS message. 3. **Bumping Active
  Streams Processing Limit**:    - The total number of active streams
  has been increased to 60% of the total streams limit.

  These changes collectively enhance the performance and reliability of
  the DoH implementation, making it more efficient and robust for
  handling high-load scenarios, particularly noticeable in long runs (>=
  1h) of `stress:long:rpz:doh+udp:linux:*` tests. It improves perf. for
  tests for BIND 9.18, but it likely will have a positive but less
  pronounced effect on newer versions as well.

  In essence, the merge request fixes three bottlenecks stacked upon
  each other.

  *It is a logical continuation of the merge requests !10109.* !10109,
  unfortunately, did not completely [address the performance drop in
  9.18](https://gitlab.isc.org/isc-projects/bind9/-/pipelines/221545)
  for longer runs of the stress test. This merge request [addresses
  that](https://gitlab.isc.org/isc-projects/bind9/-/pipelines/223661).

  **P.S.**

  The origin of the fixes is, in fact, the branch in !10193. So this MR
  is a ... *forward port* of them. :gl:`!10199`

- Post [CVE-2024-12705] Performance Drop Fixes. ``9d4aa15c1f``

  This merge request fixes a [performance
  drop](https://gitlab.isc.org/isc-projects/bind9/-/pipelines/216728)
  after merging the fixes for #4795, in particular in 9.18.

  The MR [fixes the
  problem](https://gitlab.isc.org/isc-projects/bind9/-/pipelines/219825)
  without affecting performance for the newer versions, in particular
  for [the development version](https://gitlab.isc.org/isc-projects/bind
  9/-/pipelines/220619). :gl:`!10129`

- Sync the TSAN CC, CFLAGS and LDFLAGS in the respdiff:tsan job.
  ``ff58e0ed2b``

  :gl:`!10212`


