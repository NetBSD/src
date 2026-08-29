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

Notes for BIND 9.20.27
----------------------

New Features
~~~~~~~~~~~~

- Disclose active Negative Trust Anchors with Extended DNS Error 33.

  A Negative Trust Anchor (:rfc:`7646`) turns off DNSSEC validation for
  a domain, so a name that would normally fail validation resolves
  instead. :iscman:`named` now marks such answers with Extended DNS
  Error code 33, "Negative Trust Anchor", so operators can see at a
  glance when a response came back only because an NTA was in effect.
  :gl:`#6268`

Feature Changes
~~~~~~~~~~~~~~~

- Speed up RPZ policy zone updates.

  RPZ updates used to be applied one small step at a time, adding
  overhead on large policy zones. Updates are now applied as a single
  batch, improving update performance for large RPZ zones, at the cost
  of no longer overlapping with concurrent updates. :gl:`#5787`
  :gl:`#6270`

Bug Fixes
~~~~~~~~~

- Ensure NSEC authority does not cross zonecut boundary.

  When using a cached NSEC record to prove that a delegation is
  insecure, :iscman:`named` now checks that the signer name in the
  corresponding RRSIG is not above a known secure delegation point.
  This prevents a signed namespace from being downgraded to insecure
  using an NSEC record from the grandparent zone. :gl:`#5967`

- Treat an unusable NSEC3 chain as a verification failure.

  When transferring in a mirror zone, DNSSEC verification could
  incorrectly succeed when the zone had an invalid NSEC3PARAM record,
  leading to subsequent validation failures. This has been fixed.
  :gl:`#6136`

- Treat non-canonical RPZ prefixes as any other failure.

  RPZ prefixes that were not encoded in canonical form did not work.
  They are now handled in the same way as any other encoding error. :gl:`#6043`

- Negative caching stopped working with stale-answer-client-timeout set
  to ``0``.

  Negative answers were re-fetched on every query instead of once they
  actually expired, effectively disabling negative caching. This has
  been fixed. :gl:`#6245`

- An unterminated OpenSSL private-key ``Label:`` field could be read past
  its parser buffer.

  The ``Label:`` field in a ``.private`` key file is now checked for
  length and NUL-termination. Malformed files are rejected. :gl:`#6193`

- Restore SMF support on Solaris and illumos. :gl:`#6096`

- Fix compilation on GNU/Hurd. :gl:`#6285`

- :option:`dig +yaml` was producing invalid YAML when a lookup failed.

  When no server could be reached, :iscman:`dig` printed its
  plain-text startup banner ahead of the YAML output, making the
  result unparsable. :iscman:`dig` no longer does this and correctly
  reflects options such as ``+nocmd``, ``+short`` and ``+yaml``,
  regardless of where they appear on the command line. :gl:`#1230`

- Properly prevent TSIG generation command line injection attacks.

  When key names are generated with :iscman:`rndc-confgen`,
  :iscman:`tsig-keygen` and :iscman:`ddns-confgen`, special characters
  must be escaped to ensure that the configuration is parsed correctly.
  :gl:`#6071`

- Fix a potential heap bounds overflow write in :iscman:`dnssec-signzone`.

  It was possible for :iscman:`dnssec-signzone` to overflow array
  bounds while signing. This has been fixed. :gl:`#6076`

- Fix crashes on invalid DNSTAP input in :iscman:`dnstap-read`.

  Malformed DNSTAP files could trigger a NULL pointer dereference or an
  out-of-bounds memory read in :iscman:`dnstap-read`. This has been
  fixed. :gl:`#6077` :gl:`#6124`
