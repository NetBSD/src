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

Notes for BIND 9.16.19
----------------------

New Features
~~~~~~~~~~~~

- Using a new configuration option, ``parental-agents``, each zone can
  now be associated with a list of servers that can be used to check the
  DS RRset in the parent zone. This enables automatic KSK rollovers.
  :gl:`#1126`

Feature Changes
~~~~~~~~~~~~~~~

- IP fragmentation has been disabled for outgoing UDP sockets. Errors
  triggered by sending DNS messages larger than the specified path MTU
  are properly handled by sending empty DNS replies with the ``TC``
  (TrunCated) bit set, which forces DNS clients to fall back to TCP.
  :gl:`#2790`

Bug Fixes
~~~~~~~~~

- The code managing :rfc:`5011` trust anchors created an invalid
  placeholder keydata record upon a refresh failure, which prevented the
  database of managed keys from subsequently being read back. This has
  been fixed. :gl:`#2686`

- Signed, insecure delegation responses prepared by ``named`` either
  lacked the necessary NSEC records or contained duplicate NSEC records
  when both wildcard expansion and CNAME chaining were required to
  prepare the response. This has been fixed. :gl:`#2759`

- If ``nsupdate`` sends an SOA request and receives a REFUSED response,
  it now fails over to the next available server. :gl:`#2758`

- A bug that caused the NSEC3 salt to be changed on every restart for
  zones using KASP has been fixed. :gl:`#2725`

- The configuration-checking code failed to account for the inheritance
  rules of the ``dnssec-policy`` option. This has been fixed.
  :gl:`#2780`

- The fix for :gl:`#1875` inadvertently introduced a deadlock: when
  locking key files for reading and writing, the ``in-view`` logic was
  not considered. This has been fixed. :gl:`#2783`

- A race condition could occur where two threads were competing for the
  same set of key file locks, leading to a deadlock. This has been
  fixed. :gl:`#2786`

Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.
