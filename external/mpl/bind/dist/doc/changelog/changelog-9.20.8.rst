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

BIND 9.20.8
-----------

New Features
~~~~~~~~~~~~

- Add support for EDE 20 (Not Authoritative) ``f8a293aa11``

  Support was added for EDE codes 20 (Not Authoritative) when client
  requests recursion (RD) but the server has recursion disabled.

  RFC 8914 mention EDE 20 should also be returned if the client doesn't
  have the RD bit set (and recursion is needed) but it doesn't apply for
  BIND as BIND would try to resolve from the "deepest" referral in
  AUTHORITY section. For example, if the client asks for "www.isc.org/A"
  but the server only knows the root domain, it will return NOERROR but
  no answer for "www.isc.og/A", just the list of other servers to ask.
  :gl:`#1836` :gl:`!10243`

- Add support for EDE 7 and EDE 8. ``27442c3104``

  Support was added for EDE codes 7 (Signature Expired) and 8 (Signature
  Not Yet Valid) which might occur during DNSSEC validation. :gl:`#2715`
  :gl:`!10242`

- Dig can now display the received BADVERS message during negotiation.
  ``a763080a87``

  Dig +showbadvers now displays the received BADVERS message and
  continues the EDNS version negotiation.  Previously to see the BADVERS
  message +noednsneg had to be specified which terminated the EDNS
  negotiation.  Additionally the specified EDNS value (+edns=value) is
  now used when making all the initial queries with +trace. i.e EDNS
  version negotiation will be performed with each server when performing
  the trace. :gl:`#5234` :gl:`!10247`

- Add an rndc command to reset some statistics counters. ``7c4603fc4a``

  The new ``reset-stats`` command for ``rndc`` allows some statistics
  counters to be reset during runtime. At the moment only two
  "high-water" counters are supported, so the ability to reset them
  after the initial peaks during the server's "warm-up" phase may be
  useful for some operators. :gl:`#5251` :gl:`!10335`

- Implement -T cookiealwaysvalid. ``1da738ffbb``

  When `-T cookiealwaysvalid` is passed to `named`, DNS cookie checks
  for the incoming queries always pass, given they are structurally
  correct. :gl:`!10264`

Removed Features
~~~~~~~~~~~~~~~~

- Remove dns_qpmulti_lockedread declaration. ``42ab4fce4a``

  This function was removed in 6217e434b57bd5d60ed69f792ae9a1a65a008f57
  but not from the header file. :gl:`!10314`

Feature Changes
~~~~~~~~~~~~~~~

- Carefully check if the server name used for SNI is a hostname.
  ``92eb6416e5``

  Previously the code would not check if the string intended to be used
  for SNI is a hostname.

  See also: !9923 :gl:`#5225` :gl:`!10347`

Bug Fixes
~~~~~~~~~

