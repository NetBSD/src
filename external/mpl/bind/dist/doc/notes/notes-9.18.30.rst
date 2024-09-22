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

Notes for BIND 9.18.30
----------------------

New Features
~~~~~~~~~~~~

- Print the full path of the working directory in startup log messages.

  :iscman:`named` now prints its initial working directory during
  startup, and the changed working directory when loading or reloading
  its configuration file, if it has a valid :any:`directory` option
  defined. :gl:`#4731`

Feature Changes
~~~~~~~~~~~~~~~

- Follow the number of CPUs set by ``taskset``/``cpuset``.

  Administrators may wish to constrain the set of cores that
  :iscman:`named` runs on via the ``taskset``, ``cpuset``, or ``numactl``
  programs (or equivalents on other OSes).

  If the admin has used ``taskset``, :iscman:`named` now automatically
  uses the given number of CPUs rather than the system-wide count.
  :gl:`#4884`

Bug Fixes
~~~~~~~~~

- Verification of the privacy of an EDDSA key was broken.

  The check could lead to an attempt to sign records with a public key,
  which could cause a segmentation failure (read of a NULL pointer)
  within OpenSSL. This has been fixed. :gl:`#4855`

- Fix algorithm rollover bug when there are two keys with the same
  keytag.

  If there was an algorithm rollover and two keys of different
  algorithms shared the same keytags, there was the possibility that the
  check of whether the key matched a specific state could be performed
  against the wrong key. This has been fixed by not only checking for
  the matching key tag but also the key algorithm. :gl:`#4878`

Known Issues
~~~~~~~~~~~~

- There are no new known issues with this release. See :ref:`above
  <relnotes_known_issues>` for a list of all known issues affecting this
  BIND 9 branch.
