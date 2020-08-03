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

.. _man_rndc.conf:

rndc.conf - rndc configuration file
-----------------------------------

Synopsis
~~~~~~~~

:program:`rndc.conf`

Description
~~~~~~~~~~~

``rndc.conf`` is the configuration file for ``rndc``, the BIND 9 name
server control utility. This file has a similar structure and syntax to
``named.conf``. Statements are enclosed in braces and terminated with a
semi-colon. Clauses in the statements are also semi-colon terminated.
The usual comment styles are supported:

C style: /\* \*/

C++ style: // to end of line

Unix style: # to end of line

``rndc.conf`` is much simpler than ``named.conf``. The file uses three
statements: an options statement, a server statement and a key
statement.

The ``options`` statement contains five clauses. The ``default-server``
clause is followed by the name or address of a name server. This host
will be used when no name server is given as an argument to ``rndc``.
The ``default-key`` clause is followed by the name of a key which is
identified by a ``key`` statement. If no ``keyid`` is provided on the
rndc command line, and no ``key`` clause is found in a matching
``server`` statement, this default key will be used to authenticate the
server's commands and responses. The ``default-port`` clause is followed
by the port to connect to on the remote name server. If no ``port``
option is provided on the rndc command line, and no ``port`` clause is
found in a matching ``server`` statement, this default port will be used
to connect. The ``default-source-address`` and
``default-source-address-v6`` clauses which can be used to set the IPv4
and IPv6 source addresses respectively.

After the ``server`` keyword, the server statement includes a string
which is the hostname or address for a name server. The statement has
three possible clauses: ``key``, ``port`` and ``addresses``. The key
name must match the name of a key statement in the file. The port number
specifies the port to connect to. If an ``addresses`` clause is supplied
these addresses will be used instead of the server name. Each address
can take an optional port. If an ``source-address`` or
``source-address-v6`` of supplied then these will be used to specify the
IPv4 and IPv6 source addresses respectively.

The ``key`` statement begins with an identifying string, the name of the
key. The statement has two clauses. ``algorithm`` identifies the
authentication algorithm for ``rndc`` to use; currently only HMAC-MD5
(for compatibility), HMAC-SHA1, HMAC-SHA224, HMAC-SHA256 (default),
HMAC-SHA384 and HMAC-SHA512 are supported. This is followed by a secret
clause which contains the base-64 encoding of the algorithm's
authentication key. The base-64 string is enclosed in double quotes.

There are two common ways to generate the base-64 string for the secret.
The BIND 9 program ``rndc-confgen`` can be used to generate a random
key, or the ``mmencode`` program, also known as ``mimencode``, can be
used to generate a base-64 string from known input. ``mmencode`` does
not ship with BIND 9 but is available on many systems. See the EXAMPLE
section for sample command lines for each.

Example
~~~~~~~

::

         options {
           default-server  localhost;
           default-key     samplekey;
         };

::

         server localhost {
           key             samplekey;
         };

::

         server testserver {
           key     testkey;
           addresses   { localhost port 5353; };
         };

::

         key samplekey {
           algorithm       hmac-sha256;
           secret          "6FMfj43Osz4lyb24OIe2iGEz9lf1llJO+lz";
         };

::

         key testkey {
           algorithm   hmac-sha256;
           secret      "R3HI8P6BKw9ZwXwN3VZKuQ==";
         };


In the above example, ``rndc`` will by default use the server at
localhost (127.0.0.1) and the key called samplekey. Commands to the
localhost server will use the samplekey key, which must also be defined
in the server's configuration file with the same name and secret. The
key statement indicates that samplekey uses the HMAC-SHA256 algorithm
and its secret clause contains the base-64 encoding of the HMAC-SHA256
secret enclosed in double quotes.

If ``rndc -s testserver`` is used then ``rndc`` will connect to server
on localhost port 5353 using the key testkey.

To generate a random secret with ``rndc-confgen``:

``rndc-confgen``

A complete ``rndc.conf`` file, including the randomly generated key,
will be written to the standard output. Commented-out ``key`` and
``controls`` statements for ``named.conf`` are also printed.

To generate a base-64 secret with ``mmencode``:

``echo "known plaintext for a secret" | mmencode``

Name Server Configuration
~~~~~~~~~~~~~~~~~~~~~~~~~

The name server must be configured to accept rndc connections and to
recognize the key specified in the ``rndc.conf`` file, using the
controls statement in ``named.conf``. See the sections on the
``controls`` statement in the BIND 9 Administrator Reference Manual for
details.

See Also
~~~~~~~~

:manpage:`rndc(8)`, :manpage:`rndc-confgen(8)`, :manpage:`mmencode(1)`, BIND 9 Administrator Reference Manual.