- Restore NSEC3 closest encloser lookup improvements. ``ab6fb7b8f2``

  A performance improvement for finding the closest encloser when
  generating authoritative responses from NSEC3 zones was previously
  reverted after a bug was found that could trigger an assertion
  failure. ( :gl:`#4460`, #4950, and #5108 for details.)  The bug has
  now been fixed, and the performance improvement has been restored.
  :gl:`#5204`  :gl:`!10034`

- Add missing locks when returning addresses. ``2c7594709c``

  Add missing locks in dns_zone_getxfrsource4 et al.  Addresses CID
  468706, 468708, 468741, 468742, 468785, and 468778.

  Cleanup dns_zone_setxfrsource4 et al to now return void.

  Remove double copies with dns_zone_getprimaryaddr and
  dns_zone_getsourceaddr. :gl:`#4933` :gl:`!10259`

- Stop caching lack of EDNS support. ``96bb3a1952``

  `named` could falsely learn that a server doesn't support EDNS when a
  spoofed response was received; that subsequently prevented DNSSEC
  lookups from being made. This has been fixed. :gl:`#3949` :gl:`#5066`

- Step() could ignore rollbacks. ``2334b7a371``

  The `step()` function (used for stepping to the prececessor or
  successor of a database node) could overlook a node if there was an
  rdataset that was marked IGNORE because it had been rolled back,
  covering an active rdataset under it. :gl:`#5170` :gl:`!10257`

- Fix resolver statistics counters for timed out responses.
  ``1d8334a62a``

  When query responses timed out, the resolver could incorrectly
  increase the regular responses counters, even if no response was
  received. This has been fixed. :gl:`#5193` :gl:`!10287`

- Validating ADB fetches could cause a crash in import_rdataset()
  ``b0c6af6ad7``

  Previously, in some cases, the resolver could return rdatasets of type
  CNAME or DNAME without the result code being set to `DNS_R_CNAME` or
  `DNS_R_DNAME`. This could trigger an assertion failure in the ADB. The
  resolver error has been fixed. :gl:`#5201` :gl:`!10329`

- Nested DNS validation could cause assertion failure. ``6bf4390f25``

  When multiple nested DNS validations were destroyed out of order, the
  EDE context could be freed before all EDE codes were copied, which
  could cause an assertion failure. This has been fixed. :gl:`#5213`
  :gl:`!10366`

- [9.20] Wait for memory reclamation to finish in named-checkconf.
  ``9986dad2dc``

  Previously, when named-checkzone loaded the zone to the QP database,
  the delayed memory reclamation could cause an assertion check on exit.
  This has been fixed. :gl:`#5220` :gl:`!10307`

- Ensure max-clients-per-query is at least clients-per-query.
  ``8f78219cc1``

  If the `max-clients-per-query` option is set to a lower value than
  `clients-per-query`, the value is adjusted to match
  `clients-per-query`. :gl:`#5224` :gl:`!10244`

- Fix handling of revoked keys. ``a347273b9c``

  When a key is revoked, its key ID changes due to the inclusion of the
  "revoked" flag. A collision between this changed key ID and an
  unrelated public-only key could cause a crash in `dnssec-signzone`.
  :gl:`#5231` :gl:`!10256`

- Fix adbname reference. ``7ebcc54d3b``

  Call `dns_adbname_ref` before calling `dns_resolver_createfetch` to
  ensure `adbname->name` remains stable for the life of the fetch.
  :gl:`#5239` :gl:`!10303`

- Fix write after free in validator code. ``5de1b3ba3c``

  Raw integer pointers were being used for the validator's nvalidations
  and nfails values but the memory holding them could be freed before
  they ceased to be used.  Use reference counted counters instead.
  :gl:`#5239` :gl:`!10300`

- Don't enforce NOAUTH/NOCONF flags in DNSKEYs. ``5d126d8081``

  All DNSKEY keys are able to authenticate. The `DNS_KEYTYPE_NOAUTH`
  (and `DNS_KEYTYPE_NOCONF`) flags were defined for the KEY rdata type,
  and are not applicable to DNSKEY. Previously, however, because the
  DNSKEY implementation was built on top of KEY, the `_NOAUTH` flag
  prevented authentication in DNSKEYs as well. This has been corrected.
  :gl:`#5240` :gl:`!10315`

- Fix several small DNSSEC timing issues. ``3a78a4c288``

  The following small issues related to `dnssec-policy` have been fixed:
  - In some cases the key manager inside BIND 9 could run every hour,
  while it could have run less often. - While `CDS` and `CDNSKEY`
  records will be removed correctly from the zone when the corresponding
  `DS` record needs to be updated, the expected timing metadata when
  this will happen was never set. - There were a couple of cases where
  the safety intervals are added inappropriately, delaying key rollovers
  longer than necessary. - If you have identical `keys` in your
  `dnssec-policy`, they may be retired inappropriately. Note that having
  keys with identical properties is discouraged in all cases.
  :gl:`#5242` :gl:`!10301`

- Copy __FILE__ when allocating memory. ``59635e33d0``

  When allocating memory under -m trace|record, the __FILE__ pointer is
  stored, so it can be printed out later in order to figure out in which
  file an allocation leaked. (among others, like the line number).

  However named crashes when called with -m record and using a plugin
  leaking memory. The reason is that plugins are unloaded earlier than
  when the leaked allocations are dumped (obviously, as it's done as
  late as possible). In such circumstances, `__FILE__` is dangling
  because the dynamically loaded library (the plugin) is not in memory
  anymore.

  Fix the crash by systematically copying the `__FILE__` string instead
  of copying the pointer. Of course, this make each allocation to
  consume a bit more memory (and longer, as it needs to calculate the
  length of `__FILE__`) but this occurs only under -m trace|record
  debugging flags. :gl:`!10336`

- Fix invalid cache-line padding for qpcache buckets. ``4297ae4795``

  The isc_queue_t was missing in the calculation of the required padding
  size inside the qpcache bucket structure. :gl:`!10317`


