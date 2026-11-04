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

Notes for BIND 9.20.17
----------------------

Feature Changes
~~~~~~~~~~~~~~~

- Reduce the number of outgoing queries.

  Reduce the number of outgoing queries when resolving the nameservers
  for delegation points. This helps a DNS resolver with a cold cache
  resolve client queries with complex delegation chains and
  redirections. :gl:`!11148`

- Provide more information when memory allocation fails.

  BIND now provides more information about the failure when memory allocation
  fails. :gl:`!11272`

Bug Fixes
~~~~~~~~~

- Adding NSEC3 opt-out records could leave invalid records in chain.

  When creating an NSEC3 opt-out chain, a node in the chain could be
  removed too soon. The previous NSEC3 would therefore not be found,
  resulting in invalid NSEC3 records being left in the zone. This has
  been fixed. :gl:`#5671`

- Fix spurious timeouts while resolving names.

  Sometimes, loops in the resolving process (e.g., to resolve or validate
  ``ns1.example.com``, we need to resolve ``ns1.example.com``) were not properly
  detected, leading to a spurious 10-second delay. This has been fixed,
  and such loops are properly detected. :gl:`#3033` :gl:`#5578`

- Fix bug where zone switches from NSEC3 to NSEC after retransfer.

  When a zone was re-transferred but the zone journal on an
  inline-signing secondary was out of sync, the zone could fall back to
  using NSEC records instead of NSEC3. This has been fixed. :gl:`#5527`

- ``AMTRELAY`` type 0 presentation format handling was wrong.

  :rfc:`8777` specifies a placeholder value of ``.`` for the gateway field
  when the gateway type is 0 (no gateway). This was not being checked
  for, nor was it emitted when displaying the record. This has been corrected.

  Instances of this record will need the placeholder period added to
  them when upgrading. :gl:`#5639`

- Fix parsing bug in :any:`remote-servers` with key or TLS.

  The :any:`remote-servers` clause enables the following pattern using a
  named ``server-list``::

      remote-servers a { 1.2.3.4; ... };
      remote-servers b { a key foo; };

  However, such a configuration was wrongly rejected, with an ``unexpected
  token 'foo'`` error. This configuration is now accepted. :gl:`#5646`

- Fix DoT reconfigure/reload bug in the resolver.

  If client-side TLS transport was in use (for example, when
  forwarding queries to a DoT server), :iscman:`named` could
  terminate unexpectedly when reconfiguring or reloading. This
  has been fixed.
  :gl:`#5653`

