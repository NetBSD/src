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

.. _man_pkcs11-tokens:

pkcs11-tokens - list PKCS#11 available tokens
---------------------------------------------

Synopsis
~~~~~~~~

:program:`pkcs11-tokens` [**-m** module] [**-v**]

Description
~~~~~~~~~~~

``pkcs11-tokens`` lists the PKCS#11 available tokens with defaults from
the slot/token scan performed at application initialization.

Options
~~~~~~~

``-m module``
   This option specifies the PKCS#11 provider module. This must be the full path to a
   shared library object implementing the PKCS#11 API for the device.

``-v``
   This option makes the PKCS#11 libisc initialization verbose.

See Also
~~~~~~~~

:manpage:`pkcs11-destroy(8)`, :manpage:`pkcs11-keygen(8)`, :manpage:`pkcs11-list(8)`
