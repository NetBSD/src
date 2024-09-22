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

Notes for BIND 9.18.28
----------------------

Security Fixes
~~~~~~~~~~~~~~

- A malicious DNS client that sent many queries over TCP but never read
  the responses could cause a server to respond slowly or not at all for
  other clients. This has been fixed. :cve:`2024-0760` :gl:`#4481`

- It is possible to craft excessively large resource records sets, which
  have the effect of slowing down database processing. This has been
  addressed by adding a configurable limit to the number of records that
  can be stored per name and type in a cache or zone database. The
  default is 100, which can be tuned with the new
  :any:`max-records-per-type` option. :gl:`#497` :gl:`#3405`

  It is possible to craft excessively large numbers of resource record
  types for a given owner name, which has the effect of slowing down
  database processing. This has been addressed by adding a configurable
  limit to the number of records that can be stored per name and type in
  a cache or zone database.  The default is 100, which can be tuned with
  the new :any:`max-types-per-name` option. :cve:`2024-1737` :gl:`#3403`

  ISC would like to thank Toshifumi Sakaguchi who independently
  discovered and responsibly reported the issue to ISC. :gl:`#4548`

- Validating DNS messages signed using the SIG(0) protocol (:rfc:`2931`)
  could cause excessive CPU load, leading to a denial-of-service
  condition. Support for SIG(0) message validation was removed from this
  version of :iscman:`named`. :cve:`2024-1975` :gl:`#4480`

- Due to a logic error, lookups that triggered serving stale data and
  required lookups in local authoritative zone data could have resulted
  in an assertion failure. This has been fixed. :cve:`2024-4076`
  :gl:`#4507`

- Potential data races were found in our DoH implementation, related to
  HTTP/2 session object management and endpoints set object management
  after reconfiguration. These issues have been fixed. :gl:`#4473`

  ISC would like to thank Dzintars and Ivo from nic.lv for bringing this
  to our attention.

- When looking up the NS records of parent zones as part of looking up DS
  records, it was possible for :iscman:`named` to trigger an assertion
  failure if serve-stale was enabled. This has been fixed. :gl:`#4661`

Bug Fixes
~~~~~~~~~

- Command-line options for IPv4-only (:option:`named -4`) and IPv6-only
  (:option:`named -6`) modes are now respected for zone :any:`primaries`,
  :any:`also-notify`, and :any:`parental-agents`. :gl:`#3472`

- An RPZ response's SOA record TTL was set to 1 instead of the SOA TTL,
  if ``add-soa`` was used. This has been fixed. :gl:`#3323`

- When a query related to zone maintenance (NOTIFY, SOA) timed out close
  to a view shutdown (triggered e.g. by :option:`rndc reload`),
  :iscman:`named` could crash with an assertion failure. This has been
  fixed. :gl:`#4719`

- The statistics channel counters that indicated the number of currently
  connected TCP IPv4/IPv6 clients were not properly adjusted in certain
  failure scenarios. This has been fixed. :gl:`#4742`

- Some servers that could not be reached due to EHOSTDOWN or ENETDOWN
  conditions were incorrectly prioritized during server selection. These
  are now properly handled as unreachable. :gl:`#4736`

- On some systems the libuv call may return an error code when sending a
  TCP reset for a connection, which triggers an assertion failure in
  :iscman:`named`. This error condition is now dealt with in a more
  graceful manner, by logging the incident and shutting down the
  connection. :gl:`#4708`

Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.
