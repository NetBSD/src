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

Notes for BIND 9.18.16
----------------------

Security Fixes
~~~~~~~~~~~~~~

- The overmem cleaning process has been improved, to prevent the cache from
  significantly exceeding the configured :any:`max-cache-size` limit.
  :cve:`2023-2828`

  ISC would like to thank Shoham Danino from Reichman University, Anat
  Bremler-Barr from Tel-Aviv University, Yehuda Afek from Tel-Aviv University,
  and Yuval Shavitt from Tel-Aviv University for bringing this vulnerability to
  our attention.  :gl:`#4055`

- A query that prioritizes stale data over lookup triggers a fetch to refresh
  the stale data in cache. If the fetch is aborted for exceeding the recursion
  quota, it was possible for :iscman:`named` to enter an infinite callback
  loop and crash due to stack overflow. This has been fixed. :cve:`2023-2911`
  :gl:`#4089`

New Features
~~~~~~~~~~~~

- The system test suite can now be executed with pytest (along with
  pytest-xdist for parallel execution). :gl:`#3978`

Removed Features
~~~~~~~~~~~~~~~~

- TKEY mode 2 (Diffie-Hellman Exchanged Keying) is now deprecated, and
  will be removed in a future release. A warning will be logged when
  the :any:`tkey-dhkey` option is used in ``named.conf``. :gl:`#3905`

Bug Fixes
~~~~~~~~~

- BIND could get stuck on reconfiguration when a :any:`listen-on`
  statement for HTTP is removed from the configuration. That has been
  fixed. :gl:`#4071`

- Previously, it was possible for a delegation from cache to be returned
  to the client after the :any:`stale-answer-client-timeout` duration.
  This has been fixed. :gl:`#3950`

- BIND could allocate too big buffers when sending data via
  stream-based DNS transports, leading to increased memory usage.
  This has been fixed. :gl:`#4038`

- When the :any:`stale-answer-enable` option was enabled and the
  :any:`stale-answer-client-timeout` option was enabled and larger than
  0, :iscman:`named` previously allocated two slots from the
  :any:`clients-per-query` limit for each client and failed to gradually
  auto-tune its value, as configured. This has been fixed. :gl:`#4074`

Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.
