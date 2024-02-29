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

.. highlight: console

.. iscman:: nsec3hash
.. program:: nsec3hash
.. _man_nsec3hash:

nsec3hash - generate NSEC3 hash
-------------------------------

Synopsis
~~~~~~~~

:program:`nsec3hash` {salt} {algorithm} {iterations} {domain}

:program:`nsec3hash` **-r** {algorithm} {flags} {iterations} {salt} {domain}

Description
~~~~~~~~~~~

:program:`nsec3hash` generates an NSEC3 hash based on a set of NSEC3
parameters. This can be used to check the validity of NSEC3 records in a
signed zone.

If this command is invoked as ``nsec3hash -r``, it takes arguments in
order, matching the first four fields of an NSEC3 record followed by the
domain name: ``algorithm``, ``flags``, ``iterations``, ``salt``, ``domain``. This makes it
convenient to copy and paste a portion of an NSEC3 or NSEC3PARAM record
into a command line to confirm the correctness of an NSEC3 hash.

Arguments
~~~~~~~~~

.. option:: salt

   This is the salt provided to the hash algorithm.

.. option:: algorithm

   This is a number indicating the hash algorithm. Currently the only supported
   hash algorithm for NSEC3 is SHA-1, which is indicated by the number
   1; consequently "1" is the only useful value for this argument.

.. option:: flags

   This is provided for compatibility with NSEC3 record presentation format, but
   is ignored since the flags do not affect the hash.

.. option:: iterations

   This is the number of additional times the hash should be performed.

.. option:: domain

   This is the domain name to be hashed.

See Also
~~~~~~~~

BIND 9 Administrator Reference Manual, :rfc:`5155`.
