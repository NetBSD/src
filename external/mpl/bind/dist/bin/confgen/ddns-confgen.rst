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

.. _man_ddns-confgen:

ddns-confgen - ddns key generation tool
---------------------------------------

Synopsis
~~~~~~~~
:program:`tsig-keygen` [**-a** algorithm] [**-h**] [**-r** randomfile] [**-s** name]

:program:`ddns-confgen` [**-a** algorithm] [**-h**] [**-k** keyname] [**-q**] [**-r** randomfile] [**-s** name] [**-z** zone]

Description
~~~~~~~~~~~

``tsig-keygen`` and ``ddns-confgen`` are invocation methods for a
utility that generates keys for use in TSIG signing. The resulting keys
can be used, for example, to secure dynamic DNS updates to a zone or for
the ``rndc`` command channel.

When run as ``tsig-keygen``, a domain name can be specified on the
command line which will be used as the name of the generated key. If no
name is specified, the default is ``tsig-key``.

When run as ``ddns-confgen``, the generated key is accompanied by
configuration text and instructions that can be used with ``nsupdate``
and ``named`` when setting up dynamic DNS, including an example
``update-policy`` statement. (This usage similar to the ``rndc-confgen``
command for setting up command channel security.)

Note that ``named`` itself can configure a local DDNS key for use with
``nsupdate -l``: it does this when a zone is configured with
``update-policy local;``. ``ddns-confgen`` is only needed when a more
elaborate configuration is required: for instance, if ``nsupdate`` is to
be used from a remote system.

Options
~~~~~~~

**-a** algorithm
   Specifies the algorithm to use for the TSIG key. Available choices
   are: hmac-md5, hmac-sha1, hmac-sha224, hmac-sha256, hmac-sha384 and
   hmac-sha512. The default is hmac-sha256. Options are
   case-insensitive, and the "hmac-" prefix may be omitted.

**-h**
   Prints a short summary of options and arguments.

**-k** keyname
   Specifies the key name of the DDNS authentication key. The default is
   ``ddns-key`` when neither the ``-s`` nor ``-z`` option is specified;
   otherwise, the default is ``ddns-key`` as a separate label followed
   by the argument of the option, e.g., ``ddns-key.example.com.`` The
   key name must have the format of a valid domain name, consisting of
   letters, digits, hyphens and periods.

**-q**
   (``ddns-confgen`` only.) Quiet mode: Print only the key, with no
   explanatory text or usage examples; This is essentially identical to
   ``tsig-keygen``.

**-s** name
   (``ddns-confgen`` only.) Generate configuration example to allow
   dynamic updates of a single hostname. The example ``named.conf`` text
   shows how to set an update policy for the specified name using the
   "name" nametype. The default key name is ddns-key.name. Note that the
   "self" nametype cannot be used, since the name to be updated may
   differ from the key name. This option cannot be used with the ``-z``
   option.

**-z** zone
   (``ddns-confgen`` only.) Generate configuration example to allow
   dynamic updates of a zone: The example ``named.conf`` text shows how
   to set an update policy for the specified zone using the "zonesub"
   nametype, allowing updates to all subdomain names within that zone.
   This option cannot be used with the ``-s`` option.

See Also
~~~~~~~~

:manpage:`nsupdate(1)`, :manpage:`named.conf(5)`, :manpage:`named(8)`, BIND 9 Administrator Reference Manual.
