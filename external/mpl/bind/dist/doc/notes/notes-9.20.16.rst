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

Notes for BIND 9.20.16
----------------------

Bug Fixes
~~~~~~~~~

- Skip unsupported algorithms when looking for a signing key.

  A mix of supported and unsupported DNSSEC algorithms in the same zone
  could cause validation failures. Unsupported algorithms are now
  ignored when looking for signing keys. :gl:`#5622`

- Fix :iscman:`dnssec-keygen` key collision checking for KEY RRtype
  keys.

  The :iscman:`dnssec-keygen` utility program failed to detect possible
  KEY ID collisions with existing keys generated using the non-default
  ``-T KEY`` option (e.g., for ``SIG(0)``). This has been fixed.
  :gl:`#5506`

- :iscman:`dnssec-verify` now uses exit code 1 when failing due to
  illegal options.

  Previously, :iscman:`dnssec-verify` exited with code 0 if the options
  could not be parsed. This has been fixed. :gl:`#5574`

- Prevent assertion failures of :iscman:`dig` when a server is specified
  before the ``-b`` option.

  Previously, :iscman:`dig` could exit with an assertion failure when
  a server was specified before the :option:`dig -b` option. This has
  been fixed. :gl:`#5609`

- Skip buffer allocations if not logging.

  Previously, we allocated a 2KB buffer for IXFR change logging,
  regardless of the log level.

  This results in a 28% speedup in some scenarios.


