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

.. _man_mdig:

mdig - DNS pipelined lookup utility
-----------------------------------

Synopsis
~~~~~~~~

:program:`mdig` {@server} [**-f** filename] [**-h**] [**-v**] [ [**-4**] | [**-6**] ] [**-m**] [**-b** address] [**-p** port#] [**-c** class] [**-t** type] [**-i**] [**-x** addr] [plusopt...]

:program:`mdig` {**-h**}

:program:`mdig` [@server] {global-opt...} { {local-opt...} {query} ...}

Description
~~~~~~~~~~~

``mdig`` is a multiple/pipelined query version of ``dig``: instead of
waiting for a response after sending each query, it begins by sending
all queries. Responses are displayed in the order in which they are
received, not in the order the corresponding queries were sent.

``mdig`` options are a subset of the ``dig`` options, and are divided
into "anywhere options," which can occur anywhere, "global options," which
must occur before the query name (or they are ignored with a warning),
and "local options," which apply to the next query on the command line.

The ``@server`` option is a mandatory global option. It is the name or IP
address of the name server to query. (Unlike ``dig``, this value is not
retrieved from ``/etc/resolv.conf``.) It can be an IPv4 address in
dotted-decimal notation, an IPv6 address in colon-delimited notation, or
a hostname. When the supplied ``server`` argument is a hostname,
``mdig`` resolves that name before querying the name server.

``mdig`` provides a number of query options which affect the way in
which lookups are made and the results displayed. Some of these set or
reset flag bits in the query header, some determine which sections of
the answer get printed, and others determine the timeout and retry
strategies.

Each query option is identified by a keyword preceded by a plus sign
(``+``). Some keywords set or reset an option. These may be preceded by
the string ``no`` to negate the meaning of that keyword. Other keywords
assign values to options like the timeout interval. They have the form
``+keyword=value``.

Anywhere Options
~~~~~~~~~~~~~~~~

``-f``
   This option makes ``mdig`` operate in batch mode by reading a list
   of lookup requests to process from the file ``filename``. The file
   contains a number of queries, one per line. Each entry in the file
   should be organized in the same way they would be presented as queries
   to ``mdig`` using the command-line interface.

``-h``
   This option causes ``mdig`` to print detailed help information, with the full list
   of options, and exit.

``-v``
   This option causes ``mdig`` to print the version number and exit.

Global Options
~~~~~~~~~~~~~~

``-4``
   This option forces ``mdig`` to only use IPv4 query transport.

``-6``
   This option forces ``mdig`` to only use IPv6 query transport.

``-b address``
   This option sets the source IP address of the query to
   ``address``. This must be a valid address on one of the host's network
   interfaces or "0.0.0.0" or "::". An optional port may be specified by
   appending "#<port>"

``-m``
   This option enables memory usage debugging.

``-p port#``
   This option is used when a non-standard port number is to be
   queried. ``port#`` is the port number that ``mdig`` sends its
   queries to, instead of the standard DNS port number 53. This option is
   used to test a name server that has been configured to listen for
   queries on a non-standard port number.

The global query options are:

``+[no]additional``
   This option displays [or does not display] the additional section of a reply. The
   default is to display it.

``+[no]all``
   This option sets or clears all display flags.

``+[no]answer``
   This option displays [or does not display] the answer section of a reply. The default
   is to display it.

``+[no]authority``
   This option displays [or does not display] the authority section of a reply. The
   default is to display it.

``+[no]besteffort``
   This option attempts to display [or does not display] the contents of messages which are malformed. The
   default is to not display malformed answers.

``+burst``
   This option delays queries until the start of the next second.

``+[no]cl``
   This option displays [or does not display] the CLASS when printing the record.

``+[no]comments``
   This option toggles the display of comment lines in the output. The default is to
   print comments.

``+[no]continue``
   This option toggles continuation on errors (e.g. timeouts).

``+[no]crypto``
   This option toggles the display of cryptographic fields in DNSSEC records. The
   contents of these fields are unnecessary to debug most DNSSEC
   validation failures and removing them makes it easier to see the
   common failures. The default is to display the fields. When omitted,
   they are replaced by the string "[omitted]"; in the DNSKEY case, the
   key ID is displayed as the replacement, e.g., ``[ key id = value ]``.

``+dscp[=value]``
   This option sets the DSCP code point to be used when sending the query. Valid DSCP
   code points are in the range [0...63]. By default no code point is
   explicitly set.

``+[no]multiline``
   This option toggles printing of records, like the SOA records, in a verbose multi-line format
   with human-readable comments. The default is to print each record on
   a single line, to facilitate machine parsing of the ``mdig`` output.

``+[no]question``
   This option prints [or does not print] the question section of a query when an answer
   is returned. The default is to print the question section as a
   comment.

``+[no]rrcomments``
   This option toggles the display of per-record comments in the output (for example,
   human-readable key information about DNSKEY records). The default is
   not to print record comments unless multiline mode is active.

``+[no]short``
   This option provides [or does not provide] a terse answer. The default is to print the answer in a
   verbose form.

``+split=W``
   This option splits long hex- or base64-formatted fields in resource records into
   chunks of ``W`` characters (where ``W`` is rounded up to the nearest
   multiple of 4). ``+nosplit`` or ``+split=0`` causes fields not to be
   split. The default is 56 characters, or 44 characters when
   multiline mode is active.

``+[no]tcp``
   This option uses [or does not use] TCP when querying name servers. The default behavior
   is to use UDP.

``+[no]ttlid``
   This option displays [or does not display] the TTL when printing the record.

``+[no]ttlunits``
   This option displays [or does not display] the TTL in friendly human-readable time
   units of "s", "m", "h", "d", and "w", representing seconds, minutes,
   hours, days, and weeks. This implies +ttlid.

``+[no]vc``
   This option uses [or does not use] TCP when querying name servers. This alternate
   syntax to ``+[no]tcp`` is provided for backwards compatibility. The
   ``vc`` stands for "virtual circuit".

Local Options
~~~~~~~~~~~~~

``-c class``
   This option sets the query class to ``class``. It can be any valid
   query class which is supported in BIND 9. The default query class is
   "IN".

``-t type``
   This option sets the query type to ``type``. It can be any valid
   query type which is supported in BIND 9. The default query type is "A",
   unless the ``-x`` option is supplied to indicate a reverse lookup with
   the "PTR" query type.

``-x addr``
   Reverse lookups - mapping addresses to names - are simplified by
   this option. ``addr`` is an IPv4 address in dotted-decimal
   notation, or a colon-delimited IPv6 address. ``mdig`` automatically
   performs a lookup for a query name like ``11.12.13.10.in-addr.arpa`` and
   sets the query type and class to PTR and IN respectively. By default,
   IPv6 addresses are looked up using nibble format under the IP6.ARPA
   domain.

The local query options are:

``+[no]aaflag``
   This is a synonym for ``+[no]aaonly``.

``+[no]aaonly``
   This sets the ``aa`` flag in the query.

``+[no]adflag``
   This sets [or does not set] the AD (authentic data) bit in the query. This
   requests the server to return whether all of the answer and authority
   sections have all been validated as secure, according to the security
   policy of the server. AD=1 indicates that all records have been
   validated as secure and the answer is not from a OPT-OUT range. AD=0
   indicates that some part of the answer was insecure or not validated.
   This bit is set by default.

``+bufsize=B``
   This sets the UDP message buffer size advertised using EDNS0 to ``B``
   bytes. The maximum and minimum sizes of this buffer are 65535 and 0
   respectively. Values outside this range are rounded up or down
   appropriately. Values other than zero cause a EDNS query to be
   sent.

``+[no]cdflag``
   This sets [or does not set] the CD (checking disabled) bit in the query. This
   requests the server to not perform DNSSEC validation of responses.

``+[no]cookie=####``
   This sends [or does not send] a COOKIE EDNS option, with an optional value. Replaying a COOKIE
   from a previous response allows the server to identify a previous
   client. The default is ``+nocookie``.

``+[no]dnssec``
   This requests that DNSSEC records be sent by setting the DNSSEC OK (DO) bit in
   the OPT record in the additional section of the query.

``+[no]edns[=#]``
   This specifies [or does not specify] the EDNS version to query with. Valid values are 0 to 255.
   Setting the EDNS version causes an EDNS query to be sent.
   ``+noedns`` clears the remembered EDNS version. EDNS is set to 0 by
   default.

``+[no]ednsflags[=#]``
   This sets the must-be-zero EDNS flag bits (Z bits) to the specified value.
   Decimal, hex, and octal encodings are accepted. Setting a named flag
   (e.g. DO) is silently ignored. By default, no Z bits are set.

``+[no]ednsopt[=code[:value]]``
   This specifies [or does not specify] an EDNS option with code point ``code`` and an optional payload
   of ``value`` as a hexadecimal string. ``+noednsopt`` clears the EDNS
   options to be sent.

``+[no]expire``
   This toggles sending of an EDNS Expire option.

``+[no]nsid``
   This toggles inclusion of an EDNS name server ID request when sending a query.

``+[no]recurse``
   This toggles the setting of the RD (recursion desired) bit in the query.
   This bit is set by default, which means ``mdig`` normally sends
   recursive queries.

``+retry=T``
   This sets the number of times to retry UDP queries to server to ``T``
   instead of the default, 2. Unlike ``+tries``, this does not include
   the initial query.

``+[no]subnet=addr[/prefix-length]``
   This sends [or does not send] an EDNS Client Subnet option with the specified IP
   address or network prefix.

``mdig +subnet=0.0.0.0/0``, or simply ``mdig +subnet=0``
   This sends an EDNS client-subnet option with an empty address and a source
   prefix-length of zero, which signals a resolver that the client's
   address information must *not* be used when resolving this query.

``+timeout=T``
   This sets the timeout for a query to ``T`` seconds. The default timeout is
   5 seconds for UDP transport and 10 for TCP. An attempt to set ``T``
   to less than 1 results in a query timeout of 1 second being
   applied.

``+tries=T``
   This sets the number of times to try UDP queries to server to ``T``
   instead of the default, 3. If ``T`` is less than or equal to zero,
   the number of tries is silently rounded up to 1.

``+udptimeout=T``
   This sets the timeout between UDP query retries to ``T``.

``+[no]unknownformat``
   This prints [or does not print] all RDATA in unknown RR-type presentation format (see :rfc:`3597`).
   The default is to print RDATA for known types in the type's
   presentation format.

``+[no]yaml``
   This toggles printing of the responses in a detailed YAML format.

``+[no]zflag``
   This sets [or does not set] the last unassigned DNS header flag in a DNS query.
   This flag is off by default.

See Also
~~~~~~~~

:manpage:`dig(1)`, :rfc:`1035`.
