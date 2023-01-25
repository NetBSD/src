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

Notes for BIND 9.16.17
----------------------

Feature Changes
~~~~~~~~~~~~~~~

- After the network manager was introduced to ``named`` to handle
  incoming traffic, it was discovered that recursive performance had
  degraded compared to previous BIND 9 versions. This has now been
  fixed by processing internal tasks inside network manager worker
  threads, preventing resource contention among two sets of threads.
  :gl:`#2638`

- Zone dumping tasks are now run on separate asynchronous thread pools.
  This change prevents zone dumping from blocking network I/O.
  :gl:`#2732`

- ``inline-signing`` was incorrectly described as being inherited from
  the ``options``/``view`` levels and was incorrectly accepted at those
  levels without effect. This has been fixed; ``named.conf`` files with
  ``inline-signing`` at those levels no longer load. :gl:`#2536`

Bug Fixes
~~~~~~~~~

- The calculation of the estimated IXFR transaction size in
  ``dns_journal_iter_init()`` was invalid. This resulted in excessive
  AXFR-style IXFR responses. :gl:`#2685`

- Fixed an assertion failure that could occur if stale data was used to
  answer a query, and then a prefetch was triggered after the query was
  restarted (for example, to follow a CNAME). :gl:`#2733`

- If a query was answered with stale data on a server with DNS64
  enabled, an assertion could occur if a non-stale answer arrived
  afterward. This has been fixed. :gl:`#2731`

- Fixed an error which caused the ``IP_DONTFRAG`` socket option to be
  enabled instead of disabled, leading to errors when sending oversized
  UDP packets. :gl:`#2746`

- Zones which are configured in multiple views, with different values
  set for ``dnssec-policy`` and with identical values set for
  ``key-directory``, are now detected and treated as a configuration
  error. :gl:`#2463`

- A race condition could occur when reading and writing key files for
  zones using KASP and configured in multiple views. This has been
  fixed. :gl:`#1875`

Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.
