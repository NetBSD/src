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

Notes for BIND 9.16.15
----------------------

Security Fixes
~~~~~~~~~~~~~~

- A malformed incoming IXFR transfer could trigger an assertion failure
  in ``named``, causing it to quit abnormally. (CVE-2021-25214)

  ISC would like to thank Greg Kuechle of SaskTel for bringing this
  vulnerability to our attention. :gl:`#2467`

- ``named`` crashed when a DNAME record placed in the ANSWER section
  during DNAME chasing turned out to be the final answer to a client
  query. (CVE-2021-25215)

  ISC would like to thank `Siva Kakarla`_ for bringing this
  vulnerability to our attention. :gl:`#2540`

.. _Siva Kakarla: https://github.com/sivakesava1

- When a server's configuration set the ``tkey-gssapi-keytab`` or
  ``tkey-gssapi-credential`` option, a specially crafted GSS-TSIG query
  could cause a buffer overflow in the ISC implementation of SPNEGO (a
  protocol enabling negotiation of the security mechanism used for
  GSSAPI authentication). This flaw could be exploited to crash
  ``named`` binaries compiled for 64-bit platforms, and could enable
  remote code execution when ``named`` was compiled for 32-bit
  platforms. (CVE-2021-25216)

  This vulnerability was reported to us as ZDI-CAN-13347 by Trend Micro
  Zero Day Initiative. :gl:`#2604`

Feature Changes
~~~~~~~~~~~~~~~

- The ISC implementation of SPNEGO was removed from BIND 9 source code.
  Instead, BIND 9 now always uses the SPNEGO implementation provided by
  the system GSSAPI library when it is built with GSSAPI support. All
  major contemporary Kerberos/GSSAPI libraries contain an implementation
  of the SPNEGO mechanism. :gl:`#2607`

- The default value for the ``stale-answer-client-timeout`` option was
  changed from ``1800`` (ms) to ``off``. The default value may be
  changed again in future releases as this feature matures. :gl:`#2608`

Bug Fixes
~~~~~~~~~

- TCP idle and initial timeouts were being incorrectly applied: only the
  ``tcp-initial-timeout`` was applied on the whole connection, even if
  the connection were still active, which could prevent a large zone
  transfer from being sent back to the client. The default setting for
  ``tcp-initial-timeout`` was 30 seconds, which meant that any TCP
  connection taking more than 30 seconds was abruptly terminated. This
  has been fixed. :gl:`#2583`

- When ``stale-answer-client-timeout`` was set to a positive value and
  recursion for a client query completed when ``named`` was about to
  look for a stale answer, an assertion could fail in
  ``query_respond()``, resulting in a crash. This has been fixed.
  :gl:`#2594`

- If zone journal files written by BIND 9.16.11 or earlier were present
  when BIND was upgraded to BIND 9.16.13 or BIND 9.16.14, the zone file
  for that zone could have been inadvertently rewritten with the current
  zone contents. This caused the original zone file structure (e.g.
  comments, ``$INCLUDE`` directives) to be lost, although the zone data
  itself was preserved. :gl:`#2623`

- After upgrading to BIND 9.16.13, journal files for trust anchor
  databases (e.g. ``managed-keys.bind.jnl``) could be left in a corrupt
  state. (Other zone journal files were not affected.) This has been
  fixed. If a corrupt journal file is detected, ``named`` can now
  recover from it. :gl:`#2600`

- When sending queries over TCP, ``dig`` now properly handles ``+tries=1
  +retry=0`` by not retrying the connection when the remote server
  closes the connection prematurely. :gl:`#2490`

- CDS/CDNSKEY DELETE records are now removed when a zone transitions
  from a secure to an insecure state. ``named-checkzone`` also no longer
  reports an error when such records are found in an unsigned zone.
  :gl:`#2517`

- Zones using KASP could not be thawed after they were frozen using
  ``rndc freeze``. This has been fixed. :gl:`#2523`

- After ``rndc checkds -checkds`` or ``rndc dnssec -rollover`` is used,
  ``named`` now immediately attempts to reconfigure zone keys. This
  change prevents unnecessary key rollover delays. :gl:`#2488`

- Previously, a memory leak could occur when ``named`` failed to bind a
  UDP socket to a network interface. This has been fixed. :gl:`#2575`

Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.
