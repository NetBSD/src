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

.. _introduction:

Introduction to DNS and BIND 9
==============================

The Internet Domain Name System (DNS) consists of:

- the syntax to specify the names of entities in the Internet in a hierarchical manner,
- the rules used for delegating authority over names, and
- the system implementation that actually maps names to Internet addresses.

DNS data is maintained in a group of distributed hierarchical databases.

.. _doc_scope:

Scope of Document
-----------------

The Berkeley Internet Name Domain (BIND) software implements a domain name server
for a number of operating systems. This document provides basic
information about the installation and maintenance of Internet Systems
Consortium (ISC) BIND version 9 software package for system
administrators.

This manual covers BIND version |release|.

.. _organization:

Organization of This Document
-----------------------------

:ref:`introduction` introduces the basic DNS and BIND concepts. Some tutorial material on
:ref:`dns_overview` is presented for those unfamiliar with DNS. A
:ref:`intro_dns_security` is provided to allow BIND operators to implement
appropriate security for their operational environment.

:ref:`requirements` describes the hardware and environment requirements for BIND 9
and lists both the supported and unsupported platforms.

:ref:`configuration` is intended as a quickstart guide for newer users. Sample files
are included for :ref:`config_auth_samples` (both :ref:`primary<sample_primary>` and
:ref:`secondary<sample_secondary>`), as well as a simple :ref:`config_resolver_samples` and
a :ref:`sample_forwarding`. Some reference material on the :ref:`Zone File<zone_file>` is included.

:ref:`ns_operations` covers basic BIND 9 software and DNS operations, including some
useful tools, Unix signals, and plugins.

:ref:`advanced` builds on the configurations of :ref:`configuration`, adding
functions and features the system administrator may need.

:ref:`security` covers most aspects of BIND 9 security, including file permissions,
running BIND 9 in a "jail," and securing file transfers and dynamic updates.

:ref:`dnssec` describes the theory and practice of cryptographic authentication of DNS
information. The :ref:`dnssec_guide` is a practical guide to implementing DNSSEC.

:ref:`Reference` gives exhaustive descriptions of all supported blocks, statements,
and grammars used in BIND 9's ``named.conf`` configuration file.

:ref:`troubleshooting` provides information on identifying and solving BIND 9 and DNS
problems. Information about bug-reporting procedures is also provided.

:ref:`build_bind` is a definitive guide for those occasions where the user requires
special options not provided in the standard Linux or Unix distributions.

The **Appendices** contain useful reference information, such as a bibliography and historic
information related to BIND and the Domain Name System, as well as the current *man*
pages for all the published tools.

.. _conventions:

Conventions Used in This Document
---------------------------------

In this document, we generally use ``fixed-width`` text to indicate the
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

Text in "quotes," **bold text**, or *italics* is also used for emphasis or clarity.
