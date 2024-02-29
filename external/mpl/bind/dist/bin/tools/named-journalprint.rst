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

.. iscman:: named-journalprint
.. program:: named-journalprint
.. _man_named-journalprint:

named-journalprint - print zone journal in human-readable form
--------------------------------------------------------------

Synopsis
~~~~~~~~

:program:`named-journalprint` [-c serial] [**-dux**] {journal}

Description
~~~~~~~~~~~

:program:`named-journalprint` scans the contents of a zone journal file,
printing it in a human-readable form, or, optionally, converting it
to a different journal file format.

Journal files are automatically created by :iscman:`named` when changes are
made to dynamic zones (e.g., by :iscman:`nsupdate`). They record each addition
or deletion of a resource record, in binary format, allowing the changes
to be re-applied to the zone when the server is restarted after a
shutdown or crash. By default, the name of the journal file is formed by
appending the extension ``.jnl`` to the name of the corresponding zone
file.

:program:`named-journalprint` converts the contents of a given journal file
into a human-readable text format. Each line begins with ``add`` or ``del``,
to indicate whether the record was added or deleted, and continues with
the resource record in master-file format.

The ``-c`` (compact) option provides a mechanism to reduce the size of
a journal by removing (most/all) transactions prior to the specified
serial number. Note: this option *must not* be used while :iscman:`named` is
running, and can cause data loss if the zone file has not been updated
to contain the data being removed from the journal. Use with extreme caution.

The ``-x`` option causes additional data about the journal file to be
printed at the beginning of the output and before each group of changes.

The ``-u`` (upgrade) and ``-d`` (downgrade) options recreate the journal
file with a modified format version.  The existing journal file is
replaced.  ``-d`` writes out the journal in the format used by
versions of BIND up to 9.16.11; ``-u`` writes it out in the format used
by versions since 9.16.13. (9.16.12 is omitted due to a journal-formatting
bug in that release.) Note that these options *must not* be used while
:iscman:`named` is running.

See Also
~~~~~~~~

:iscman:`named(8) <named>`, :iscman:`nsupdate(1) <nsupdate>`, BIND 9 Administrator Reference Manual.
