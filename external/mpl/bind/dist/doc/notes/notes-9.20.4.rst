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

Notes for BIND 9.20.4
---------------------

New Features
~~~~~~~~~~~~

- Update built-in :file:`bind.keys` file with the new 2025 `IANA root key
  <https://www.iana.org/dnssec/files>`_.

  Add an `initial-ds` entry to :file:`bind.keys` for the new root key, ID
  38696, which is scheduled for publication in January 2025. :gl:`#4896`

Removed Features
~~~~~~~~~~~~~~~~

- Move contributed DLZ modules into a separate repository. DLZ modules should
  not be used except in testing.

  The DLZ modules were not maintained, the DLZ interface itself is going to be
  scheduled for removal, and the DLZ interface is blocking. Any module that
  blocks the query to the :namedconf:ref:`database` blocks the whole server.

  The DLZ modules now live in
  https://gitlab.isc.org/isc-projects/dlz-modules repository.
  :gl:`#4865`

Feature Changes
~~~~~~~~~~~~~~~

- :iscman:`dnssec-ksr` now supports KSK rollovers.

  The tool now allows for KSK generation, as well as planned KSK rollovers.
  When signing a bundle from a Key Signing Request (KSR), only the
  key that is active in that time frame is
  used for signing. Also, the CDS and CDNSKEY records are now added and
  removed at the correct time. :gl:`#4697`  :gl:`#4705`

- Print :rfc:`7314`: EXPIRE option in transfer summary. :gl:`#5013`

- Emit more helpful log messages for exceeding :namedconf:ref:`max-records-per-type`.

  The new log message is emitted when adding or updating an RRset fails
  due to exceeding the :namedconf:ref:`max-records-per-type` limit. The log includes the
  owner name and type, corresponding zone name, and the limit value. It
  will be emitted on loading a zone file, inbound zone transfer (both
  AXFR and IXFR), handling a DDNS update, or updating a cache DB. It's
  especially helpful in the case of zone transfer, since the secondary
  side doesn't have direct access to the offending zone data.

  It could also be used for :namedconf:ref:`max-types-per-name`, but this change doesn't
  implement it yet as it's much less likely to happen in practice.

- Harden key management when key files have become unavailable.

  Prior to doing key management, BIND 9 will check if the key files on
  disk match the expected keys. If key files for previously observed
  keys have become unavailable, this will prevent the internal key
  manager from running.

Bug Fixes
~~~~~~~~~

- Use TLS for notifies if configured to do so.

  Notifies configured to use TLS will now be sent over TLS, instead of
  plain text UDP or TCP. Also, failing to load the TLS configuration for
  :namedconf:ref:`notify` now results in an error. :gl:`#4821`

- `{&dns}` is as valid as `{?dns}` in a SVCB's dohpath.

  :iscman:`dig` failed to parse a valid `SVCB` record with a `dohpath` URI
  template containing a `{&dns}`, like `dohpath=/some/path?key=value{&dns}"`.
  :gl:`#4922`

- Fix NSEC3 closest encloser lookup for names with empty non-terminals.

  A previous performance optimization for finding the NSEC3 closest encloser
  when generating authoritative responses could cause servers to return
  incorrect NSEC3 records in some cases. This has been fixed.
  :gl:`#4950`

- :namedconf:ref:`recursive-clients` statement with value 0 triggered an assertion failure.

  BIND 9.20.0 broke `recursive-clients 0;`.  This has now been fixed.
  :gl:`#4987`

- Parsing of hostnames in :iscman:`rndc.conf` was broken.

  When DSCP support was removed, parsing of hostnames in :iscman:`rndc.conf` was
  accidentally broken, resulting in an assertion failure.  This has been
  fixed. :gl:`#4991`

- :iscman:`dig` options of the form `[+-]option=<value>` failed to display the
  value on the printed command line. This has been fixed. :gl:`#4993`

- Provide more visibility into TLS configuration errors by logging
  `SSL_CTX_use_certificate_chain_file()` and `SSL_CTX_use_PrivateKey_file()`
  errors individually. :gl:`#5008`

- Fix a race condition when canceling ADB find which could cause an assertion
  failure. :gl:`#5024`

- SERVFAIL cache memory cleaning is now more aggressive; it no longer consumes a
  lot of memory if the server encounters many SERVFAILs at once.
  :gl:`#5025`

- Fix trying the next primary XoT server when the previous one was marked as
  unreachable.

  In some cases :iscman:`named` failed to try the next primary
  server in the :namedconf:ref:`primaries` list when the previous one was marked as
  unreachable. This has been fixed. :gl:`#5038`
