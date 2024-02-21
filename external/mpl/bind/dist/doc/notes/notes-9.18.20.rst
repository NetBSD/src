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

Notes for BIND 9.18.20
----------------------

Feature Changes
~~~~~~~~~~~~~~~

- The IP addresses for B.ROOT-SERVERS.NET have been updated to
  170.247.170.2 and 2801:1b8:10::b. :gl:`#4101`

Bug Fixes
~~~~~~~~~

- If the unsigned version of an inline-signed zone contained DNSSEC
  records, it was incorrectly scheduled for resigning. This has been
  fixed. :gl:`#4350`

- Looking up stale data from the cache did not take local authoritative
  data into account. This has been fixed. :gl:`#4355`

- An assertion failure was triggered when :any:`lock-file` was used at
  the same time as the :option:`named -X` command-line option. This has
  been fixed. :gl:`#4386`

- The :any:`lock-file` file was being removed when it should not have
  been, making the statement ineffective when :iscman:`named` was
  started three or more times. This has been fixed. :gl:`#4387`

Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.
