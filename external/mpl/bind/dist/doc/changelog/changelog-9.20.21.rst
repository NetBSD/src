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

BIND 9.20.21
------------

Security Fixes
~~~~~~~~~~~~~~

- [CVE-2026-1519] Fix unbounded NSEC3 iterations when validating
  referrals to unsigned delegations. ``5af03a06066``

  DNSSEC-signed zones may contain high iteration-count NSEC3 records,
  which prove that certain delegations are insecure. Previously, a
  validating resolver encountering such a delegation processed these
  iterations up to the number given, which could be a maximum of 65,535.
  This has been addressed by introducing a processing limit, set at 50.
  Now, if such an NSEC3 record is encountered, the delegation will be
  treated as insecure.

  ISC would like to thank Samy Medjahed/Ap4sh for bringing this
  vulnerability to our attention. :gl:`#5708`

- [CVE-2026-3104] Fix memory leaks in code preparing DNSSEC proofs of
  non-existence. ``13215b9cbbf``

  An attacker controlling a DNSSEC-signed zone could trigger a memory
  leak in the logic preparing DNSSEC proofs of non-existence, by
  creating more than :any:`max-records-per-type` RRSIGs for NSEC
  records. These memory leaks have been fixed.

  ISC would like to thank Vitaly Simonovich for bringing this
  vulnerability to our attention. :gl:`#5742`

- [CVE-2026-3119] Prevent a crash in code processing queries containing
  a TKEY record. ``308baa89105``

  The :iscman:`named` process could terminate unexpectedly when
  processing a correctly signed query containing a TKEY record. This has
  been fixed.

  ISC would like to thank Vitaly Simonovich for bringing this
  vulnerability to our attention. :gl:`#5748`

- [CVE-2026-3591] Fix a stack use-after-return flaw in SIG(0) handling
  code. ``aaaae0fd97e``

  A stack use-after-return flaw in SIG(0) handling code could enable ACL
  bypass and/or assertion failures in certain circumstances. This flaw
  has been fixed.

  ISC would like to thank Mcsky23 for bringing this vulnerability to our
  attention. :gl:`#5754`

Bug Fixes
~~~~~~~~~

- Resolve "key defined in view is not found" ``819fe452745``

  Commit `2956e4fc` hardened the `key` name check when used in
  `primaries` to reject the configuration if the key was not defined,
  rather than simply checking whether the key name was correctly formed.

  However, the key name check didn't include the view configuration,
  causing keys not to be recognized if they were defined inside the view
  and not at the global level.  This regression is now fixed.
  :gl:`#5761` :gl:`!11613`


