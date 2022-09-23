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

.. _Introduction:

Introduction
============

The Internet Domain Name System (DNS) consists of the syntax to specify
the names of entities in the Internet in a hierarchical manner, the
rules used for delegating authority over names, and the system
implementation that actually maps names to Internet addresses. DNS data
is maintained in a group of distributed hierarchical databases.

.. _doc_scope:

Scope of Document
-----------------

The Berkeley Internet Name Domain (BIND) implements a domain name server
for a number of operating systems. This document provides basic
information about the installation and care of the Internet Systems
Consortium (ISC) BIND version 9 software package for system
administrators.

This manual covers BIND version |release|.

.. _organization:

Organization of This Document
-----------------------------

In this document, *Chapter 1* introduces the basic DNS and BIND
concepts. *Chapter 2* describes resource requirements for running BIND
in various environments. Information in *Chapter 3* is *task-oriented*
in its presentation and is organized functionally, to aid in the process
of installing the BIND 9 software. The task-oriented section is followed
by *Chapter 4*, which is organized as a reference manual to aid in the ongoing
maintenance of the software. *Chapter 5* contains more advanced concepts that
the system administrator may need for implementing certain options. *Chapter 6*
addresses security considerations, and *Chapter 7* contains troubleshooting help.
The main body of the document is followed by several *appendices* which contain
useful reference information, such as a *bibliography* and historic
information related to BIND and the Domain Name System.

.. _conventions:

Conventions Used in This Document
---------------------------------

In this document, we generally use ``Fixed Width`` text to indicate the
following types of information:

- pathnames
- filenames
- URLs
- hostnames
- mailing list names
- new terms or concepts
- literal user input
- program output
- keywords
- variables

Text in "quotes," **bold**, or *italics* is also used for emphasis or clarity.

.. _dns_overview:

The Domain Name System (DNS)
----------------------------

This document explains the installation and upkeep
of the BIND (Berkeley Internet Name Domain) software package. We
begin by reviewing the fundamentals of the Domain Name System (DNS) as
they relate to BIND.

.. _dns_fundamentals:

DNS Fundamentals
~~~~~~~~~~~~~~~~

The Domain Name System (DNS) is a hierarchical, distributed database. It
stores information for mapping Internet host names to IP addresses and
vice versa, mail routing information, and other data used by Internet
applications.

Clients look up information in the DNS by calling a *resolver* library,
which sends queries to one or more *name servers* and interprets the
responses. The BIND 9 software distribution contains a name server,
``named``, and a set of associated tools.

.. _domain_names:

Domains and Domain Names
~~~~~~~~~~~~~~~~~~~~~~~~

The data stored in the DNS is identified by *domain names* that are
organized as a tree according to organizational or administrative
boundaries. Each node of the tree, called a *domain*, is given a label.
The domain name of the node is the concatenation of all the labels on
the path from the node to the *root* node. This is represented in
written form as a string of labels listed from right to left and
separated by dots. A label need only be unique within its parent domain.

For example, a domain name for a host at the company *Example, Inc.*
could be ``ourhost.example.com``, where ``com`` is the top-level domain
to which ``ourhost.example.com`` belongs, ``example`` is a subdomain of
``com``, and ``ourhost`` is the name of the host.

For administrative purposes, the name space is partitioned into areas
called *zones*, each starting at a node and extending down to the "leaf"
nodes or to nodes where other zones start. The data for each zone is
stored in a *name server*, which answers queries about the zone using
the *DNS protocol*.

The data associated with each domain name is stored in the form of
*resource records* (RRs). Some of the supported resource record types
are described in :ref:`types_of_resource_records_and_when_to_use_them`.

For more detailed information about the design of the DNS and the DNS
protocol, please refer to the standards documents listed in :ref:`rfcs`.

Zones
~~~~~

To properly operate a name server, it is important to understand the
difference between a *zone* and a *domain*.

As stated previously, a zone is a point of delegation in the DNS tree. A
zone consists of those contiguous parts of the domain tree for which a
name server has complete information and over which it has authority. It
contains all domain names from a certain point downward in the domain
tree except those which are delegated to other zones. A delegation point
is marked by one or more *NS records* in the parent zone, which should
be matched by equivalent NS records at the root of the delegated zone.

For instance, consider the ``example.com`` domain, which includes names
such as ``host.aaa.example.com`` and ``host.bbb.example.com``, even
though the ``example.com`` zone includes only delegations for the
``aaa.example.com`` and ``bbb.example.com`` zones. A zone can map
exactly to a single domain, but could also include only part of a
domain, the rest of which could be delegated to other name servers.
Every name in the DNS tree is a *domain*, even if it is *terminal*, that
is, has no *subdomains*. Every subdomain is a domain and every domain
except the root is also a subdomain. The terminology is not intuitive
and we suggest reading :rfc:`1033`, :rfc:`1034`, and :rfc:`1035` to gain a complete
understanding of this difficult and subtle topic.

Though BIND 9 is called a "domain name server," it deals primarily in
terms of zones. The ``primary`` and ``secondary`` declarations in the ``named.conf``
file specify zones, not domains. When BIND asks some other site if it is
willing to be a secondary server for a *domain*, it is actually asking
for secondary service for some collection of *zones*.

.. _auth_servers:

Authoritative Name Servers
~~~~~~~~~~~~~~~~~~~~~~~~~~

