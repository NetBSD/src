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

.. iscman:: named-nzd2nzf
.. program:: named-nzd2nzf
.. _man_named-nzd2nzf:

named-nzd2nzf - convert an NZD database to NZF text format
----------------------------------------------------------

Synopsis
~~~~~~~~

:program:`named-nzd2nzf` {filename}

Description
~~~~~~~~~~~

:program:`named-nzd2nzf` converts an NZD database to NZF format and prints it
to standard output. This can be used to review the configuration of
zones that were added to :iscman:`named` via :option:`rndc addzone`. It can also be
used to restore the old file format when rolling back from a newer
version of BIND to an older version.

Arguments
~~~~~~~~~

.. option:: filename

   This is the name of the ``.nzd`` file whose contents should be printed.

See Also
~~~~~~~~

BIND 9 Administrator Reference Manual.
