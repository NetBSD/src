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

Notes for BIND 9.20.9
---------------------

Security Fixes
~~~~~~~~~~~~~~

- Prevent an assertion failure when processing TSIG algorithm.

  DNS messages that included a Transaction Signature (TSIG) containing
  an invalid value in the algorithm field caused :iscman:`named` to
  crash with an assertion failure. This has been fixed.
  :cve:`2025-40775` :gl:`#5300`

Feature Changes
~~~~~~~~~~~~~~~

- Return DNS COOKIE and NSID with BADVERS.

  This change allows the client to identify a server that returns a
  BADVERS response and to provide a DNS SERVER COOKIE to be included in
  the resent request. :gl:`#5235`

- Disable separate memory context for libxml2 memory allocations on
  macOS.

  As of macOS Sequoia 15.4, custom memory allocation functions are no
  longer supported by the system-wide version of libxml2. This prevents
  tracking libxml2 memory allocations in a separate :iscman:`named`
  memory context, so the latter has been disabled on macOS; the system
  allocator is now directly used for libxml2 memory allocations on that
  operating system. :gl:`#5268`

- Use Jinja2 templates in system tests.

  `python-jinja2` is now required to run system tests. :gl:`#4938`

Bug Fixes
~~~~~~~~~

- Revert NSEC3 closest encloser lookup improvements.

  The performance improvements for NSEC3 closest encloser lookups that
  were restored in BIND 9.20.8 turned out to cause incorrect NSEC3
  records to be returned in nonexistence proofs and were therefore
  reverted again. :gl:`#5292`

- Fix EDNS YAML output in :iscman:`dig`.

  :iscman:`dig` was producing invalid YAML when displaying some EDNS
  options.  This has been corrected.

  Several other improvements have been made to the display of EDNS
  option data:

    - The correct name is now used for the UPDATE-LEASE option, which
      was previously displayed as ``UL``, and it is split into separate
      ``LEASE`` and ``LEASE-KEY`` components in YAML mode.

    - Human-readable durations are now displayed as comments in YAML
      mode so as not to interfere with machine parsing.

    - KEY-TAG options are now displayed as an array of integers in YAML
      mode.

    - EDNS COOKIE options are displayed as separate ``CLIENT`` and
      ``SERVER`` components, and cookie STATUS is a retrievable variable
      in YAML mode.

  :gl:`#5014`

- Fix RDATA checks for PRIVATEOID keys.

  In PRIVATEOID keys, the key data begins with a length byte followed by
  an ASN.1 object identifier that indicates the cryptographic algorithm
  to use. Previously, the length byte was not accounted for when
  checking the contents of keys and signatures, which could have led to
  interoperability problems with any zones signed using PRIVATEOID. This
  has been fixed. :gl:`#5270`

- Fix a serve-stale issue with a delegated zone.

  Even with :any:`stale-answer-client-timeout` set to ``0``, stale
  responses were not returned immediately for names in domains delegated
  from authoritative zones configured on the resolver. This has been
  fixed. :gl:`#5275`
