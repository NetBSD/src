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

Notes for BIND 9.18.5
---------------------

Feature Changes
~~~~~~~~~~~~~~~

- The :option:`dnssec-signzone -H` default value has been changed to 0
  additional NSEC3 iterations. This change aligns the
  :iscman:`dnssec-signzone` default with the default used by the
  :any:`dnssec-policy` feature. At the same
  time, documentation about NSEC3 has been aligned with the `Best
  Current Practice`_. :gl:`#3395`

.. _Best Current Practice: https://datatracker.ietf.org/doc/html/draft-ietf-dnsop-nsec3-guidance-10

Bug Fixes
~~~~~~~~~

- An assertion failure caused by a TCP connection closing between a
  connect (or accept) and a read from a socket has been fixed.
  :gl:`#3400`

- When grafting non-delegated namespace onto delegated namespace,
  :any:`synth-from-dnssec` could incorrectly synthesize non-existence of
  records within the non-delegated namespace using NSEC records from
  higher zones. :gl:`#3402`

- Previously, :iscman:`named` immediately returned a SERVFAIL response
  to the client when it received a FORMERR response from an
  authoritative server during recursive resolution. This has been fixed:
  :iscman:`named` acting as a resolver now attempts to contact other
  authoritative servers for a given domain when it receives a FORMERR
  response from one of them. :gl:`#3152`

- Previously, :option:`rndc reconfig` did not pick up changes to
  :any:`endpoints` statements in :any:`http` blocks. This has been
  fixed. :gl:`#3415`

- It was possible for a catalog zone consumer to process a catalog zone
  member zone when there was a configured pre-existing forward-only
  forward zone with the same name. This has been fixed. :gl:`#2506`

Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.
