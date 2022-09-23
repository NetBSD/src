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
or options are given, ``host`` prints a short summary of its
command-line arguments and options.

``name`` is the domain name that is to be looked up. It can also be a
dotted-decimal IPv4 address or a colon-delimited IPv6 address, in which
case ``host`` by default performs a reverse lookup for that address.
``server`` is an optional argument which is either the name or IP
address of the name server that ``host`` should query instead of the
server or servers listed in ``/etc/resolv.conf``.

Options
~~~~~~~

``-4``
   This option specifies that only IPv4 should be used for query transport. See also the ``-6`` option.

``-6``
   This option specifies that only IPv6 should be used for query transport. See also the ``-4`` option.

``-a``
   The ``-a`` ("all") option is normally equivalent to ``-v -t ANY``. It
   also affects the behavior of the ``-l`` list zone option.

``-A``
   The ``-A`` ("almost all") option is equivalent to ``-a``, except that RRSIG,
   NSEC, and NSEC3 records are omitted from the output.

``-c class``
   This option specifies the query class, which can be used to lookup HS (Hesiod) or CH (Chaosnet)
   class resource records. The default class is IN (Internet).

``-C``
   This option indicates that ``named`` should check consistency, meaning that ``host`` queries the SOA records for zone
   ``name`` from all the listed authoritative name servers for that
   zone. The list of name servers is defined by the NS records that are
   found for the zone.

``-d``
   This option prints debugging traces, and is equivalent to the ``-v`` verbose option.

``-l``
   This option tells ``named`` to list the zone, meaning the ``host`` command performs a zone transfer of zone
   ``name`` and prints out the NS, PTR, and address records (A/AAAA).

   Together, the ``-l -a`` options print all records in the zone.

``-N ndots``
   This option specifies the number of dots (``ndots``) that have to be in ``name`` for it to be
   considered absolute. The default value is that defined using the
   ``ndots`` statement in ``/etc/resolv.conf``, or 1 if no ``ndots`` statement
   is present. Names with fewer dots are interpreted as relative names,
   and are searched for in the domains listed in the ``search`` or
   ``domain`` directive in ``/etc/resolv.conf``.

``-p port``
   This option specifies the port to query on the server. The default is 53.

``-r``
   This option specifies a non-recursive query; setting this option clears the RD (recursion
   desired) bit in the query. This means that the name server
   receiving the query does not attempt to resolve ``name``. The ``-r``
   option enables ``host`` to mimic the behavior of a name server by
   making non-recursive queries, and expecting to receive answers to
   those queries that can be referrals to other name servers.

``-R number``
   This option specifies the number of retries for UDP queries. If ``number`` is negative or zero,
   the number of retries is silently set to 1. The default value is 1, or
   the value of the ``attempts`` option in ``/etc/resolv.conf``, if set.

``-s``
   This option tells ``named`` *not* to send the query to the next nameserver if any server responds
   with a SERVFAIL response, which is the reverse of normal stub
   resolver behavior.

``-t type``
   This option specifies the query type. The ``type`` argument can be any recognized query type:
   CNAME, NS, SOA, TXT, DNSKEY, AXFR, etc.

   When no query type is specified, ``host`` automatically selects an
   appropriate query type. By default, it looks for A, AAAA, and MX
   records. If the ``-C`` option is given, queries are made for SOA
   records. If ``name`` is a dotted-decimal IPv4 address or
   colon-delimited IPv6 address, ``host`` queries for PTR records.

   If a query type of IXFR is chosen, the starting serial number can be
   specified by appending an equals sign (=), followed by the starting serial
   number, e.g., ``-t IXFR=12345678``.

``-T``; ``-U``
   This option specifies TCP or UDP. By default, ``host`` uses UDP when making queries; the
   ``-T`` option makes it use a TCP connection when querying the name
   server. TCP is automatically selected for queries that require
   it, such as zone transfer (AXFR) requests. Type ``ANY`` queries default
   to TCP, but can be forced to use UDP initially via ``-U``.

``-m flag``
   This option sets memory usage debugging: the flag can be ``record``, ``usage``, or
   ``trace``. The ``-m`` option can be specified more than once to set
   multiple flags.

``-v``
   This option sets verbose output, and is equivalent to the ``-d`` debug option. Verbose output
   can also be enabled by setting the ``debug`` option in
   ``/etc/resolv.conf``.

``-V``
   This option prints the version number and exits.

``-w``
   This option sets "wait forever": the query timeout is set to the maximum possible. See
   also the ``-W`` option.

``-W wait``
   This options sets the length of the wait timeout, indicating that ``named`` should wait for up to ``wait`` seconds for a reply. If ``wait`` is
   less than 1, the wait interval is set to 1 second.

   By default, ``host`` waits for 5 seconds for UDP responses and 10
   seconds for TCP connections. These defaults can be overridden by the
   ``timeout`` option in ``/etc/resolv.conf``.

   See also the ``-w`` option.

IDN Support
~~~~~~~~~~~

If ``host`` has been built with IDN (internationalized domain name)
support, it can accept and display non-ASCII domain names. ``host``
appropriately converts character encoding of a domain name before sending
a request to a DNS server or displaying a reply from the server.
To turn off IDN support, define the ``IDN_DISABLE``
environment variable. IDN support is disabled if the variable is set
when ``host`` runs.

Files
~~~~~

``/etc/resolv.conf``

See Also
~~~~~~~~

:manpage:`dig(1)`, :manpage:`named(8)`.
