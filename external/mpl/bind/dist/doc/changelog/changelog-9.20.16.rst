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

BIND 9.20.16
------------

Feature Changes
~~~~~~~~~~~~~~~

- Fix assertion failure from arc4random_uniform with invalid limit.
  ``1040282de7e``

  When the arc4random_uniform() is called on NetBSD with upper_bound
  that makes no sense statistically (0 or 1), the call crashes the
  calling program.  Fix this by returning 0 when upper bound is < 2 as
  does Linux, FreeBSD and NetBSD.  (Hint: System CSPRNG should never
  crash.) :gl:`#5596` :gl:`!11151`

Bug Fixes
~~~~~~~~~

- Fix dnssec-keygen key collision checking for KEY rrtype keys.
  ``ac8b23b80bf``

  The :iscman:`dnssec-keygen` utility program failed to detect possible
  Key ID collisions with the existing keys generated using the
  non-default ``-T KEY`` option (e.g. for ``SIG(0)``). This has been
  fixed. :gl:`#5506` :gl:`!11128`

- Fix shutdown INSIST in dns_dispatchmgr_getblackhole. ``f0aaaef166c``

  Previously, `named` could trigger an assertion in
  `dns_dispatchmgr_getblackhole` while shutting down. This has been
  fixed. :gl:`#5525` :gl:`!11162`

- Dnssec-verify now uses exit code 1 when failing due to illegal
  options. ``6ead0aa4a2b``

  Previously, dnssec-verify exited with code 0 if the options could not
  be parsed. This has been fixed. :gl:`#5574` :gl:`!11129`

- Prevent assertion failures of dig when server is specified before the
  -b option. ``deada63e2b2``

  Previously, :iscman:`dig` could exit with an assertion failure when
  the server was specified before the :option:`dig -b` option. This has
  been fixed. :gl:`#5609` :gl:`!11204`

- Skip unsupported algorithms when looking for signing key.
  ``c346fe88a1b``

  A mix of supported and unsupported DNSSEC algorithms in the same zone
  could have caused validation failures. Ignore the DNSSEC keys with
  unsupported algorithm when looking for the signing keys. :gl:`#5622`
  :gl:`!11210`

- Fix configuration bugs involving global defaults. ``a85d6fb581c``

  The configuration code for the `max-cache-size`, `dnssec-validation`,
  and `response-padding` options were unnecessarily complicated, and in
  the case of `max-cache-size`, buggy. These have been fixed. The
  `optionmaps` variable in `configure_view()` is no longer needed and
  has been removed. :gl:`!11172`

- Skip buffer allocations if not logging. ``4f601175bd0``

  Currently, during IXFR we allocate a 2KB buffer for IXFR change
  logging regardless of the log level. This commit introduces an early
  check on the log level in dns_diff_print to avoid this.

  Results in a speedup from 28% in the test case from issue #5442.
  :gl:`!11192`


