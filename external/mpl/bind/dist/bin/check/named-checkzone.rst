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

.. BEWARE: Do not forget to edit also named-compilezone.rst!

.. _man_named-checkzone:

named-checkzone - zone file validation tool
-------------------------------------------

Synopsis
~~~~~~~~

:program:`named-checkzone` [**-d**] [**-h**] [**-j**] [**-q**] [**-v**] [**-c** class] [**-f** format] [**-F** format] [**-J** filename] [**-i** mode] [**-k** mode] [**-m** mode] [**-M** mode] [**-n** mode] [**-l** ttl] [**-L** serial] [**-o** filename] [**-r** mode] [**-s** style] [**-S** mode] [**-t** directory] [**-T** mode] [**-w** directory] [**-D**] [**-W** mode] {zonename} {filename}

Description
~~~~~~~~~~~

``named-checkzone`` checks the syntax and integrity of a zone file. It
performs the same checks as ``named`` does when loading a zone. This
makes ``named-checkzone`` useful for checking zone files before
configuring them into a name server.

Options
~~~~~~~

``-d``
   This option enables debugging.

``-h``
   This option prints the usage summary and exits.

``-q``
   This option sets quiet mode, which only sets an exit code to indicate
   successful or failed completion.

``-v``
   This option prints the version of the ``named-checkzone`` program and exits.

``-j``
   When loading a zone file, this option tells ``named`` to read the journal if it exists. The journal
   file name is assumed to be the zone file name with the
   string ``.jnl`` appended.

``-J filename``
   When loading the zone file, this option tells ``named`` to read the journal from the given file, if
   it exists. This implies ``-j``.

``-c class``
   This option specifies the class of the zone. If not specified, ``IN`` is assumed.

``-i mode``
   This option performs post-load zone integrity checks. Possible modes are
   ``full`` (the default), ``full-sibling``, ``local``,
   ``local-sibling``, and ``none``.

   Mode ``full`` checks that MX records refer to A or AAAA records
   (both in-zone and out-of-zone hostnames). Mode ``local`` only
   checks MX records which refer to in-zone hostnames.

   Mode ``full`` checks that SRV records refer to A or AAAA records
   (both in-zone and out-of-zone hostnames). Mode ``local`` only
   checks SRV records which refer to in-zone hostnames.

   Mode ``full`` checks that delegation NS records refer to A or AAAA
   records (both in-zone and out-of-zone hostnames). It also checks that
   glue address records in the zone match those advertised by the child.
   Mode ``local`` only checks NS records which refer to in-zone
   hostnames or verifies that some required glue exists, i.e., when the
   name server is in a child zone.

   Modes ``full-sibling`` and ``local-sibling`` disable sibling glue
   checks, but are otherwise the same as ``full`` and ``local``,
   respectively.

   Mode ``none`` disables the checks.

``-f format``
   This option specifies the format of the zone file. Possible formats are
   ``text`` (the default), ``raw``, and ``map``.

``-F format``
   This option specifies the format of the output file specified. For
   ``named-checkzone``, this does not have any effect unless it dumps
   the zone contents.

   Possible formats are ``text`` (the default), which is the standard
   textual representation of the zone, and ``map``, ``raw``, and ``raw=N``, which
   store the zone in a binary format for rapid loading by ``named``.
   ``raw=N`` specifies the format version of the raw zone file: if ``N`` is
   0, the raw file can be read by any version of ``named``; if N is 1, the
   file can only be read by release 9.9.0 or higher. The default is 1.

``-k mode``
   This option performs ``check-names`` checks with the specified failure mode.
   Possible modes are ``fail``, ``warn`` (the default), and ``ignore``.

``-l ttl``
   This option sets a maximum permissible TTL for the input file. Any record with a
   TTL higher than this value causes the zone to be rejected. This
   is similar to using the ``max-zone-ttl`` option in ``named.conf``.

``-L serial``
   When compiling a zone to ``raw`` or ``map`` format, this option sets the "source
   serial" value in the header to the specified serial number. This is
   expected to be used primarily for testing purposes.

``-m mode``
   This option specifies whether MX records should be checked to see if they are
   addresses. Possible modes are ``fail``, ``warn`` (the default), and
   ``ignore``.

``-M mode``
   This option checks whether a MX record refers to a CNAME. Possible modes are
   ``fail``, ``warn`` (the default), and ``ignore``.

``-n mode``
   This option specifies whether NS records should be checked to see if they are
   addresses. Possible modes are ``fail``, ``warn`` (the default), and ``ignore``.

``-o filename``
   This option writes the zone output to ``filename``. If ``filename`` is ``-``, then
   the zone output is written to standard output.

``-r mode``
   This option checks for records that are treated as different by DNSSEC but are
   semantically equal in plain DNS. Possible modes are ``fail``,
   ``warn`` (the default), and ``ignore``.

``-s style``
   This option specifies the style of the dumped zone file. Possible styles are
   ``full`` (the default) and ``relative``. The ``full`` format is most
   suitable for processing automatically by a separate script.
   The relative format is more human-readable and is thus
   suitable for editing by hand. This does not have any effect unless it dumps
   the zone contents. It also does not have any meaning if the output format
   is not text.

``-S mode``
   This option checks whether an SRV record refers to a CNAME. Possible modes are
   ``fail``, ``warn`` (the default), and ``ignore``.

``-t directory``
   This option tells ``named`` to chroot to ``directory``, so that ``include`` directives in the
   configuration file are processed as if run by a similarly chrooted
   ``named``.

``-T mode``
   This option checks whether Sender Policy Framework (SPF) records exist and issues a
   warning if an SPF-formatted TXT record is not also present. Possible
   modes are ``warn`` (the default) and ``ignore``.

``-w directory``
   This option instructs ``named`` to chdir to ``directory``, so that relative filenames in master file
   ``$INCLUDE`` directives work. This is similar to the directory clause in
   ``named.conf``.

``-D``
   This option dumps the zone file in canonical format.

``-W mode``
   This option specifies whether to check for non-terminal wildcards. Non-terminal
   wildcards are almost always the result of a failure to understand the
   wildcard matching algorithm (:rfc:`4592`). Possible modes are ``warn``
   (the default) and ``ignore``.

``zonename``
   This indicates the domain name of the zone being checked.

``filename``
   This is the name of the zone file.

Return Values
~~~~~~~~~~~~~

``named-checkzone`` returns an exit status of 1 if errors were detected
and 0 otherwise.

See Also
~~~~~~~~

:manpage:`named(8)`, :manpage:`named-checkconf(8)`, :manpage:`named-compilezone(8)`,
:rfc:`1035`, BIND 9 Administrator Reference Manual.
