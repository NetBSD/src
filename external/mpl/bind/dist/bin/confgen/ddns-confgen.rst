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

.. BEWARE: Do not forget to edit also tsig-keygen.rst!

.. iscman:: ddns-confgen
.. program:: ddns-confgen
.. _man_ddns-confgen:

ddns-confgen - TSIG key generation tool
---------------------------------------

Synopsis
~~~~~~~~
:program:`ddns-confgen` [**-a** algorithm] [**-h**] [**-k** keyname] [**-q**] [**-s** name] [**-z** zone]

Description
~~~~~~~~~~~

:program:`ddns-confgen` is an utility that generates keys for use in TSIG signing.
The resulting keys can be used, for example, to secure dynamic DNS updates
to a zone, or for the :iscman:`rndc` command channel.

The key name can specified using :option:`-k` parameter and defaults to ``ddns-key``.
The generated key is accompanied by configuration text and instructions that
can be used with :iscman:`nsupdate` and :iscman:`named` when setting up dynamic DNS,
including an example ``update-policy`` statement.
(This usage is similar to the :iscman:`rndc-confgen` command for setting up
command-channel security.)

Note that :iscman:`named` itself can configure a local DDNS key for use with
:option:`nsupdate -l`; it does this when a zone is configured with
``update-policy local;``. :program:`ddns-confgen` is only needed when a more
elaborate configuration is required: for instance, if :iscman:`nsupdate` is to
be used from a remote system.

Options
~~~~~~~

.. option:: -a algorithm

   This option specifies the algorithm to use for the TSIG key. Available
   choices are: hmac-md5, hmac-sha1, hmac-sha224, hmac-sha256, hmac-sha384,
   and hmac-sha512. The default is hmac-sha256. Options are
   case-insensitive, and the "hmac-" prefix may be omitted.

.. option:: -h

   This option prints a short summary of options and arguments.

.. option:: -k keyname

   This option specifies the key name of the DDNS authentication key. The
   default is ``ddns-key`` when neither the :option:`-s` nor :option:`-z` option is
   specified; otherwise, the default is ``ddns-key`` as a separate label
   followed by the argument of the option, e.g., ``ddns-key.example.com.``
   The key name must have the format of a valid domain name, consisting of
   letters, digits, hyphens, and periods.

.. option:: -q

   This option enables quiet mode, which prints only the key, with no
   explanatory text or usage examples. This is essentially identical to
   :iscman:`tsig-keygen`.

.. option:: -s name

   This option generates a configuration example to allow dynamic updates
   of a single hostname. The example :iscman:`named.conf` text shows how to set
   an update policy for the specified name using the "name" nametype. The
   default key name is ``ddns-key.name``. Note that the "self" nametype
   cannot be used, since the name to be updated may differ from the key
   name. This option cannot be used with the :option:`-z` option.

.. option:: -z zone

   This option generates a configuration example to allow
   dynamic updates of a zone. The example :iscman:`named.conf` text shows how
   to set an update policy for the specified zone using the "zonesub"
   nametype, allowing updates to all subdomain names within that zone.
   This option cannot be used with the :option:`-s` option.

See Also
~~~~~~~~

:iscman:`nsupdate(1) <nsupdate>`, :iscman:`named.conf(5) <named.conf>`, :iscman:`named(8) <named>`, BIND 9 Administrator Reference Manual.
