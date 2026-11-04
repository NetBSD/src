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

Notes for BIND 9.20.15
----------------------

Security Fixes
~~~~~~~~~~~~~~

- DNSSEC validation fails if matching but invalid DNSKEY is found.
  :cve:`2025-8677`

  Previously, if a matching but cryptographically invalid key was
  encountered during DNSSEC validation, the key was skipped and not
  counted towards validation failures. :iscman:`named` now treats such
  DNSSEC keys as hard failures and the DNSSEC validation fails
  immediately, instead of continuing with the next DNSKEYs in the RRset.

  ISC would like to thank Zuyao Xu and Xiang Li from the All-in-One
  Security and Privacy Laboratory at Nankai University for bringing this
  vulnerability to our attention. :gl:`#5343`

- Address various spoofing attacks. :cve:`2025-40778`

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

- Cache-poisoning due to weak pseudo-random number generator.
  :cve:`2025-40780`

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

- Add :any:`dnssec-policy` keys configuration check to
  :iscman:`named-checkconf`.

  A new option :option:`-k <named-checkconf -k>` was added to
  :iscman:`named-checkconf` that allows checking the
  :any:`dnssec-policy` :any:`keys` configuration against the configured
  key stores. If the found key files are not in sync with the given
  :any:`dnssec-policy`, the check will fail.

  This is useful to run before migrating to :any:`dnssec-policy`.
  :gl:`#5486`

Bug Fixes
~~~~~~~~~

- Missing DNSSEC information when CD bit is set in query.

  The RRSIGs for glue records were not being cached correctly for CD=1
  queries. This has been fixed. :gl:`#5502`

- :option:`rndc sign` during ZSK rollover will now replace signatures.

  When performing a ZSK rollover, if the new DNSKEY is omnipresent, the
  :option:`rndc sign` command now signs the zone completely with the
  successor key, replacing all zone signatures from the predecessor key
  with new ones. :gl:`#5483`

- Use signer name when disabling DNSSEC algorithms.

  :any:`disable-algorithms` could cause DNSSEC validation failures when
  the parent zone was signed with the algorithms that were being
  disabled for the child zone. This has been fixed;
  :any:`disable-algorithms` now works on a whole-of-zone basis.

  If the zone's name is at or below the :any:`disable-algorithms` name
  the algorithm is disabled for that zone, using deepest match when
  there are multiple :any:`disable-algorithms` clauses. :gl:`#5165`

- Preserve cache when reload fails and reload the server again.

  This fixes an issue where failing to reconfigure/reload the server
  would fail to preserve the views' caches for subsequent server
  reconfigurations/reloads. :gl:`#5523`
