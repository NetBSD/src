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

.. _man_host:

host - DNS lookup utility
-------------------------

Synopsis
~~~~~~~~

:program:`host` [**-aACdlnrsTUwv**] [**-c** class] [**-N** ndots] [**-p** port] [**-R** number] [**-t** type] [**-W** wait] [**-m** flag] [ [**-4**] | [**-6**] ] [**-v**] [**-V**] {name} [server]

Description
~~~~~~~~~~~

``host`` is a simple utility for performing DNS lookups. It is normally
used to convert names to IP addresses and vice versa. When no arguments
or options are given, ``host`` prints a short summary of its command
line arguments and options.

``name`` is the domain name that is to be looked up. It can also be a
dotted-decimal IPv4 address or a colon-delimited IPv6 address, in which
case ``host`` will by default perform a reverse lookup for that address.
``server`` is an optional argument which is either the name or IP
address of the name server that ``host`` should query instead of the
server or servers listed in ``/etc/resolv.conf``.

Options
~~~~~~~

**-4**
   Use IPv4 only for query transport. See also the ``-6`` option.

**-6**
   Use IPv6 only for query transport. See also the ``-4`` option.

**-a**
   "All". The ``-a`` option is normally equivalent to ``-v -t ANY``. It
   also affects the behaviour of the ``-l`` list zone option.

**-A**
   "Almost all". The ``-A`` option is equivalent to ``-a`` except RRSIG,
   NSEC, and NSEC3 records are omitted from the output.

**-c** class
   Query class: This can be used to lookup HS (Hesiod) or CH (Chaosnet)
   class resource records. The default class is IN (Internet).

**-C**
   Check consistency: ``host`` will query the SOA records for zone
   ``name`` from all the listed authoritative name servers for that
   zone. The list of name servers is defined by the NS records that are
   found for the zone.

**-d**
   Print debugging traces. Equivalent to the ``-v`` verbose option.

**-l**
   List zone: The ``host`` command performs a zone transfer of zone
   ``name`` and prints out the NS, PTR and address records (A/AAAA).

   Together, the ``-l -a`` options print all records in the zone.

**-N** ndots
   The number of dots that have to be in ``name`` for it to be
   considered absolute. The default value is that defined using the
   ndots statement in ``/etc/resolv.conf``, or 1 if no ndots statement
   is present. Names with fewer dots are interpreted as relative names
   and will be searched for in the domains listed in the ``search`` or
   ``domain`` directive in ``/etc/resolv.conf``.

**-p** port
   Specify the port on the server to query. The default is 53.

**-r**
   Non-recursive query: Setting this option clears the RD (recursion
   desired) bit in the query. This should mean that the name server
   receiving the query will not attempt to resolve ``name``. The ``-r``
   option enables ``host`` to mimic the behavior of a name server by
   making non-recursive queries and expecting to receive answers to
   those queries that can be referrals to other name servers.

**-R** number
   Number of retries for UDP queries: If ``number`` is negative or zero,
   the number of retries will default to 1. The default value is 1, or
   the value of the ``attempts`` option in ``/etc/resolv.conf``, if set.

**-s**
   Do *not* send the query to the next nameserver if any server responds
   with a SERVFAIL response, which is the reverse of normal stub
   resolver behavior.

**-t** type
   Query type: The ``type`` argument can be any recognized query type:
   CNAME, NS, SOA, TXT, DNSKEY, AXFR, etc.

   When no query type is specified, ``host`` automatically selects an
   appropriate query type. By default, it looks for A, AAAA, and MX
   records. If the ``-C`` option is given, queries will be made for SOA
   records. If ``name`` is a dotted-decimal IPv4 address or
   colon-delimited IPv6 address, ``host`` will query for PTR records.

   If a query type of IXFR is chosen the starting serial number can be
   specified by appending an equal followed by the starting serial
   number (like ``-t IXFR=12345678``).

**-T**; **-U**
   TCP/UDP: By default, ``host`` uses UDP when making queries. The
   ``-T`` option makes it use a TCP connection when querying the name
   server. TCP will be automatically selected for queries that require
   it, such as zone transfer (AXFR) requests. Type ANY queries default
   to TCP but can be forced to UDP initially using ``-U``.

**-m** flag
   Memory usage debugging: the flag can be ``record``, ``usage``, or
   ``trace``. You can specify the ``-m`` option more than once to set
   multiple flags.

**-v**
   Verbose output. Equivalent to the ``-d`` debug option. Verbose output
   can also be enabled by setting the ``debug`` option in
   ``/etc/resolv.conf``.

**-V**
   Print the version number and exit.

**-w**
   Wait forever: The query timeout is set to the maximum possible. See
   also the ``-W`` option.

**-W** wait
   Timeout: Wait for up to ``wait`` seconds for a reply. If ``wait`` is
   less than one, the wait interval is set to one second.

   By default, ``host`` will wait for 5 seconds for UDP responses and 10
   seconds for TCP connections. These defaults can be overridden by the
   ``timeout`` option in ``/etc/resolv.conf``.

   See also the ``-w`` option.

IDN Support
~~~~~~~~~~~

If ``host`` has been built with IDN (internationalized domain name)
support, it can accept and display non-ASCII domain names. ``host``
appropriately converts character encoding of domain name before sending
a request to DNS server or displaying a reply from the server. If you'd
like to turn off the IDN support for some reason, define the IDN_DISABLE
environment variable. The IDN support is disabled if the variable is set
when ``host`` runs.

Files
~~~~~

``/etc/resolv.conf``

See Also
~~~~~~~~

:manpage:`dig(1)`, :manpage:`named(8)`.
