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

Notes for BIND 9.20.21
----------------------

Security Fixes
~~~~~~~~~~~~~~

- Fix unbounded NSEC3 iterations when validating referrals to unsigned
  delegations. :cve:`2026-1519`

  DNSSEC-signed zones may contain high iteration-count NSEC3 records,
  which prove that certain delegations are insecure. Previously, a
  validating resolver encountering such a delegation processed these
  iterations up to the number given, which could be a maximum of 65,535.
  This has been addressed by introducing a processing limit, set at 50.
  Now, if such an NSEC3 record is encountered, the delegation will be
  treated as insecure.

  ISC would like to thank Samy Medjahed/Ap4sh for bringing this
  vulnerability to our attention. :gl:`#5708`

- Fix memory leaks in code preparing DNSSEC proofs of non-existence.
  :cve:`2026-3104`

  An attacker controlling a DNSSEC-signed zone could trigger a memory
  leak in the logic preparing DNSSEC proofs of non-existence, by
  creating more than :any:`max-records-per-type` RRSIGs for NSEC
  records. These memory leaks have been fixed.

  ISC would like to thank Vitaly Simonovich for bringing this
  vulnerability to our attention. :gl:`#5742`

- Prevent a crash in code processing queries containing a TKEY record.
  :cve:`2026-3119`

  The :iscman:`named` process could terminate unexpectedly when
  processing a correctly signed query containing a TKEY record. This has
  been fixed.

  ISC would like to thank Vitaly Simonovich for bringing this
  vulnerability to our attention. :gl:`#5748`

- Fix a stack use-after-return flaw in SIG(0) handling code.
  :cve:`2026-3591`

  A stack use-after-return flaw in SIG(0) handling code could enable ACL
  bypass and/or assertion failures in certain circumstances. This flaw
  has been fixed.

  ISC would like to thank Mcsky23 for bringing this vulnerability to our
  attention. :gl:`#5754`

Bug Fixes
~~~~~~~~~

- Fix the handling of :namedconf:ref:`key` statements defined inside
  views.

  A recent change introduced in BIND 9.20.17 hardened the
  :namedconf:ref:`key` name check when used in :any:`primaries`, to
  immediately reject the configuration if the key was not defined
  (rather than only checking whether the key name was correctly formed).
  However, that change introduced a regression that prevented the use of
  a :namedconf:ref:`key` defined in a view. This has now been fixed.
  :gl:`#5761`
