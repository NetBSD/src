.. 
   Copyright (C) Internet Systems Consortium, Inc. ("ISC")
   
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, you can obtain one at https://mozilla.org/MPL/2.0/.
   
   See the COPYRIGHT file distributed with this work for additional
   information regarding copyright ownership.

.. _getting_started:

Getting Started
---------------

.. _software_requirements:

Software Requirements
~~~~~~~~~~~~~~~~~~~~~

.. _bind_version:

BIND Version
^^^^^^^^^^^^

Most configuration examples given in this document require BIND version
9.16.0 or newer (although many do work with all versions of BIND
later than 9.9). To check the version of ``named`` you have installed,
use the ``-v`` switch as shown below:

::

   # named -v
   BIND 9.16.0 (Stable Release) <id:6270e602ea>

Some configuration examples are added in BIND version 9.17 and backported
to 9.16. For example, NSEC3 configuration requires BIND version 9.16.9.

We recommend you run the latest stable version to get the most complete
DNSSEC configuration, as well as the latest security fixes.

.. _dnssec_support_in_bind:

DNSSEC Support in BIND
^^^^^^^^^^^^^^^^^^^^^^

All versions of BIND 9 since BIND 9.7 can support DNSSEC, as currently
deployed in the global DNS, so the BIND software you are running most
likely already supports DNSSEC. Run the command ``named -V``
to see what flags it was built with. If it was built with OpenSSL
(``--with-openssl``), then it supports DNSSEC. Below is an example
of the output from running ``named -V``:

::

   $ named -V
   BIND 9.16.0 (Stable Release) <id:6270e602ea>
   running on Linux x86_64 4.9.0-9-amd64 #1 SMP Debian 4.9.168-1+deb9u4 (2019-07-19)
   built by make with defaults
   compiled by GCC 6.3.0 20170516
   compiled with OpenSSL version: OpenSSL 1.1.0l  10 Sep 2019
   linked to OpenSSL version: OpenSSL 1.1.0l  10 Sep 2019
   compiled with libxml2 version: 2.9.4
   linked to libxml2 version: 20904
   compiled with json-c version: 0.12.1
   linked to json-c version: 0.12.1
   compiled with zlib version: 1.2.8
   linked to zlib version: 1.2.8
   threads support is enabled

   default paths:
     named configuration:  /usr/local/etc/named.conf
     rndc configuration:   /usr/local/etc/rndc.conf
     DNSSEC root key:      /usr/local/etc/bind.keys
     nsupdate session key: /usr/local/var/run/named/session.key
     named PID file:       /usr/local/var/run/named/named.pid
     named lock file:      /usr/local/var/run/named/named.lock

If the BIND 9 software you have does not support DNSSEC, you should
upgrade it. (It has not been possible to build BIND without DNSSEC
support since BIND 9.13, released in 2018.) As well as missing out on
DNSSEC support, you are also missing a number of security fixes
made to the software in recent years.

.. _system_entropy:

System Entropy
^^^^^^^^^^^^^^

To deploy DNSSEC to your authoritative server, you
need to generate cryptographic keys. The amount of time it takes to
generate the keys depends on the source of randomness, or entropy, on
your systems. On some systems (especially virtual machines) with
insufficient entropy, it may take much longer than one cares to wait to
generate keys.

There are software packages, such as ``haveged`` for Linux, that
provide additional entropy for a system. Once installed, they
significantly reduce the time needed to generate keys.

The more entropy there is, the better pseudo-random numbers you get, and
the stronger the keys that are generated. If you want or need high-quality random
numbers, take a look at :ref:`hardware_security_modules` for some of
the hardware-based solutions.

.. _hardware_requirements:

Hardware Requirements
~~~~~~~~~~~~~~~~~~~~~

.. _recursive_server_hardware:

Recursive Server Hardware
^^^^^^^^^^^^^^^^^^^^^^^^^

Enabling DNSSEC validation on a recursive server makes it a *validating
resolver*. The job of a validating resolver is to fetch additional
information that can be used to computationally verify the answer set.
Below are the areas that should be considered for possible hardware
enhancement for a validating resolver:

1. *CPU*: a validating resolver executes cryptographic functions on many
   of the answers returned, which usually leads to increased CPU usage,
   unless your recursive server has built-in hardware to perform
   cryptographic computations.

2. *System memory*: DNSSEC leads to larger answer sets and occupies
   more memory space.

3. *Network interfaces*: although DNSSEC does increase the amount of DNS
   traffic overall, it is unlikely that you need to upgrade your network
   interface card (NIC) on the name server unless you have some truly
   outdated hardware.

One factor to consider is the destinations of your current DNS
traffic. If your current users spend a lot of time visiting ``.gov`` 
websites, you should expect a jump in all of the above
categories when validation is enabled, because ``.gov`` is more than 90%
signed. This means that more than 90% of the time, your validating resolver
will be doing what is described in
:ref:`how_does_dnssec_change_dns_lookup`. However, if your users
only care about resources in the ``.com`` domain, which, as of mid-2020,
is under 1.5% signed [#]_, your recursive name server is unlikely
to experience a significant load increase after enabling DNSSEC
validation.

.. _authoritative_server_hardware:

Authoritative Server Hardware
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

On the authoritative server side, DNSSEC is enabled on a zone-by-zone
basis. When a zone is DNSSEC-enabled, it is also known as "signed."
Below are the areas to consider for possible hardware
enhancements for an authoritative server with signed zones:

1. *CPU*: a DNSSEC-signed zone requires periodic re-signing, which is a
   cryptographic function that is CPU-intensive. If your DNS zone is
   dynamic or changes frequently, that also adds to higher CPU loads.

2. *System storage*: A signed zone is definitely larger than an unsigned
   zone. How much larger? See
   :ref:`your_zone_before_and_after_dnssec` for a comparison
   example. Roughly speaking, you should expect your zone file to grow by at
   least three times, and frequently more.

3. *System memory*: Larger DNS zone files take up not only more storage
   space on the file system, but also more space when they are loaded
   into system memory.

4. *Network interfaces*: While your authoritative name servers will
   begin sending back larger responses, it is unlikely that you need to
   upgrade your network interface card (NIC) on the name server unless
   you have some truly outdated hardware.

One factor to consider, but over which you really have no control, is
the number of users who query your domain name who themselves have DNSSEC enabled. It was
estimated in late 2014 that roughly 10% to 15% of the Internet DNS
queries were DNSSEC-aware. Estimates by `APNIC <https://www.apnic.net/>`__
suggest that in 2020 about `one-third <https://stats.labs.apnic.net/dnssec>`__ of all queries are
validating queries, although the percentage varies widely on a
per-country basis. This means that more DNS queries for your domain will
take advantage of the additional security features, which will result in
increased system load and possibly network traffic.

.. [#]
   https://rick.eng.br/dnssecstat

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
