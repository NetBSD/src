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

Notes for BIND 9.18.10
----------------------

Feature Changes
~~~~~~~~~~~~~~~

- To reduce unnecessary memory consumption in the cache, NXDOMAIN
  records are no longer retained past the normal negative cache TTL,
  even if :any:`stale-cache-enable` is set to ``yes``. :gl:`#3386`

- The :any:`auto-dnssec` option has been deprecated and will be removed
  in a future BIND 9.19.x release. Please migrate to
  :any:`dnssec-policy`. :gl:`#3667`

- The :any:`coresize`, :any:`datasize`, :any:`files`, and
  :any:`stacksize` options have been deprecated. The limits these
  options set should be enforced externally, either by manual
  configuration (e.g. using ``ulimit``) or via the process supervisor
  (e.g. ``systemd``). :gl:`#3676`

- Setting alternate local addresses for inbound zone transfers has been
  deprecated. The relevant options (:any:`alt-transfer-source`,
  :any:`alt-transfer-source-v6`, and :any:`use-alt-transfer-source`)
  will be removed in a future BIND 9.19.x release. :gl:`#3694`

- The number of HTTP headers allowed in requests sent to
  :iscman:`named`'s statistics channel has been increased from 10 to
  100, to accommodate some browsers that send more than 10 headers
  by default. :gl:`#3670`

Bug Fixes
~~~~~~~~~

- :iscman:`named` could crash due to an assertion failure when an HTTP
  connection to the statistics channel was closed prematurely (due to a
  connection error, shutdown, etc.). This has been fixed. :gl:`#3693`

- When a catalog zone was removed from the configuration, in some cases
  a dangling pointer could cause the :iscman:`named` process to crash.
  This has been fixed. :gl:`#3683`

- When a zone was deleted from a server, a key management object related
  to that zone was inadvertently kept in memory and only released upon
  shutdown. This could lead to constantly increasing memory use on
  servers with a high rate of changes affecting the set of zones being
  served. This has been fixed. :gl:`#3727`

- TLS configuration for primary servers was not applied for zones that
  were members of a catalog zone. This has been fixed. :gl:`#3638`

- In certain cases, :iscman:`named` waited for the resolution of
  outstanding recursive queries to finish before shutting down. This was
  unintended and has been fixed. :gl:`#3183`

- :iscman:`host` and :iscman:`nslookup` command-line options setting the
  custom TCP/UDP port to use were ignored for ANY queries (which are
  sent over TCP). This has been fixed. :gl:`#3721`

- The ``zone <name>/<class>: final reference detached`` log message was
  moved from the INFO log level to the DEBUG(1) log level to prevent the
  :iscman:`named-checkzone` tool from superfluously logging this message
  in non-debug mode. :gl:`#3707`

Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.
