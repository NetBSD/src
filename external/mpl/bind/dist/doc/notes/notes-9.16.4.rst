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

Notes for BIND 9.16.4
---------------------

Security Fixes
~~~~~~~~~~~~~~

-  It was possible to trigger an assertion when attempting to fill an
   oversized TCP buffer. This was disclosed in CVE-2020-8618.
   :gl:`#1850`

-  It was possible to trigger an INSIST failure when a zone with an
   interior wildcard label was queried in a certain pattern. This was
   disclosed in CVE-2020-8619. :gl:`#1111` :gl:`#1718`

New Features
~~~~~~~~~~~~

-  Documentation was converted from DocBook to reStructuredText. The
   BIND 9 ARM is now generated using Sphinx and published on `Read the
   Docs`_. Release notes are no longer available as a separate document
   accompanying a release. :gl:`#83`

-  ``named`` and ``named-checkzone`` now reject master zones that have a
   DS RRset at the zone apex. Attempts to add DS records at the zone
   apex via UPDATE will be logged but otherwise ignored. DS records
   belong in the parent zone, not at the zone apex. :gl:`#1798`

-  ``dig`` and other tools can now print the Extended DNS Error (EDE)
   option when it appears in a request or a response. :gl:`#1835`

Feature Changes
~~~~~~~~~~~~~~~

-  The default value of ``max-stale-ttl`` has changed from 1 week to 12
   hours. This option controls how long ``named`` retains expired RRsets
   in cache as a potential mitigation mechanism, should there be a
   problem with one or more domains. Note that cache content retention
   is independent of whether stale answers are used in response to
   client queries (``stale-answer-enable yes|no`` and ``rndc serve-stale
   on|off``). Serving of stale answers when the authoritative servers
   are not responding must be explicitly enabled, whereas the retention
   of expired cache content takes place automatically on all versions of
   BIND 9 that have this feature available. :gl:`#1877`

   .. warning::
       This change may be significant for administrators who expect that
       stale cache content will be automatically retained for up to 1
       week. Add option ``max-stale-ttl 1w;`` to ``named.conf`` to keep
       the previous behavior of ``named``.

-  ``listen-on-v6 { any; }`` creates a separate socket for each
   interface. Previously, just one socket was created on systems
   conforming to :rfc:`3493` and :rfc:`3542`. This change was introduced
   in BIND 9.16.0, but it was accidentally omitted from documentation.
   :gl:`#1782`

Bug Fixes
~~~~~~~~~

-  When fully updating the NSEC3 chain for a large zone via IXFR, a
   temporary loss of performance could be experienced on the secondary
   server when answering queries for nonexistent data that required
   DNSSEC proof of non-existence (in other words, queries that required
   the server to find and to return NSEC3 data). The unnecessary
   processing step that was causing this delay has now been removed.
   :gl:`#1834`

-  ``named`` could crash with an assertion failure if the name of a
   database node was looked up while the database was being modified.
   :gl:`#1857`

-  A possible deadlock in ``lib/isc/unix/socket.c`` was fixed.
   :gl:`#1859`

-  Previously, ``named`` did not destroy some mutexes and conditional
   variables in netmgr code, which caused a memory leak on FreeBSD. This
   has been fixed. :gl:`#1893`

-  A data race in ``lib/dns/resolver.c:log_formerr()`` that could lead
   to an assertion failure was fixed. :gl:`#1808`

-  Previously, ``provide-ixfr no;`` failed to return up-to-date
   responses when the serial number was greater than or equal to the
   current serial number. :gl:`#1714`

-  A bug in dnssec-policy keymgr was fixed, where the check for the
   existence of a given key's successor would incorrectly return
   ``true`` if any other key in the keyring had a successor. :gl:`#1845`

-  With dnssec-policy, when creating a successor key, the "goal" state
   of the current active key (the predecessor) was not changed and thus
   never removed from the zone. :gl:`#1846`

-  ``named-checkconf -p`` could include spurious text in
   ``server-addresses`` statements due to an uninitialized DSCP value.
   This has been fixed. :gl:`#1812`

-  The ARM has been updated to indicate that the TSIG session key is
   generated when named starts, regardless of whether it is needed.
   :gl:`#1842`

Known Issues
~~~~~~~~~~~~

-  There are no new known issues with this release. See :ref:`above
   <relnotes_known_issues>` for a list of all known issues affecting
   this BIND 9 branch.

.. _Read the Docs: https://bind9.readthedocs.io/
