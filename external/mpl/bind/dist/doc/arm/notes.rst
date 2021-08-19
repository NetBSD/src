.. 
   Copyright (C) Internet Systems Consortium, Inc. ("ISC")
   
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, you can obtain one at https://mozilla.org/MPL/2.0/.
   
   See the COPYRIGHT file distributed with this work for additional
   information regarding copyright ownership.

Release Notes
=============

.. contents::

Introduction
------------

BIND 9.16 (Extended Support Version) is a stable branch of BIND. This
document summarizes significant changes since the last production
release on that branch. Please see the CHANGES file for a more detailed
list of changes and bug fixes.

Note on Version Numbering
-------------------------

As of BIND 9.13/9.14, BIND has adopted the "odd-unstable/even-stable"
release numbering convention. BIND 9.16 contains new features that were
added during the BIND 9.15 development process. Henceforth, the 9.16
branch will be limited to bug fixes, and new feature development will
proceed in the unstable 9.17 branch.

Supported Platforms
-------------------

To build on Unix-like systems, BIND requires support for POSIX.1c
threads (IEEE Std 1003.1c-1995), the Advanced Sockets API for IPv6
(:rfc:`3542`), and standard atomic operations provided by the C
compiler.

The libuv asynchronous I/O library and the OpenSSL cryptography library
must be available for the target platform. A PKCS#11 provider can be
used instead of OpenSSL for Public Key cryptography (i.e., DNSSEC
signing and validation), but OpenSSL is still required for general
cryptography operations such as hashing and random number generation.

More information can be found in the ``PLATFORMS.md`` file that is
included in the source distribution of BIND 9. If your compiler and
system libraries provide the above features, BIND 9 should compile and
run. If that is not the case, the BIND development team will generally
accept patches that add support for systems that are still supported by
their respective vendors.

Download
--------

The latest versions of BIND 9 software can always be found at
https://www.isc.org/download/. There you will find additional
information about each release, source code, and pre-compiled versions
for Microsoft Windows operating systems.

.. include:: ../notes/notes-9.16.20.rst
.. include:: ../notes/notes-9.16.19.rst
.. include:: ../notes/notes-9.16.18.rst
.. include:: ../notes/notes-9.16.17.rst
.. include:: ../notes/notes-9.16.16.rst
.. include:: ../notes/notes-9.16.15.rst
.. include:: ../notes/notes-9.16.14.rst
.. include:: ../notes/notes-9.16.13.rst
.. include:: ../notes/notes-9.16.12.rst
.. include:: ../notes/notes-9.16.11.rst
.. include:: ../notes/notes-9.16.10.rst
.. include:: ../notes/notes-9.16.9.rst
.. include:: ../notes/notes-9.16.8.rst
.. include:: ../notes/notes-9.16.7.rst
.. include:: ../notes/notes-9.16.6.rst
.. include:: ../notes/notes-9.16.5.rst
.. include:: ../notes/notes-9.16.4.rst
.. include:: ../notes/notes-9.16.3.rst
.. include:: ../notes/notes-9.16.2.rst
.. include:: ../notes/notes-9.16.1.rst
.. include:: ../notes/notes-9.16.0.rst

.. _relnotes_license:

License
-------

BIND 9 is open source software licensed under the terms of the Mozilla
Public License, version 2.0 (see the ``LICENSE`` file for the full
text).

The license requires that if you make changes to BIND and distribute
them outside your organization, those changes must be published under
the same license. It does not require that you publish or disclose
anything other than the changes you have made to our software. This
requirement does not affect anyone who is using BIND, with or without
modifications, without redistributing it, nor anyone redistributing BIND
without changes.

Those wishing to discuss license compliance may contact ISC at
https://www.isc.org/contact/.

End of Life
-----------

BIND 9.16 (Extended Support Version) will be supported until at least
December, 2023. See https://kb.isc.org/docs/aa-00896 for details of
ISC's software support policy.

Thank You
---------

Thank you to everyone who assisted us in making this release possible.
