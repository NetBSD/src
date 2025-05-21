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

Notes for BIND 9.20.6
---------------------

New Features
~~~~~~~~~~~~

- Adds support for EDE code 1 and 2.

  Support was added for EDE codes 1 and 2, which might occur during DNSSEC
  validation in the case of an unsupported RRSIG algorithm or DNSKEY digest.
  :gl:`#2715`

- Add an :iscman:`rndc` command to toggle jemalloc profiling.

  The new command is :option:`rndc memprof`; the memory profiling status is also
  reported inside :option:`rndc status`. The status shows whether
  :iscman:`named` can toggle memory profiling, and whether the server is built
  with jemalloc. :gl:`#4759`

- Add support for multiple extended DNS errors.

  The Extended DNS Error (EDE) mechanism may raise errors
  during a DNS resolution. :iscman:`named` is now able to add up to three EDE
  codes in a DNS response. If there are duplicate error codes, only
  the first one is part of the DNS response. :gl:`#5085`

- Print the expiration time of stale records.

  BIND now prints the expiration time of any stale RRsets in the cache dump.

Bug Fixes
~~~~~~~~~

- Recently expired records could be returned with a timestamp in future.

  Under rare circumstances, an RRSet that expired at the time of the
  query could be returned with a TTL in the future. This has been
  fixed.

  As a side effect, the expiration time of expired RRSets is no longer
  returned in a cache dump. :gl:`#5094`

- YAML string not terminated in negative response in delv.

  :gl:`#5098`

- Fix a bug in :iscman:`dnssec-signzone` related to keys being offline.

  When :iscman:`dnssec-signzone` was called on an already-signed
  zone and the private key file was unavailable, a signature that needed
  to be refreshed was dropped without being able to generate a
  replacement. This has been fixed. :gl:`#5126`

- Apply the memory limit only to ADB database items.

  Under heavy load, a resolver could exhaust the memory available for
  storing the information in the Address Database (ADB), effectively
  discarding previously stored information in the ADB. The memory used to
  retrieve and provide information from the ADB is no longer subject to
  the same memory limits that are applied to
  the Address Database. :gl:`#5127`

- Avoid unnecessary locking in the zone/cache database.

  Lock contention among many worker threads referring to the
  same database node at the same time is now prevented. This improves zone and
  cache database performance for any heavily contended database nodes.
  :gl:`#5130`

- Fix reporting of Extended DNS Error 22 (No Reachable Authority).

  This error code was previously not reported in some applicable
  situations.  This has been fixed. :gl:`#5137`

