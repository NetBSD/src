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

Notes for BIND 9.20.2
---------------------

New Features
~~~~~~~~~~~~

- Support for Offline KSK implemented.

  Add a new configuration option :any:`offline-ksk` to enable Offline
  KSK key management. Signed Key Response (SKR) files created with
  :iscman:`dnssec-ksr` (or other programs) can now be imported into
  :iscman:`named` with the new :option:`rndc skr -import <rndc skr>`
  command. Rather than creating new DNSKEY, CDS, and CDNSKEY records and
  generating signatures covering these types, these records are loaded
  from the currently active bundle from the imported SKR.

  The implementation is loosely based on
  `draft-icann-dnssec-keymgmt-01.txt
  <https://web.archive.org/web/20250121040252/https://www.iana.org/dnssec/archive/files/draft-icann-dnssec-keymgmt-01.txt>`_.
  :gl:`#1128`

- Print the full path of the working directory in startup log messages.

  :iscman:`named` now prints its initial working directory during
  startup, and the changed working directory when loading or reloading
  its configuration file, if it has a valid :any:`directory` option
  defined. :gl:`#4731`

- Support a restricted key tag range when generating new keys.

  When multiple signers are being used to sign a zone, it is useful to
  be able to specify a restricted range of key tags to be used by an
  operator to sign the zone. The range can be specified with
  ``tag-range`` in :any:`dnssec-policy`'s :ref:`keys
  <dnssec-policy-keys>` (for :iscman:`named` and :iscman:`dnssec-ksr`)
  and with the new options :option:`dnssec-keyfromlabel -M` and
  :option:`dnssec-keygen -M`. :gl:`#4830`


Feature Changes
~~~~~~~~~~~~~~~

- Exempt prefetches from the :any:`fetches-per-zone` and
  :any:`fetches-per-server` quotas.

  Fetches generated automatically as a result of :any:`prefetch` are now
  exempt from the :any:`fetches-per-zone` and :any:`fetches-per-server`
  quotas. This should help in maintaining the cache from which query
  responses can be given. :gl:`#4219`

- Improve performance for queries that require an NSEC3 wildcard proof.

  Rather than starting from the longest matching part of the requested name,
  lookup the shortest partial match. Most of the time this will be the actual
  closest encloser. :gl:`#4460`

- Follow the number of CPUs set by ``taskset``/``cpuset``.

  Administrators may wish to constrain the set of cores that
  :iscman:`named` runs on via the ``taskset``, ``cpuset``, or ``numactl``
  programs (or equivalents on other OSes).

  If the admin has used ``taskset``, :iscman:`named` now automatically
  uses the given number of CPUs rather than the system-wide count.
  :gl:`#4884`

Bug Fixes
~~~~~~~~~

- Delay the release of root privileges until after configuring controls.

  Delay relinquishing root privileges until the control channel has been
  configured, for the benefit of systems that require root to use
  privileged port numbers.  This mostly affects systems without fine-
  grained privilege systems (i.e., other than Linux). :gl:`#4793`

- Fix a rare assertion failure when shutting down incoming transfer.

  A very rare assertion failure could be triggered when the incoming
  transfer was either forcefully shut down, or it finished during the
  printing of the details about the statistics channel.  This has been
  fixed. :gl:`#4860`

- Fix algorithm rollover bug when there are two keys with the same
  keytag.

  If there was an algorithm rollover and two keys of different
  algorithms shared the same keytags, there was the possibility that the
  check of whether the key matched a specific state could be performed
  against the wrong key. This has been fixed by not only checking for
  the matching key tag but also the key algorithm. :gl:`#4878`

- Fix an assertion failure in ``validate_dnskey_dsset_done()``.

  Under rare circumstances, :iscman:`named` could terminate unexpectedly
  when validating a DNSKEY resource record if the validation had been
  canceled in the meantime. This has been fixed. :gl:`#4911`

Known Issues
~~~~~~~~~~~~

- Long-running tasks in offloaded threads (e.g. the loading of RPZ zones
  or processing zone transfers) may block the resolution of queries
  during these operations and cause the queries to time out.

  To work around the issue, the ``UV_THREADPOOL_SIZE`` environment
  variable can be set to a larger value before starting :iscman:`named`.
  The recommended value is the number of RPZ zones (or number of
  transfers) plus the number of threads BIND should use, which is
  typically the number of CPUs. :gl:`#4898`
