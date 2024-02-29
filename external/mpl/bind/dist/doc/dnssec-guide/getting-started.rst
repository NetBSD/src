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

.. _getting_started:

Getting Started
---------------

.. _software_requirements:

Software Requirements
~~~~~~~~~~~~~~~~~~~~~

This guide assumes BIND 9.18.0 or newer, although the more elaborate manual
procedures do work with all versions of BIND later than 9.9.

We recommend running the latest stable version to get the most
complete DNSSEC configuration, as well as the latest security fixes.

.. _hardware_requirements:

Hardware Requirements
~~~~~~~~~~~~~~~~~~~~~

.. _recursive_server_hardware:

Recursive Server Hardware
^^^^^^^^^^^^^^^^^^^^^^^^^

Enabling DNSSEC validation on a recursive server makes it a *validating
resolver*. The job of a validating resolver is to fetch additional
information that can be used to computationally verify the answer set.
Contrary to popular belief, the increase in resource consumption is very modest:

1. *CPU*: a validating resolver executes cryptographic functions on cache-miss
   answers, which leads to increased CPU usage. Thanks to standard DNS caching
   and contemporary CPUs, the increase in CPU-time consumption in a steady
   state is negligible - typically on the order of 5%. For a brief period (a few
   minutes) after the resolver starts, the increase might be as much as 20%, but it
   quickly decreases as the DNS cache fills in.

2. *System memory*: DNSSEC leads to larger answer sets and occupies
   more memory space. With typical ISP traffic and the state of the Internet as
   of mid-2022, memory consumption for the cache increases by roughly 20%.

3. *Network interfaces*: although DNSSEC does increase the amount of DNS
   traffic overall, in practice this increase is often within measurement
   error.

.. _authoritative_server_hardware:

Authoritative Server Hardware
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

On the authoritative server side, DNSSEC is enabled on a zone-by-zone
basis. When a zone is DNSSEC-enabled, it is also known as "signed."
Below are the expected changes to resource consumption caused by serving
DNSSEC-signed zones:

1. *CPU*: a DNSSEC-signed zone requires periodic re-signing, which is a
   cryptographic function that is CPU-intensive. If your DNS zone is
   dynamic or changes frequently, that also adds to higher CPU loads.

2. *System storage*: A signed zone is definitely larger than an unsigned
   zone. How much larger? See
   :ref:`your_zone_before_and_after_dnssec` for a comparison
   example. The final size depends on the structure of the zone, the signing algorithm,
   the number of keys, the choice of NSEC or NSEC3, the ratio of signed delegations, the zone file
   format, etc. Usually, the size of a signed zone ranges from a negligible
   increase to as much as three times the size of the unsigned zone.

3. *System memory*: Larger DNS zone files take up not only more storage
   space on the file system, but also more space when they are loaded
   into system memory. The final memory consumption also depends on all the
   variables listed above: in the typical case the increase is around half of
   the unsigned zone memory consumption, but it can be as high as three times
   for some corner cases.

4. *Network interfaces*: While your authoritative name servers will
   begin sending back larger responses, it is unlikely that you need to
   upgrade your network interface card (NIC) on the name server unless
   you have some truly outdated hardware.

One factor to consider, but over which you really have no control, is
the number of users who query your domain name who themselves have DNSSEC
enabled. As of mid-2022, measurements by `APNIC
<https://stats.labs.apnic.net/dnssec>`__ show 41% of Internet users send
DNSSEC-aware queries. This means that more DNS queries for your domain will
take advantage of the additional security features, which will result in
increased system load and possibly network traffic.

.. _network_requirements:

Network Requirements
~~~~~~~~~~~~~~~~~~~~

From a network perspective, DNS and DNSSEC packets are very similar;
DNSSEC packets are just bigger, which means DNS is more likely to use
TCP. You should test for the following two items to make sure your
network is ready for DNSSEC:

1. *DNS over TCP*: Verify network connectivity over TCP port 53, which
   may mean updating firewall policies or Access Control Lists (ACL) on
   routers. See :ref:`dns_uses_tcp` for more details.

2. *Large UDP packets*: Some network equipment, such as firewalls, may
   make assumptions about the size of DNS UDP packets and incorrectly
   reject DNS traffic that appears "too big." Verify that the
   responses your name server generates are being seen by the rest of the
   world: see :ref:`whats_edns0_all_about` for more details.

.. _operational_requirements:

Operational Requirements
~~~~~~~~~~~~~~~~~~~~~~~~

.. _parent_zone:

Parent Zone
^^^^^^^^^^^

Before starting your DNSSEC deployment, check with your parent zone
administrators to make sure they support DNSSEC. This may or may not be
the same entity as your registrar. As you will see later in
:ref:`working_with_parent_zone`, a crucial step in DNSSEC deployment
is establishing the parent-child trust relationship. If your parent zone
does not yet support DNSSEC, contact that administrator to voice your concerns.

.. _security_requirements:

Security Requirements
^^^^^^^^^^^^^^^^^^^^^

Some organizations may be subject to stricter security requirements than
others. Check to see if your organization requires stronger
cryptographic keys be generated and stored, and how often keys need to be
rotated. The examples presented in this document are not intended for
high-value zones. We cover some of these security considerations in
:ref:`dnssec_advanced_discussions`.
