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

Notes for BIND 9.16.27
----------------------

Security Fixes
~~~~~~~~~~~~~~

- The rules for acceptance of records into the cache have been tightened
  to prevent the possibility of poisoning if forwarders send records
  outside the configured bailiwick. (CVE-2021-25220)

  ISC would like to thank Xiang Li, Baojun Liu, and Chaoyi Lu from
  Network and Information Security Lab, Tsinghua University, and
  Changgen Zou from Qi An Xin Group Corp. for bringing this
  vulnerability to our attention. :gl:`#2950`

- TCP connections with ``keep-response-order`` enabled could leave the
  TCP sockets in the ``CLOSE_WAIT`` state when the client did not
  properly shut down the connection. (CVE-2022-0396) :gl:`#3112`

Feature Changes
~~~~~~~~~~~~~~~

- DEBUG(1)-level messages were added when starting and ending the BIND 9
  task-exclusive mode that stops normal DNS operation (e.g. for
  reconfiguration, interface scans, and other events that require
  exclusive access to a shared resource). :gl:`#3137`

Bug Fixes
~~~~~~~~~

- The ``max-transfer-time-out`` and ``max-transfer-idle-out`` options
  were not implemented when the BIND 9 networking stack was refactored
  in 9.16. The missing functionality has been re-implemented and
  outgoing zone transfers now time out properly when not progressing.
  :gl:`#1897`

- TCP connections could hang indefinitely if the other party did not
  read sent data, causing the TCP write buffers to fill. This has been
  fixed by adding a "write" timer. Connections that are hung while
  writing now time out after the ``tcp-idle-timeout`` period has
  elapsed. :gl:`#3132`

- The statistics counter representing the current number of clients
  awaiting recursive resolution results (``RecursClients``) could be
  miscalculated in certain resolution scenarios, potentially causing the
  value of the counter to drop below zero. This has been fixed.
  :gl:`#3147`

Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.
