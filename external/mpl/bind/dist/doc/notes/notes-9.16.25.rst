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

Notes for BIND 9.16.25
----------------------

Feature Changes
~~~~~~~~~~~~~~~

- Overall memory use by ``named`` has been optimized and reduced,
  especially on systems with many CPU cores. The default memory
  allocator has been switched from ``internal`` to ``external``. A new
  command-line option ``-M internal`` allows ``named`` to be started
  with the old internal memory allocator. :gl:`#2398`

Bug Fixes
~~~~~~~~~

- On FreeBSD, TCP connections leaked a small amount of heap memory,
  leading to an eventual out-of-memory problem. This has been fixed.
  :gl:`#3051`

- If signatures created by the ZSK were expired and the ZSK private key
  was offline, the signatures were not replaced. This behavior has been
  amended to replace the expired signatures with new signatures created
  using the KSK. :gl:`#3049`

- Under certain circumstances, the signed version of an inline-signed
  zone could be dumped to disk without the serial number of the unsigned
  version of the zone. This prevented resynchronization of the zone
  contents after ``named`` restarted, if the unsigned zone file was
  modified while ``named`` was not running. This has been fixed.
  :gl:`#3071`

Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.
