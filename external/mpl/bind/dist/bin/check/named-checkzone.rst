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

.. _man_named-checkzone:

named-checkzone, named-compilezone - zone file validity checking or converting tool
-----------------------------------------------------------------------------------

Synopsis
~~~~~~~~

:program:`named-checkzone` [**-d**] [**-h**] [**-j**] [**-q**] [**-v**] [**-c** class] [**-f** format] [**-F** format] [**-J** filename] [**-i** mode] [**-k** mode] [**-m** mode] [**-M** mode] [**-n** mode] [**-l** ttl] [**-L** serial] [**-o** filename] [**-r** mode] [**-s** style] [**-S** mode] [**-t** directory] [**-T** mode] [**-w** directory] [**-D**] [**-W** mode] {zonename} {filename}

:program:`named-compilezone` [**-d**] [**-j**] [**-q**] [**-v**] [**-c** class] [**-C** mode] [**-f** format] [**-F** format] [**-J** filename] [**-i** mode] [**-k** mode] [**-m** mode] [**-n** mode] [**-l** ttl] [**-L** serial] [**-r** mode] [**-s** style] [**-t** directory] [**-T** mode] [**-w** directory] [**-D**] [**-W** mode] {**-o** filename} {zonename} {filename}

Description
~~~~~~~~~~~

``named-checkzone`` checks the syntax and integrity of a zone file. It
performs the same checks as ``named`` does when loading a zone. This
makes ``named-checkzone`` useful for checking zone files before
configuring them into a name server.

``named-compilezone`` is similar to ``named-checkzone``, but it always
dumps the zone contents to a specified file in a specified format.
Additionally, it applies stricter check levels by default, since the
dump output will be used as an actual zone file loaded by ``named``.
When manually specified otherwise, the check levels must at least be as
strict as those specified in the ``named`` configuration file.

Options
~~~~~~~

**-d**
   Enable debugging.

**-h**
   Print the usage summary and exit.

**-q**
   Quiet mode - exit code only.

**-v**
   Print the version of the ``named-checkzone`` program and exit.

**-j**
   When loading a zone file, read the journal if it exists. The journal
   file name is assumed to be the zone file name appended with the
   string ``.jnl``.

**-J** filename
   When loading the zone file read the journal from the given file, if
   it exists. (Implies -j.)

**-c** class
   Specify the class of the zone. If not specified, "IN" is assumed.

**-i** mode
   Perform post-load zone integrity checks. Possible modes are
   ``"full"`` (default), ``"full-sibling"``, ``"local"``,
   ``"local-sibling"`` and ``"none"``.

   Mode ``"full"`` checks that MX records refer to A or AAAA record
   (both in-zone and out-of-zone hostnames). Mode ``"local"`` only
   checks MX records which refer to in-zone hostnames.

   Mode ``"full"`` checks that SRV records refer to A or AAAA record
   (both in-zone and out-of-zone hostnames). Mode ``"local"`` only
   checks SRV records which refer to in-zone hostnames.

   Mode ``"full"`` checks that delegation NS records refer to A or AAAA
   record (both in-zone and out-of-zone hostnames). It also checks that
   glue address records in the zone match those advertised by the child.
   Mode ``"local"`` only checks NS records which refer to in-zone
   hostnames or that some required glue exists, that is when the
   nameserver is in a child zone.

   Mode ``"full-sibling"`` and ``"local-sibling"`` disable sibling glue
   checks but are otherwise the same as ``"full"`` and ``"local"``
   respectively.

   Mode ``"none"`` disables the checks.

**-f** format
   Specify the format of the zone file. Possible formats are ``"text"``
   (default), ``"raw"``, and ``"map"``.

**-F** format
   Specify the format of the output file specified. For
   ``named-checkzone``, this does not cause any effects unless it dumps
   the zone contents.

   Possible formats are ``"text"`` (default), which is the standard
   textual representation of the zone, and ``"map"``, ``"raw"``, and
   ``"raw=N"``, which store the zone in a binary format for rapid
   loading by ``named``. ``"raw=N"`` specifies the format version of the
   raw zone file: if N is 0, the raw file can be read by any version of
   ``named``; if N is 1, the file can be read by release 9.9.0 or
   higher; the default is 1.

**-k** mode
   Perform ``"check-names"`` checks with the specified failure mode.
   Possible modes are ``"fail"`` (default for ``named-compilezone``),
   ``"warn"`` (default for ``named-checkzone``) and ``"ignore"``.

**-l** ttl
   Sets a maximum permissible TTL for the input file. Any record with a
   TTL higher than this value will cause the zone to be rejected. This
   is similar to using the ``max-zone-ttl`` option in ``named.conf``.

**-L** serial
   When compiling a zone to "raw" or "map" format, set the "source
   serial" value in the header to the specified serial number. (This is
   expected to be used primarily for testing purposes.)

**-m** mode
   Specify whether MX records should be checked to see if they are
   addresses. Possible modes are ``"fail"``, ``"warn"`` (default) and
   ``"ignore"``.

**-M** mode
   Check if a MX record refers to a CNAME. Possible modes are
   ``"fail"``, ``"warn"`` (default) and ``"ignore"``.

**-n** mode
   Specify whether NS records should be checked to see if they are
   addresses. Possible modes are ``"fail"`` (default for
   ``named-compilezone``), ``"warn"`` (default for ``named-checkzone``)
   and ``"ignore"``.

**-o** filename
   Write zone output to ``filename``. If ``filename`` is ``-`` then
   write to standard out. This is mandatory for ``named-compilezone``.

**-r** mode
   Check for records that are treated as different by DNSSEC but are
   semantically equal in plain DNS. Possible modes are ``"fail"``,
   ``"warn"`` (default) and ``"ignore"``.

**-s** style
   Specify the style of the dumped zone file. Possible styles are
   ``"full"`` (default) and ``"relative"``. The full format is most
   suitable for processing automatically by a separate script. On the
   other hand, the relative format is more human-readable and is thus
   suitable for editing by hand. For ``named-checkzone`` this does not
   cause any effects unless it dumps the zone contents. It also does not
   have any meaning if the output format is not text.

**-S** mode
   Check if a SRV record refers to a CNAME. Possible modes are
   ``"fail"``, ``"warn"`` (default) and ``"ignore"``.

**-t** directory
   Chroot to ``directory`` so that include directives in the
   configuration file are processed as if run by a similarly chrooted
   ``named``.

**-T** mode
   Check if Sender Policy Framework (SPF) records exist and issues a
   warning if an SPF-formatted TXT record is not also present. Possible
   modes are ``"warn"`` (default), ``"ignore"``.

**-w** directory
   chdir to ``directory`` so that relative filenames in master file
   $INCLUDE directives work. This is similar to the directory clause in
   ``named.conf``.

**-D**
   Dump zone file in canonical format. This is always enabled for
   ``named-compilezone``.

**-W** mode
   Specify whether to check for non-terminal wildcards. Non-terminal
   wildcards are almost always the result of a failure to understand the
   wildcard matching algorithm (:rfc:`1034`). Possible modes are ``"warn"``
   (default) and ``"ignore"``.

zonename
   The domain name of the zone being checked.

filename
   The name of the zone file.

Return Values
~~~~~~~~~~~~~

``named-checkzone`` returns an exit status of 1 if errors were detected
and 0 otherwise.

See Also
~~~~~~~~

:manpage:`named(8)`, :manpage:`named-checkconf(8)`, :rfc:`1035`, BIND 9 Administrator Reference
Manual.
