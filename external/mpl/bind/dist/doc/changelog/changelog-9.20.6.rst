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

BIND 9.20.6
-----------

New Features
~~~~~~~~~~~~

- Adds support for EDE code 1 and 2. ``b3eab79bc18``

  Add support for EDE codes 1 & 2 which might occurs during DNSSEC
  validation in case of unsupported RRSIG algorithm or DNSKEY digest.
  :gl:`#2715` :gl:`!9996`

- Add a rndc command to toggle jemalloc profiling. ``38c51c84014``

  The new command is `rndc memprof`. The memory profiling status is also
  reported inside `rndc status`. The status also shows whether named can
  toggle memory profiling or not and if the server is built with
  jemalloc. :gl:`#4759` :gl:`!10000`

- Add support for multiple extended DNS errors. ``4d945128dc1``

  Extended DNS error mechanism (EDE) may have several errors raised
  during a DNS resolution. `named` is now able to add up to three EDE
  codes in a DNS response. In the case of duplicate error codes, only
  the first one will be part of the DNS response. :gl:`#5085`
  :gl:`!9978`

- Print the expiration time of the stale records. ``b5cce0f5972``

  Print the expiration time of the stale RRsets in the cache dump.
  :gl:`!10061`

Feature Changes
~~~~~~~~~~~~~~~

- Refactor reference counting in both QPDB and RBTDB. ``3244f7848fd``

  Clean up the pattern in the newref() and decref() functions in QP and
  RBTDB databases.  Replace the `db_nodelock_t` structure with plain
  reference counting for every active database node in QPDB.

  Related to #5134 :gl:`!10035`

- Shutdown the fetch context after canceling the last fetch.
  ``55b7cc9596e``

  Shutdown the fetch context immediately after the last fetch has been
  canceled from that particular fetch context. :gl:`!9977`

Bug Fixes
~~~~~~~~~

- Fix possible truncation in dns_keymgr_status() ``1333dac316c``

  If the generated status output exceeds 4096 it was silently truncated,
  now we output that the status was truncated. :gl:`#4180` :gl:`!9981`

- Recently expired records could be returned with timestamp in future.
  ``9a4df4caac0``

  Under rare circumstances, the RRSet that expired at the time of the
  query could be returned with TTL far in the future.  This has been
  fixed.

  As a side-effect, the expiration time of expired RRSets are no longer
  printed out in the cache dump. :gl:`#5094` :gl:`!10059`

- Yaml string not terminated in negative response in delv.
  ``74640b3613c``

  :gl:`#5098` :gl:`!9979`

- Fix a bug in dnssec-signzone related to keys being offline.
  ``ddda6cb59e5``

  In the case when `dnssec-signzone` is called on an already signed
  zone, and the private key file is unavailable, a signature that needs
  to be refreshed may be dropped without being able to generate a
  replacement. This has been fixed. :gl:`#5126` :gl:`!9982`

- Apply the memory limit only to ADB database items. ``0ab22458f51``

  Resolver under heavy-load could exhaust the memory available for
  storing the information in the Address Database (ADB) effectively
  evicting already stored information in the ADB.  The memory used to
  retrieve and provide information from the ADB is now not a subject of
  the same memory limits that are applied for storing the information in
  the Address Database. :gl:`#5127` :gl:`!9975`

- Avoid unnecessary locking in the zone/cache database. ``60b81239de1``

  Prevent lock contention among many worker threads referring to the
  same database node at the same time. This would improve zone and cache
  database performance for the heavily contended database nodes.
  :gl:`#5130` :gl:`!9964`

- Fix EDE 22 time out detection. ``8662424442c``

  Extended DNS error 22 (No reachable authority) was previously detected
  when `fctx_expired` fired. It turns out this function is used as a
  "safety net" and the timeout detection should be caught earlier.

  It was working though, because of another issue fixed by !9927. But
  then, the recursive request timed out detection occurs before
  `fctx_expired` making impossible to raise the EDE 22 error.

  This fixes the problem by triggering the EDE 22 in the part of the
  code detecting the (TCP or UDP) time out and taking the decision to
  cancel the whole fetch (i.e. There is no other server to attempt to
  contact).

  Note this is not targeting users (no release note) because there is no
  release versions of BIND between !9927 and this changes. Thus a
  release note would be confusing. :gl:`#5137` :gl:`!10001`

- Split and simplify the use of EDE list implementation. ``23a9bed310b``

  Instead of mixing the dns_resolver and dns_validator units directly
  with the EDE code, split-out the dns_ede functionality into own
  separate compilation unit and hide the implementation details behind
  abstraction.

  Additionally, the new dns_edelist_t doesn't have to be copied into all
  responses as those are attached to the fetch context, but it could be
  only passed by reference.

  This makes the dns_ede implementation simpler to use, although sligtly
  more complicated on the inside. :gl:`#5141` :gl:`!10030`

- Fix the cache findzonecut() implementation. ``619f163e680``

  The search for the deepest known zone cut in the cache could
  improperly reject a node if it contained any stale data, regardless of
  whether it was the NS RRset that was stale. :gl:`#5155` :gl:`!10050`

- DNSSEC EDE system tests on FIPS platform. ``917181b4e27``

  Changes introducing the support of extended DNS error code 1 and 2
  uses SHA-1 digest for some tests which break FIPS platform. The digest
  itself was irrelevant, another digest is used. :gl:`!10031`

- Reduce the false sharing the dns_qpcache and dns_qpzone.
  ``5c27e9cdda6``

  Instead of having many node_lock_count * sizeof(<member>) arrays, pack
  all the members into a qpcache_bucket_t that is cacheline aligned to
  prevent false sharing between RWLocks. :gl:`!10074`


