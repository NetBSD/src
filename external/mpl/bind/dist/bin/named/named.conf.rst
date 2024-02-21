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

.. iscman:: named.conf

named.conf - configuration file for **named**
---------------------------------------------

Synopsis
~~~~~~~~

:program:`named.conf`

Description
~~~~~~~~~~~

:file:`named.conf` is the configuration file for :iscman:`named`.

For complete documentation about the configuration statements, please refer to
the Configuration Reference section in the BIND 9 Administrator Reference
Manual.

Statements are enclosed in braces and terminated with a semi-colon.
Clauses in the statements are also semi-colon terminated. The usual
comment styles are supported:

C style: /\* \*/

C++ style: // to end of line

Unix style: # to end of line

.. literalinclude:: ../../doc/misc/options

Any of these zone statements can also be set inside the view statement.

.. literalinclude:: ../../doc/misc/primary.zoneopt
.. literalinclude:: ../../doc/misc/secondary.zoneopt
.. literalinclude:: ../../doc/misc/mirror.zoneopt
.. literalinclude:: ../../doc/misc/forward.zoneopt
.. literalinclude:: ../../doc/misc/hint.zoneopt
.. literalinclude:: ../../doc/misc/redirect.zoneopt
.. literalinclude:: ../../doc/misc/static-stub.zoneopt
.. literalinclude:: ../../doc/misc/stub.zoneopt
.. literalinclude:: ../../doc/misc/delegation-only.zoneopt
.. literalinclude:: ../../doc/misc/in-view.zoneopt

Files
~~~~~

|named_conf|

See Also
~~~~~~~~

:iscman:`named(8) <named>`, :iscman:`named-checkconf(8) <named-checkconf>`, :iscman:`rndc(8) <rndc>`, :iscman:`rndc-confgen(8) <rndc-confgen>`, :iscman:`tsig-keygen(8) <tsig-keygen>`, BIND 9 Administrator Reference Manual.

