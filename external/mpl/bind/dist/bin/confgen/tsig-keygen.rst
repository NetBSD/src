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

.. BEWARE: Do not forget to edit also ddns-confgen.rst!

.. _man_tsig-keygen:

tsig-keygen - TSIG key generation tool
--------------------------------------

Synopsis
~~~~~~~~
:program:`tsig-keygen` [**-a** algorithm] [**-h**] [name]

Description
~~~~~~~~~~~

``tsig-keygen`` is an utility that generates keys for use in TSIG signing.
The resulting keys can be used, for example, to secure dynamic DNS updates
to a zone, or for the ``rndc`` command channel.

A domain name can be specified on the command line to be used as the name
of the generated key. If no name is specified, the default is ``tsig-key``.

Options
~~~~~~~

``-a algorithm``
   This option specifies the algorithm to use for the TSIG key. Available
   choices are: hmac-md5, hmac-sha1, hmac-sha224, hmac-sha256, hmac-sha384,
   and hmac-sha512. The default is hmac-sha256. Options are
   case-insensitive, and the "hmac-" prefix may be omitted.

``-h``
   This option prints a short summary of options and arguments.

See Also
~~~~~~~~

:manpage:`nsupdate(1)`, :manpage:`named.conf(5)`, :manpage:`named(8)`, BIND 9 Administrator Reference Manual.
