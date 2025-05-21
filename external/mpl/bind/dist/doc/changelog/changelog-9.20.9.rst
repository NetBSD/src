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

BIND 9.20.9
-----------

Security Fixes
~~~~~~~~~~~~~~

- [CVE-2025-40775] Prevent assertion when processing TSIG algorithm.
  ``b8c198ac5ca``

  DNS messages that included a Transaction Signature (TSIG) containing
  an invalid value in the algorithm field caused :iscman:`named` to
  crash with an assertion failure. This has been fixed.
  :cve:`2025-40775` :gl:`#5300`

Feature Changes
~~~~~~~~~~~~~~~

- Use jinja2 templates in system tests. ``8f545784ff0``

  `python-jinja2` is now required to run system tests. :gl:`#4938`
  :gl:`!10396`

Bug Fixes
~~~~~~~~~

- Fix EDNS yaml output. ``8c3b226d89b``

  `dig` was producing invalid YAML when displaying some EDNS options.
  This has been corrected.

  Several other improvements have been made to the display of EDNS
  option data: - We now use the correct name for the UPDATE-LEASE
  option, which was previously displayed as "UL", and split it into
  separate LEASE and LEASE-KEY components in YAML mode. - Human-readable
  durations are now displayed as comments in YAML mode so as not to
  interfere with machine parsing. - KEY-TAG options are now displayed as
  an array of integers in YAML mode. - EDNS COOKIE options are displayed
  as separate CLIENT and SERVER components, and cookie STATUS is a
  retrievable variable in YAML mode. :gl:`#5014` :gl:`!10414`

- Return DNS COOKIE and NSID with BADVERS. ``34b7323bad6``

  This change allows the client to identify the server that returns the
  BADVERS and to provide a DNS SERVER COOKIE to be included in the
  resend of the request. :gl:`#5235` :gl:`!10392`

- Disable own memory context for libxml2 on macOS. ``51e51d5ea8f``

  Apple broke custom memory allocation functions in the system-wide
  libxml2 starting with macOS Sequoia 15.4.  Usage of the custom memory
  allocation functions has been disabled on macOS. :gl:`#5268`
  :gl:`!10411`

- `check_private` failed to account for the length byte before the OID.
  ``2b827380e75``

  In PRIVATEOID keys, the key data begins with a length byte followed
  by an ASN.1 object identifier that indicates the cryptographic
  algorithm  to use. Previously, the length byte was not accounted for
  when  checking the contents of keys and signatures, which could have
  led to interoperability problems with any zones signed using
  PRIVATEOID. This has been fixed. :gl:`#5270` :gl:`!10376`

- Fix a serve-stale issue with a delegated zone. ``d839d11bf62``

  When ``stale-answer-client-timeout 0`` option was enabled, it could be
  ignored when resolving a zone which is a delegation of an
  authoritative zone belonging to the resolver. This has been fixed.
  :gl:`#5275` :gl:`!10420`

- Fix the ksr two-tone test. ``3e2b255b5b7``

  The two-tone ksr subtest (test_ksr_twotone) depended on the
  dnssec-policy keys algorithm values in named.conf being entered in
  numerical order.  As the algorithms used in the test can be selected
  randomly this does not always happen. Sort the dnssec-policy keys by
  algorithm when adding them to the key list from named.conf.
  :gl:`#5286` :gl:`!10435`

- Revert NSEC3 closest encloser lookup improvements. ``ac41f158fad``

  The performance improvements for NSEC3 closest encloser lookups that
  were restored in BIND 9.20.8 turned out to cause incorrect NSEC3
  records to be returned in nonexistence proofs and were therefore
  reverted again. :gl:`#5292` :gl:`!10443`


