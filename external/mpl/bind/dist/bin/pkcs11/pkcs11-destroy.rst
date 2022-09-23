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

.. _man_pkcs11-destroy:

pkcs11-destroy - destroy PKCS#11 objects

Synopsis
~~~~~~~~

:program:`pkcs11-destroy` [**-m** module] [**-s** slot] [**-i** ID] [**-l** label] [**-p** PIN] [**-w** seconds]

Description
~~~~~~~~~~~

``pkcs11-destroy`` destroys keys stored in a PKCS#11 device, identified
by their ``ID`` or ``label``.

Matching keys are displayed before being destroyed. By default, there is
a five-second delay to allow the user to interrupt the process before
the destruction takes place.

Options
~~~~~~~

``-m module``
   This option specifies the PKCS#11 provider module. This must be the full path to a
   shared library object implementing the PKCS#11 API for the device.

``-s slot``
   This option opens the session with the given PKCS#11 slot. The default is slot 0.

``-i ID``
   This option destroys keys with the given object ID.

``-l label``
   This option destroys keys with the given label.

``-p PIN``
   This option specifies the ``PIN`` for the device. If no ``PIN`` is provided on the command
   line, ``pkcs11-destroy`` prompts for it.

``-w seconds``
   This option specifies how long, in seconds, to pause before carrying out key destruction. The
   default is 5 seconds. If set to ``0``, destruction is
   immediate.

See Also
~~~~~~~~

:manpage:`pkcs11-keygen(8)`, :manpage:`pkcs11-list(8)`, :manpage:`pkcs11-tokens(8)`
