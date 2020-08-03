.. 
   Copyright (C) Internet Systems Consortium, Inc. ("ISC")
   
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/.
   
   See the COPYRIGHT file distributed with this work for additional
   information regarding copyright ownership.

..
   Copyright (C) Internet Systems Consortium, Inc. ("ISC")

   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/.

   See the COPYRIGHT file distributed with this work for additional
   information regarding copyright ownership.


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
a five second delay to allow the user to interrupt the process before
the destruction takes place.

Arguments
~~~~~~~~~

**-m** module
   Specify the PKCS#11 provider module. This must be the full path to a
   shared library object implementing the PKCS#11 API for the device.

**-s** slot
   Open the session with the given PKCS#11 slot. The default is slot 0.

**-i** ID
   Destroy keys with the given object ID.

**-l** label
   Destroy keys with the given label.

**-p** PIN
   Specify the PIN for the device. If no PIN is provided on the command
   line, ``pkcs11-destroy`` will prompt for it.

**-w** seconds
   Specify how long to pause before carrying out key destruction. The
   default is five seconds. If set to ``0``, destruction will be
   immediate.

See Also
~~~~~~~~

:manpage:`pkcs11-keygen(8)`, :manpage:`pkcs11-list(8)`, :manpage:`pkcs11-tokens(8)`
