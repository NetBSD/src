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

BIND 9.20.15
------------

Security Fixes
~~~~~~~~~~~~~~

- [CVE-2025-8677] DNSSEC validation fails if matching but invalid DNSKEY
  is found. ``0d676bf9f23``

  Previously, if a matching but cryptographically invalid key was
  encountered during DNSSEC validation, the key was skipped and not
  counted towards validation failures. :iscman:`named` now treats such
  DNSSEC keys as hard failures and the DNSSEC validation fails
  immediately, instead of continuing with the next DNSKEYs in the RRset.

  ISC would like to thank Zuyao Xu and Xiang Li from the All-in-One
  Security and Privacy Laboratory at Nankai University for bringing this
  vulnerability to our attention. :gl:`#5343`

- [CVE-2025-40778] Address various spoofing attacks. ``23de94fd236``

  Previously, several issues could be exploited to poison a DNS cache
  with spoofed records for zones which were not DNSSEC-signed or if the
  resolver was configured to not do DNSSEC validation. These issues were
  assigned CVE-2025-40778 and have now been fixed.

  As an additional layer of protection, :iscman:`named` no longer
  accepts DNAME records or extraneous NS records in the AUTHORITY
  section unless these are received via spoofing-resistant transport
  (TCP, UDP with DNS cookies, TSIG, or SIG(0)).

  ISC would like to thank Yuxiao Wu, Yunyi Zhang, Baojun Liu, and Haixin
  Duan from Tsinghua University for bringing this vulnerability to our
  attention. :gl:`#5414`

- [CVE-2025-40780] Cache-poisoning due to weak pseudo-random number
  generator. ``34af35c2df8``

  It was discovered during research for an upcoming academic paper that
  a xoshiro128\*\* internal state can be recovered by an external 3rd
  party, allowing the prediction of UDP ports and DNS IDs in outgoing
  queries. This could lead to an attacker spoofing the DNS answers with
  great efficiency and poisoning the DNS cache.

  The internal random generator has been changed to a cryptographically
  secure pseudo-random generator.

  ISC would like to thank Prof. Amit Klein and Omer Ben Simhon from
  Hebrew University of Jerusalem for bringing this vulnerability to our
  attention. :gl:`#5484`

New Features
~~~~~~~~~~~~

- Add dnssec-policy keys configuration check to named-checkconf.
  ``1f5a0405f72``

  A new option `-k` is added to `named-checkconf` that allows checking
  the `dnssec-policy` `keys` configuration against the configured key
  stores. If the found key files are not in sync with the given
  `dnssec-policy`, the check will fail.

  This is useful to run before migrating to `dnssec-policy`. :gl:`#5486`
  :gl:`!11011`

Feature Changes
~~~~~~~~~~~~~~~

- Minor refactor of dst code. ``c6acbaa020b``

  Convert the defines to enums. Initialize the tags more explicitly and
  less ugly. :gl:`!11038`

Bug Fixes
~~~~~~~~~

- Use signer name when disabling DNSSEC algorithms. ``986816baa74``

  ``disable-algorithms`` could cause DNSSEC validation failures when the
  parent zone was signed with the algorithms that were being disabled
  for the child zone. This has been fixed; `disable-algorithms` now
  works on a whole-of-zone basis.

  If the zone's name is at or below the ``disable-algorithms`` name the
  algorithm is disabled for that zone, using deepest match when there
  are multiple ``disable-algorithms`` clauses.  :gl:`#5165` :gl:`!11014`

- Rndc sign during ZSK rollover will now replace signatures.
  ``d2f551140cd``

  When performing a ZSK rollover, if the new DNSKEY is omnipresent, the
  :option:`rndc sign` command now signs the zone completely with the
  successor key, replacing all zone signatures from the predecessor key
  with new ones. :gl:`#5483` :gl:`!11017`

- Missing DNSSEC information when CD bit is set in query.
  ``968a6be41fb``

  The RRSIGs for glue records were not being cached correctly for CD=1
  queries.  This has been fixed. :gl:`#5502` :gl:`!10956`

- Preserve cache when reload fails and reload the server again.
  ``975aeda10b4``

  Fixes an issue where failing to reconfigure/reload the server would
  prevent to preserved the views caches on the subsequent server
  reconfiguration/reload. :gl:`#5523` :gl:`!10988`

- Check plugin config before registering. ``e2260b80702``

  In `named_config_parsefile()`, when checking the validity of
  `named.conf`, the checking of plugin correctness was deliberately
  postponed until the plugin is loaded and registered. However, the
  checking was never actually done: the `plugin_register()`
  implementation was called, but `plugin_check()` was not.

  `ns_plugin_register()` (used by `named`) now calls the check function
  before the register function, and aborts if either one fails.
  `ns_plugin_check()` (used by `named-checkconf`) calls only the check
  function. :gl:`!11032`


