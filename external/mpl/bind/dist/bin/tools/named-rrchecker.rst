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

.. _man_named-rrchecker:

named-rrchecker - syntax checker for individual DNS resource records
--------------------------------------------------------------------

Synopsis
~~~~~~~~

:program:`named-rrchecker` [**-h**] [**-o** origin] [**-p**] [**-u**] [**-C**] [**-T**] [**-P**]

Description
~~~~~~~~~~~

``named-rrchecker`` reads a individual DNS resource record from standard
input and checks whether it is syntactically correct.

Options
~~~~~~~

``-h``
   This option prints out the help menu.

``-o origin``
   This option specifies the origin to be used when interpreting
   the record.

``-p``
   This option prints out the resulting record in canonical form. If there
   is no canonical form defined, the record is printed in unknown
   record format.

``-u``
   This option prints out the resulting record in unknown record form.

``-C``, ``-T``, and ``-P``
   These options print out the known class, standard type,
   and private type mnemonics, respectively.

See Also
~~~~~~~~

:rfc:`1034`, :rfc:`1035`, :manpage:`named(8)`.
