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

``client``
    Processing of client requests.

``cname``
    Name servers that are skipped for being a CNAME rather than A/AAAA records.

``config``
    Configuration file parsing and processing.

``database``
    Messages relating to the databases used internally by the name server to store zone and cache data.

``default``
    Logging options for those categories where no specific configuration has been defined.

``delegation-only``
    Queries that have been forced to NXDOMAIN as the result of a delegation-only zone or a :any:`delegation-only` in a forward, hint, or stub zone declaration.

``dispatch``
    Dispatching of incoming packets to the server modules where they are to be processed.

``dnssec``
    DNSSEC and TSIG protocol processing.

``dnstap``
    The :any:`dnstap` DNS traffic capture system.

``edns-disabled``
    Log queries that have been forced to use plain DNS due to timeouts. This is often due to the remote servers not being :rfc:`1034`-compliant (not always returning FORMERR or similar to EDNS queries and other extensions to the DNS when they are not understood). In other words, this is targeted at servers that fail to respond to DNS queries that they don't understand.

    Note: the log message can also be due to packet loss. Before reporting servers for non-:rfc:`1034` compliance they should be re-tested to determine the nature of the non-compliance. This testing should prevent or reduce the number of false-positive reports.

    Note: eventually :iscman:`named` will have to stop treating such timeouts as due to :rfc:`1034` non-compliance and start treating it as plain packet loss. Falsely classifying packet loss as due to :rfc:`1034` non-compliance impacts DNSSEC validation, which requires EDNS for the DNSSEC records to be returned.

``general``
    A catch-all for many things that still are not classified into categories.

``lame-servers``
    Misconfigurations in remote servers, discovered by BIND 9 when trying to query those servers during resolution.

``network``
    Network operations.

``notify``
    The NOTIFY protocol.

``nsid``
    NSID options received from upstream servers.

``queries``
    A location where queries should be logged.

    At startup, specifying the category ``queries`` also enables query logging unless the :any:`querylog` option has been specified.

    The query log entry first reports a client object identifier in @0x<hexadecimal-number> format. Next, it reports the client's IP address and port number, and the query name, class, and type. Next, it reports whether the Recursion Desired flag was set (+ if set, - if not set), whether the query was signed (S), whether EDNS was in use along with the EDNS version number (E(#)), whether TCP was used (T), whether DO (DNSSEC Ok) was set (D), whether CD (Checking Disabled) was set (C), whether a valid DNS Server COOKIE was received (V), and whether a DNS COOKIE option without a valid Server COOKIE was present (K). After this, the destination address the query was sent to is reported. Finally, if any CLIENT-SUBNET option was present in the client query, it is included in square brackets in the format [ECS address/source/scope].

    .. code-block:: none

       client @0x7f91b8005490 127.0.0.1#62536 (www.example.com): query: www.example.com IN AAAA +E(0)K (127.0.0.1)
       client @0x7f91b4007400 ::1#62537 (www.example.net): query: www.example.net IN AAAA +E(0)K (::1)

    The first part of this log message, showing the client address/port number and query name, is repeated in all subsequent log messages related to the same query.

``query-errors``
    Information about queries that resulted in some failure.

``rate-limit``
    Start, periodic, and final notices of the rate limiting of a stream of responses that are logged at ``info`` severity in this category. These messages include a hash value of the domain name of the response and the name itself, except when there is insufficient memory to record the name for the final notice. The final notice is normally delayed until about one minute after rate limiting stops. A lack of memory can hurry the final notice, which is indicated by an initial asterisk (\*). Various internal events are logged at debug level 1 and higher.

    Rate limiting of individual requests is logged in the ``query-errors`` category.

``resolver``
    DNS resolution, such as the recursive lookups performed on behalf of clients by a caching name server.

``rpz``
    Information about errors in response policy zone files, rewritten responses, and, at the highest ``debug`` levels, mere rewriting attempts.

``rpz-passthru``
    Information about RPZ PASSTHRU policy activity. This category allows pre-approved policy activity to be logged into a dedicated channel.

``security``
    Approval and denial of requests.

``serve-stale``
    Indication of whether a stale answer is used following a resolver failure.

``spill``
    Queries that have been terminated, either by dropping or responding with SERVFAIL, as a result of a fetchlimit quota being exceeded.

``sslkeylog``
    TLS pre-master secrets (for debugging purposes).

``trust-anchor-telemetry``
    :any:`trust-anchor-telemetry` requests received by :iscman:`named`.

``unmatched``
    Messages that :iscman:`named` was unable to determine the class of, or for which there was no matching :any:`view`. A one-line summary is also logged to the ``client`` category. This category is best sent to a file or stderr; by default it is sent to the :any:`null` channel.

``update``
    Dynamic updates.

``update-security``
    Approval and denial of update requests.

``xfer-in``
    Zone transfers the server is receiving.

``xfer-out``
    Zone transfers the server is sending.

``zoneload``
    Loading of zones and creation of automatic empty zones.