Each zone is served by at least one *authoritative name server*, which
contains the complete data for the zone. To make the DNS tolerant of
server and network failures, most zones have two or more authoritative
servers, on different networks.

Responses from authoritative servers have the "authoritative answer"
(AA) bit set in the response packets. This makes them easy to identify
when debugging DNS configurations using tools like ``dig`` (:ref:`diagnostic_tools`).

.. _primary_master:

The Primary Server
^^^^^^^^^^^^^^^^^^

The authoritative server, where the main copy of the zone data is
maintained, is called the *primary* (formerly *master*) server, or simply the
*primary*. Typically it loads the zone contents from some local file
edited by humans or perhaps generated mechanically from some other local
file which is edited by humans. This file is called the *zone file* or
*master file*.

In some cases, however, the master file may not be edited by humans at
all, but may instead be the result of *dynamic update* operations.

.. _secondary_server:

Secondary Servers
^^^^^^^^^^^^^^^^^

The other authoritative servers, the *secondary* servers (formerly known as
*slave* servers) load the zone contents from another server using a
replication process known as a *zone transfer*. Typically the data is
transferred directly from the primary, but it is also possible to
transfer it from another secondary. In other words, a secondary server may
itself act as a primary to a subordinate secondary server.

Periodically, the secondary server must send a refresh query to determine
whether the zone contents have been updated. This is done by sending a
query for the zone's Start of Authority (SOA) record and checking whether the SERIAL field
has been updated; if so, a new transfer request is initiated. The timing
of these refresh queries is controlled by the SOA REFRESH and RETRY
fields, but can be overridden with the ``max-refresh-time``,
``min-refresh-time``, ``max-retry-time``, and ``min-retry-time``
options.

If the zone data cannot be updated within the time specified by the SOA
EXPIRE option (up to a hard-coded maximum of 24 weeks), the secondary
zone expires and no longer responds to queries.

.. _stealth_server:

Stealth Servers
^^^^^^^^^^^^^^^

Usually, all of the zone's authoritative servers are listed in NS
records in the parent zone. These NS records constitute a *delegation*
of the zone from the parent. The authoritative servers are also listed
in the zone file itself, at the *top level* or *apex* of the zone.
Servers that are not in the parent's NS delegation can be listed in the
zone's top-level NS records, but servers that are not present at the
zone's top level cannot be listed in the parent's delegation.

A *stealth server* is a server that is authoritative for a zone but is
not listed in that zone's NS records. Stealth servers can be used for
keeping a local copy of a zone, to speed up access to the zone's records
or to make sure that the zone is available even if all the "official"
servers for the zone are inaccessible.

A configuration where the primary server itself is a stealth
server is often referred to as a "hidden primary" configuration. One use
for this configuration is when the primary is behind a firewall
and is therefore unable to communicate directly with the outside world.

.. _cache_servers:

Caching Name Servers
~~~~~~~~~~~~~~~~~~~~

The resolver libraries provided by most operating systems are *stub
resolvers*, meaning that they are not capable of performing the full DNS
resolution process by themselves by talking directly to the
authoritative servers. Instead, they rely on a local name server to
perform the resolution on their behalf. Such a server is called a
*recursive* name server; it performs *recursive lookups* for local
clients.

To improve performance, recursive servers cache the results of the
lookups they perform. Since the processes of recursion and caching are
intimately connected, the terms *recursive server* and *caching server*
are often used synonymously.

The length of time for which a record may be retained in the cache of a
caching name server is controlled by the Time-To-Live (TTL) field
associated with each resource record.

.. _forwarder:

Forwarding
^^^^^^^^^^

Even a caching name server does not necessarily perform the complete
recursive lookup itself. Instead, it can *forward* some or all of the
queries that it cannot satisfy from its cache to another caching name
server, commonly referred to as a *forwarder*.

Forwarders are typically used when an administrator does not wish for
all the servers at a given site to interact directly with the rest of
the Internet. For example, a common scenario is when multiple internal
DNS servers are behind an Internet firewall. Servers behind the firewall
forward their requests to the server with external access, which queries
Internet DNS servers on the internal servers' behalf.

Another scenario (largely now superseded by Response Policy Zones) is to
send queries first to a custom server for RBL processing before
forwarding them to the wider Internet.

There may be one or more forwarders in a given setup. The order in which
the forwarders are listed in ``named.conf`` does not determine the
sequence in which they are queried; rather, ``named`` uses the response
times from previous queries to select the server that is likely to
respond the most quickly. A server that has not yet been queried is
given an initial small random response time to ensure that it is tried
at least once. Dynamic adjustment of the recorded response times ensures
that all forwarders are queried, even those with slower response times.
This permits changes in behavior based on server responsiveness.

.. _multi_role:

Name Servers in Multiple Roles
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The BIND name server can simultaneously act as a primary for some zones,
a secondary for other zones, and as a caching (recursive) server for a set
of local clients.

However, since the functions of authoritative name service and
caching/recursive name service are logically separate, it is often
advantageous to run them on separate server machines. A server that only
provides authoritative name service (an *authoritative-only* server) can
run with recursion disabled, improving reliability and security. A
server that is not authoritative for any zones and only provides
recursive service to local clients (a *caching-only* server) does not
need to be reachable from the Internet at large and can be placed inside
a firewall.
