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

BIND 9.20.27
------------

New Features
~~~~~~~~~~~~

- Disclose active Negative Trust Anchors with Extended DNS Error 33.
  ``566e7018278``

  A Negative Trust Anchor (RFC 7646) turns off DNSSEC validation for a
  domain, so a name that would normally fail validation resolves
  instead. named now marks such answers with Extended DNS Error code 33,
  "Negative Trust Anchor", so operators can see at a glance when a
  response came back only because an NTA was in effect. :gl:`#6268`
  :gl:`!12426`

- Add development guidance for AI coding agents under .agents/skills/
  ``ef651c93709``

  This adds a set of skill documents that give AI coding agents the
  project's established practices up front instead of having them
  rediscovered (or gotten wrong) in every session: the canonical build
  and test invocations, the memory-allocator contract, the disciplines
  for RCU mutation, per-loop sharded structures, struct-layout work, and
  flight-recorder debugging of concurrency bugs, plus the commit and
  merge-request conventions. :gl:`!12401`

- Add more unit tests for isc_time API. ``9d6855ef735``

  While working on internal 64-bit time for BIND 9, the unit test suite
  for isc_time API has been extended.

  Backport the unit tests from 64-bit time branch to the main branch.
  This should make the unit tests for isc_time API (mostly) complete.

  Related to #2959 :gl:`!12432`

Feature Changes
~~~~~~~~~~~~~~~

- Batch qp transaction for RPZ updates. ``cb40fb91fa2``

  RPZ was built around fine-grained locking, but that forces the use of
  many small qp transactions. With this MR, we switch qp transaction to
  handle the full update to the rpz summary structure. While this
  serializes the RPZ updates, the reduced overhead from batching qp
  transactions more than compensates for it and results in improvements
  for big RPZ zones. :gl:`#5787`, #6270 :gl:`!12493`

- Pass the work callback result to the done callback. ``65cdb7c7613``

  The `isc_work` callback now returns `isc_result_t` and the value is
  handed to the done callback, so the callers no longer need their own
  result-passing state. :gl:`!12391`

Bug Fixes
~~~~~~~~~

- Dig +yaml producing invalid YAML when a lookup fails. ``65d1776959e``

  When "dig +yaml" was run and no server could be reached, dig printed
  its plain-text startup banner (the "; \<\<\>\> DiG ..." and ";; global
  options" lines) ahead of the machine-readable output, so the result
  was not valid YAML and could not be parsed. dig no longer emits that
  banner in YAML mode. As part of the same change, the banner is now
  built only after the whole command line has been read, so options
  given after the query name (such as +nocmd, +short and +yaml) are
  correctly reflected in it. :gl:`#1230` :gl:`!12430`

- Ensure NSEC authority does not cross zonecut boundary. ``0baaddd096a``

  When using a cached NSEC record to prove that a delegation is
  insecure, we now check that the signer name in the corresponding RRSIG
  is not above a known secure delegation point. This prevents a signed
  namespace from being downgraded to insecure using an NSEC record from
  the grandparent zone. :gl:`#5967` :gl:`!12394`

- Treat non canonical RPZ prefixes as any other failure. ``17f0828b460``

  RPZ prefixes that were not encoded in canonical form do not work.
  Treat them as any other encoding error. :gl:`#6043` :gl:`!12482`

- Properly prevent TSIG generation command line injection attacks.
  ``f777451d261``

  When key names are generated with `rndc-confgen`, `tsig-keygen` and
  `ddns-confgen`, special characters must be escaped to ensure the
  configuration is parsed correctly. :gl:`#6071` :gl:`!12396`

- Dig with IDN output could leak memory on ISC_R_NOSPACE  retry.
  ``c4e53b59b2e``

  The IDN to text display call back could leak the memory holding the
  converted name if it did not fit into the buffer.  This has been
  fixed. :gl:`#6073` :gl:`!12483`

- Dnssec-signzone had a potential heap bounds overflow write.
  ``994c3bd4323``

  It was possible for `dnssec-signzone` to overflow array bounds while
  signing.  This has been fixed. :gl:`#6076` :gl:`!12503`

- Restore SMF support on Solaris and illumos. ``8bde9318f63``

  SMF support on Solaris and illumos was silently dropped by a build
  system rewrite in 2018; it is now detected and enabled again.
  :gl:`#6096` :gl:`!12456`

- Fix NULL pointer dereference in dnstap-read. ``9c459b1854d``

  It was possible to dereference a NULL pointer in dnstap-read causing
  it to exit on a malformed DNSTAP file.  This has been fixed.
  :gl:`#6124` :gl:`!12494`

- Change catz coo locking. ``78265c8f6fc``

  Catalog zones might need to inspect the change-of-ownership records of
  other catalog zones, which required to release the lock in the middle
  of certain operations, leading to possible race conditions.

  Since the operations on change-of-ownership records are limited, we
  can instead use a design with a second lock protecting the
  change-of-ownership records on read. We structure the API so that
  holding two change-of-ownership locks at the same time is impossible.
  :gl:`#6131` :gl:`!12410`

- Treat an unusable NSEC3 chain as a verification failure.
  ``879a383ef16``

  When transferring in a mirror zone, DNSSEC verification could
  incorrectly succeed when the zone had an invalid `NSEC3PARAM` record,
  leading to subsequent validation failures. This has been fixed.
  :gl:`#6136` :gl:`!12431`

- Unterminated OpenSSL private-key `Label:` field can be read past its
  parser buffer. ``932581b74bd``

  Check that the string encoded in the Label: field of the .private file
  of a key pair is NUL terminated and the correct length.  Reject the
  .private file if it is not. :gl:`#6193` :gl:`!12417`

- Negative caching stopped working with stale-answer-client-timeout 0.
  ``2687d16ed2f``

  With "stale-answer-client-timeout 0" configured, every client query
  for a name cached as NXDOMAIN or NODATA was sent on to the
  authoritative servers, even while the cached negative answer was still
  within its TTL, so the resolver effectively lost negative caching.
  Negative answers are now refreshed only once they have actually gone
  stale. :gl:`#6245` :gl:`!12384`

- MacOS byte swapping macros already defined. ``5c1d3df49c0``

  Don't redefine them if the development environment already defines
  them. :gl:`#6250` :gl:`!12406`

- Use memmove in isc_sockaddr_fromin/isc_sockaddr_fromin6.
  ``3f3c82292c9``

  Use memmove instead of direct assignment from the source pointer
  because the source pointer is not guaranteed to be correctly aligned.
  :gl:`#6260` :gl:`!12449`

- Fix compilation on GNU/Hurd. ``20f5580e017``

  Fix compilation issues on GNU/Hurd. :gl:`#6285` :gl:`!12496`

- Restore arc4random() detection dropped in the v9.21.14 merge.
  ``86ad6866eeb``

  Commit 4db9e5d90e2 ("Use arc4random for CSPRNG when available", part
  of the CVE-2025-40780 fix) guarded the arc4random() code paths in
  lib/isc/random.h and lib/isc/random.c with HAVE_ARC4RANDOM and added
  the corresponding function check to meson.build.  The manual conflict
  resolution in merge c2a672bbaef ("Merge tag 'v9.21.14'") kept the code
  changes but dropped the meson.build hunk, so HAVE_ARC4RANDOM was never
  defined and platforms with arc4random() (macOS and the BSDs) silently
  fell back to the internal ChaCha-based CSPRNG.  Restore the check.

  Assisted-by: Claude:claude-fable-5 :gl:`!12452`


