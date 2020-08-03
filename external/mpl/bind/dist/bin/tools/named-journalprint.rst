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

.. _man_named-journalprint:

named-journalprint - print zone journal in human-readable form
--------------------------------------------------------------

Synopsis
~~~~~~~~

:program:`named-journalprint` {journal}

Description
~~~~~~~~~~~

``named-journalprint`` prints the contents of a zone journal file in a
human-readable form.

Journal files are automatically created by ``named`` when changes are
made to dynamic zones (e.g., by ``nsupdate``). They record each addition
or deletion of a resource record, in binary format, allowing the changes
to be re-applied to the zone when the server is restarted after a
shutdown or crash. By default, the name of the journal file is formed by
appending the extension ``.jnl`` to the name of the corresponding zone
file.

``named-journalprint`` converts the contents of a given journal file
into a human-readable text format. Each line begins with "add" or "del",
to indicate whether the record was added or deleted, and continues with
the resource record in master-file format.

See Also
~~~~~~~~

:manpage:`named(8)`, :manpage:`nsupdate(1)`, BIND 9 Administrator Reference Manual.
