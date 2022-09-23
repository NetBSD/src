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

.. _man_pkcs11-list:

pkcs11-list - list PKCS#11 objects
----------------------------------

:program:`pkcs11-list` [**-P**] [**-m** module] [**-s** slot] [**-i** ID **] [-l** label] [**-p** PIN]

Description
~~~~~~~~~~~

``pkcs11-list`` lists the PKCS#11 objects with ``ID`` or ``label`` or, by
default, all objects. The object class, label, and ID are displayed for
all keys. For private or secret keys, the extractability attribute is
also displayed, as either ``true``, ``false``, or ``never``.

Options
~~~~~~~

``-P``
   This option lists only the public objects. (Note that on some PKCS#11 devices, all
   objects are private.)

``-m module``
   This option specifies the PKCS#11 provider module. This must be the full path to a
   shared library object implementing the PKCS#11 API for the device.

``-s slot``
   This option opens the session with the given PKCS#11 slot. The default is slot 0.

``-i ID``
   This option lists only key objects with the given object ID.

``-l label``
   This option lists only key objects with the given label.

``-p PIN``
   This option specifies the ``PIN`` for the device. If no ``PIN`` is provided on the command
   line, ``pkcs11-list`` prompts for it.

See Also
~~~~~~~~

:manpage:`pkcs11-destroy(8)`, :manpage:`pkcs11-keygen(8)`, :manpage:`pkcs11-tokens(8)`
