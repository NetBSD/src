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

.. _tsig:

TSIG
----

TSIG (Transaction SIGnatures) is a mechanism for authenticating DNS
messages, originally specified in :rfc:`2845`. It allows DNS messages to be
cryptographically signed using a shared secret. TSIG can be used in any
DNS transaction, as a way to restrict access to certain server functions
(e.g., recursive queries) to authorized clients when IP-based access
control is insufficient or needs to be overridden, or as a way to ensure
message authenticity when it is critical to the integrity of the server,
such as with dynamic UPDATE messages or zone transfers from a primary to
a secondary server.

This section is a guide to setting up TSIG in BIND. It describes the
configuration syntax and the process of creating TSIG keys.

:iscman:`named` supports TSIG for server-to-server communication, and some of
the tools included with BIND support it for sending messages to
:iscman:`named`:

   * :ref:`man_nsupdate` supports TSIG via the :option:`-k <nsupdate -k>`, :option:`-l <nsupdate -l>`, and :option:`-y <nsupdate -y>` command-line options, or via the ``key`` command when running interactively.
   * :ref:`man_dig` supports TSIG via the :option:`-k <dig -k>` and :option:`-y <dig -y>` command-line options.

Generating a Shared Key
~~~~~~~~~~~~~~~~~~~~~~~

TSIG keys can be generated using the :iscman:`tsig-keygen` command; the output
of the command is a ``key`` directive suitable for inclusion in
:iscman:`named.conf`. The key name, algorithm, and size can be specified by
command-line parameters; the defaults are "tsig-key", HMAC-SHA256, and
256 bits, respectively.

Any string which is a valid DNS name can be used as a key name. For
example, a key to be shared between servers called ``host1`` and ``host2``
could be called "host1-host2.", and this key can be generated using:

::

     $ tsig-keygen host1-host2. > host1-host2.key

This key may then be copied to both hosts. The key name and secret must
be identical on both hosts. (Note: copying a shared secret from one
server to another is beyond the scope of the DNS. A secure transport
mechanism should be used: secure FTP, SSL, ssh, telephone, encrypted
email, etc.)

:iscman:`tsig-keygen` can also be run as :iscman:`ddns-confgen`, in which case its
output includes additional configuration text for setting up dynamic DNS
in :iscman:`named`. See :ref:`man_ddns-confgen` for details.

Loading a New Key
~~~~~~~~~~~~~~~~~

For a key shared between servers called ``host1`` and ``host2``, the
following could be added to each server's :iscman:`named.conf` file:

::

   key "host1-host2." {
       algorithm hmac-sha256;
       secret "DAopyf1mhCbFVZw7pgmNPBoLUq8wEUT7UuPoLENP2HY=";
   };

(This is the same key generated above using :iscman:`tsig-keygen`.)

Since this text contains a secret, it is recommended that either
:iscman:`named.conf` not be world-readable, or that the ``key`` directive be
stored in a file which is not world-readable and which is included in
:iscman:`named.conf` via the ``include`` directive.

Once a key has been added to :iscman:`named.conf` and the server has been
restarted or reconfigured, the server can recognize the key. If the
server receives a message signed by the key, it is able to verify
the signature. If the signature is valid, the response is signed
using the same key.

TSIG keys that are known to a server can be listed using the command
:option:`rndc tsig-list`.

Instructing the Server to Use a Key
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A server sending a request to another server must be told whether to use
a key, and if so, which key to use.

For example, a key may be specified for each server in the :any:`primaries`
statement in the definition of a secondary zone; in this case, all SOA QUERY
messages, NOTIFY messages, and zone transfer requests (AXFR or IXFR)
are signed using the specified key. Keys may also be specified in
the :any:`also-notify` statement of a primary or secondary zone, causing NOTIFY
messages to be signed using the specified key.

Keys can also be specified in a :namedconf:ref:`server` directive. Adding the
following on ``host1``, if the IP address of ``host2`` is 10.1.2.3, would
cause *all* requests from ``host1`` to ``host2``, including normal DNS
queries, to be signed using the ``host1-host2.`` key:

::

   server 10.1.2.3 {
       keys { host1-host2. ;};
   };

Multiple keys may be present in the :any:`keys` statement, but only the
first one is used. As this directive does not contain secrets, it can be
used in a world-readable file.

Requests sent by ``host2`` to ``host1`` would *not* be signed, unless a
similar ``server`` directive were in ``host2``'s configuration file.

When any server sends a TSIG-signed DNS request, it expects the
response to be signed with the same key. If a response is not signed, or
if the signature is not valid, the response is rejected.

TSIG-Based Access Control
~~~~~~~~~~~~~~~~~~~~~~~~~

TSIG keys may be specified in ACL definitions and ACL directives such as
:any:`allow-query`, :any:`allow-transfer`, and :any:`allow-update`. The above key
would be denoted in an ACL element as ``key host1-host2.``

Here is an example of an :any:`allow-update` directive using a TSIG key:

::

   allow-update { !{ !localnets; any; }; key host1-host2. ;};

This allows dynamic updates to succeed only if the UPDATE request comes
from an address in ``localnets``, *and* if it is signed using the
``host1-host2.`` key.

See :ref:`dynamic_update_policies` for a
discussion of the more flexible :any:`update-policy` statement.

Errors
~~~~~~

Processing of TSIG-signed messages can result in several errors:

-  If a TSIG-aware server receives a message signed by an unknown key,
   the response will be unsigned, with the TSIG extended error code set
   to BADKEY.
-  If a TSIG-aware server receives a message from a known key but with
   an invalid signature, the response will be unsigned, with the TSIG
   extended error code set to BADSIG.
-  If a TSIG-aware server receives a message with a time outside of the
   allowed range, the response will be signed but the TSIG extended
   error code set to BADTIME, and the time values will be adjusted so
   that the response can be successfully verified.

In all of the above cases, the server returns a response code of
NOTAUTH (not authenticated).
