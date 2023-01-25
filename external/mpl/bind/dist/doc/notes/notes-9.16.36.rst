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

Notes for BIND 9.16.36
----------------------

Feature Changes
~~~~~~~~~~~~~~~

- The ``auto-dnssec`` option has been deprecated and will be removed in
  a future BIND 9.19.x release. Please migrate to ``dnssec-policy``.
  :gl:`#3667`

Bug Fixes
~~~~~~~~~

- When a catalog zone was removed from the configuration, in some cases
  a dangling pointer could cause the :iscman:`named` process to crash.
  This has been fixed. :gl:`#3683`

- When a zone was deleted from a server, a key management object related
  to that zone was inadvertently kept in memory and only released upon
  shutdown. This could lead to constantly increasing memory use on
  servers with a high rate of changes affecting the set of zones being
  served. This has been fixed. :gl:`#3727`

- In certain cases, :iscman:`named` waited for the resolution of
  outstanding recursive queries to finish before shutting down. This was
  unintended and has been fixed. :gl:`#3183`

- The ``zone <name>/<class>: final reference detached`` log message was
  moved from the INFO log level to the DEBUG(1) log level to prevent the
  :iscman:`named-checkzone` tool from superfluously logging this message
  in non-debug mode. :gl:`#3707`

Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.
