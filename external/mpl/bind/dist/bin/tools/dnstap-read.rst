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

.. iscman:: dnstap-read
.. program:: dnstap-read
.. _man_dnstap-read:

dnstap-read - print dnstap data in human-readable form
------------------------------------------------------

Synopsis
~~~~~~~~

:program:`dnstap-read` [**-m**] [**-p**] [**-x**] [**-y**] {file}

Description
~~~~~~~~~~~

:program:`dnstap-read` reads ``dnstap`` data from a specified file and prints
it in a human-readable format. By default, ``dnstap`` data is printed in
a short summary format, but if the :option:`-y` option is specified, a
longer and more detailed YAML format is used.

Options
~~~~~~~

.. option:: -m

   This option indicates trace memory allocations, and is used for debugging memory leaks.

.. option:: -p

   This option prints the text form of the DNS
   message that was encapsulated in the ``dnstap`` frame, after printing the ``dnstap`` data.

.. option:: -x

   This option prints a hex dump of the wire form
   of the DNS message that was encapsulated in the ``dnstap`` frame, after printing the ``dnstap`` data.

.. option:: -y

   This option prints ``dnstap`` data in a detailed YAML format.

See Also
~~~~~~~~

:iscman:`named(8) <named>`, :iscman:`rndc(8) <rndc>`, BIND 9 Administrator Reference Manual.
