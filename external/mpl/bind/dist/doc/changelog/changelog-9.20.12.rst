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

BIND 9.20.12
------------

New Features
~~~~~~~~~~~~

- Support for parsing the DSYNC record has been added. ``f440fe712d``

  :gl:`#5440` :gl:`!10820`

Feature Changes
~~~~~~~~~~~~~~~

- Adaptive memory allocation strategy for qp-tries. ``9a046cbed5``

  qp-tries allocate their nodes (twigs) in chunks to reduce allocator
  pressure and improve memory locality. The choice of chunk size
  presents a tradeoff: larger chunks benefit qp-tries with many values
  (as seen in large zones and resolvers) but waste memory in smaller use
  cases.

  Previously, our fixed chunk size of 2^10 twigs meant that even an
  empty qp-trie would consume 12KB of memory, while reducing this size
  would negatively impact resolver performance.

  This MR implements an adaptive chunking strategy that tracks the size
  of the most recently allocated chunk and doubles the chunk size for
  each new allocation until reaching a predefined maximum.

  This approach effectively balances memory efficiency for small tries
  while maintaining the performance benefits of larger chunk sizes for
  bigger data structures. :gl:`#5445` :gl:`!10804`

- Add deprecation warnings for RSASHA1, RSASHA1-NSEC3SHA1 and DS digest
  type 1. ``5aefaa4b97``

  RSASHA1 and RSASHA1-NSEC-SHA1 DNSKEY algorithms have been deprecated
  by the IETF and should no longer be used for DNSSEC. DS digest type 1
  (SHA1) has also been deprecated. Validators are now expected to treat
  these algorithms and digest as unknown, resulting in some zones being
  treated as insecure when they were previously treated as secure.
  Warnings have been added to named and tools when these algorithms and
  this digest are being used for signing.

  Zones signed with RSASHA1 or RSASHA1-NSEC-SHA1 should be migrated to a
  different DNSKEY algorithm.

  Zones with DS or CDS records with digest type 1 (SHA1) should be
  updated to use a different digest type (e.g. SHA256) and the digest
  type 1 records should be removed.

  Related to #5358 :gl:`!10738`

Bug Fixes
~~~~~~~~~

- Stale RRsets in a CNAME chain were not always refreshed.
  ``ed37c7825e``

  With serve-stale enabled, a CNAME chain that contains a stale RRset,
  the refresh query doesn't always properly refresh the stale RRsets.
  This has been fixed. :gl:`#5243` :gl:`!10767`

- Add RPZ extended DNS error for zones with a CNAME override policy
  configured. ``39ad2016c1``

  When the zone is configured with a CNAME override policy, or the
  response policy zone contains a wildcard CNAME, the extended DNS error
  code was not added. This has been fixed. :gl:`#5342` :gl:`!10819`

- Fix a possible crash when adding a zone while recursing.
  ``7a3ec8dd94``

  A query for a zone that was not yet loaded may yield an unexpected
  result such as a CNAME or DNAME, triggering an assertion failure. This
  has been fixed. :gl:`#5357` :gl:`!10718`

- Fix dig issues. ``8c50819aa8``

  When used with the ``+keepopen`` option with a TCP connection,
  iscman:`dig` could terminate unexpectedly in rare situations.
  Additionally, iscman:`dig` could hang and fail to shutdown properly
  when interrupted during a query. These have been fixed. :gl:`#5381`
  :gl:`!10727`

- Log dropped or slipped responses in the query-errors category.
  ``47470b586d``

  Responses which were dropped or slipped because of RRL (Response Rate
  Limiting) were logged in the ``rate-limit`` category instead of the
  ``query-errors`` category, as documented in ARM. This has been fixed.
  :gl:`#5388` :gl:`!10725`

- Separate out adbname type flags. ``fc689c6525``

  There are three adbname flags that are used to identify different
  types of adbname lookups when hashing rather than using multiple hash
  tables.  Separate these to their own structure element as these need
  to be able to be read without locking the adbname structure.
  :gl:`#5404` :gl:`!10695`

- Synth-from-dnssec was not working in some scenarios. ``bc54f059e0``

  Aggressive use of DNSSEC-Validated cache with NSEC was not working in
  scenarios when no parent NSEC was not in cache.  This has been fixed.
  :gl:`#5422` :gl:`!10754`

- Clean enough memory when adding new ADB names/entries under memory
  pressure. ``b7e7923daa``

  The ADB memory cleaning is opportunistic even when we are under memory
  pressure (in the overmem condition).  Split the opportunistic LRU
  cleaning and overmem cleaning and make the overmem cleaning always
  cleanup double of the newly allocated adbname/adbentry to ensure we
  never allocate more memory than the assigned limit. :gl:`!10707`

- Prevent spurious validation failures. ``3b98c7cc9d``

  Under rare circumstances, validation could fail if multiple clients
  simultaneously iterated the same set of signatures.

  References #3014 :gl:`!10815`

- Rename variable called 'free' to prevent the clash with free()
  ``7f25d92c5d``

  :gl:`!10757`


