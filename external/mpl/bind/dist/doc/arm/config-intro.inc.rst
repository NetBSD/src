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

.. _configuration:

.. _sample_configuration:

Configurations and Zone Files
=============================

Introduction
------------

BIND 9 uses a single configuration file called :ref:`named.conf <named_conf>`.
which is typically located in either /etc/namedb or
/usr/local/etc/namedb.

   .. Note:: If :ref:`rndc<ops_rndc>` is being used locally (on the same host
      as BIND 9) then an additional file :iscman:`rndc.conf` may be present, though
      :iscman:`rndc` operates without this file. If :iscman:`rndc` is being run
      from a remote host then an :iscman:`rndc.conf` file must be present as it
      defines the link characteristics and properties.

Depending on the functionality of the system, one or more zone files is
required.

The samples given throughout this and subsequent chapters use a standard base
format for both the :iscman:`named.conf` and the zone files for **example.com**. The
intent is for the reader to see the evolution from a common base as features
are added or removed.

.. _base_named_conf:

``named.conf`` Base File
~~~~~~~~~~~~~~~~~~~~~~~~

This file illustrates the typical format and layout style used for
:iscman:`named.conf` and provides a basic logging service, which may be extended
as required by the user.

.. code-block:: c

        // base named.conf file
        // Recommended that you always maintain a change log in this file as shown here
        // options clause defining the server-wide properties
        options {
          // all relative paths use this directory as a base
          directory "/var";
          // version statement for security to avoid hacking known weaknesses
          // if the real version number is revealed
          version "not currently available";
        };

        // logging clause
        // log to /var/log/named/example.log all events from info UP in severity (no debug)
        // uses 3 files in rotation swaps files when size reaches 250K
        // failure messages that occur before logging is established are
        // in syslog (/var/log/messages)
        //
        logging {
          channel example_log {
            // uses a relative path name and the directory statement to
            // expand to /var/log/named/example.log
            file "log/named/example.log" versions 3 size 250k;
            // only log info and up messages - all others discarded
            severity info;
          };
          category default {
            example_log;
          };
        };

The :any:`logging` and :namedconf:ref:`options` blocks
and :any:`category`, :any:`channel`,
:any:`directory`, :any:`file`, and :any:`severity`
statements are all described further in the appropriate sections of this ARM.

.. _base_zone_file:

**example.com** base zone file
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The following is a complete zone file for the domain **example.com**, which
illustrates a number of common features. Comments in the file explain these
features where appropriate.  Zone files consist of :ref:`Resource Records (RR)
<zone_file>`, which describe the zone's characteristics or properties.

.. code-block::
        :linenos:

        ; base zone file for example.com
        $TTL 2d    ; default TTL for zone
        $ORIGIN example.com. ; base domain-name
        ; Start of Authority RR defining the key characteristics of the zone (domain)
        @         IN      SOA   ns1.example.com. hostmaster.example.com. (
                                        2003080800 ; serial number
                                        12h        ; refresh
                                        15m        ; update retry
                                        3w         ; expiry
                                        2h         ; minimum
                                        )
        ; name server RR for the domain
                   IN      NS      ns1.example.com.
        ; the second name server is external to this zone (domain)
                   IN      NS      ns2.example.net.
        ; mail server RRs for the zone (domain)
                3w IN      MX  10  mail.example.com.
        ; the second  mail servers is  external to the zone (domain)
                   IN      MX  20  mail.example.net.
        ; domain hosts includes NS and MX records defined above
        ; plus any others required
        ; for instance a user query for the A RR of joe.example.com will
        ; return the IPv4 address 192.168.254.6 from this zone file
        ns1        IN      A       192.168.254.2
        mail       IN      A       192.168.254.4
        joe        IN      A       192.168.254.6
        www        IN      A       192.168.254.7
        ; aliases ftp (ftp server) to an external domain
        ftp        IN      CNAME   ftp.example.net.

This type of zone file is frequently referred to as a **forward-mapped zone
file**, since it maps domain names to some other value, while a
:ref:`reverse-mapped zone file<ipv4_reverse>` maps an IP address to a domain
name.  The zone file is called **example.com** for no good reason except that
it is the domain name of the zone it describes; as always, users are free to
use whatever file-naming convention is appropriate to their needs.

Other Zone Files
~~~~~~~~~~~~~~~~

Depending on the configuration additional zone files may or should be present.
Their format and functionality are briefly described here.

localhost Zone File
~~~~~~~~~~~~~~~~~~~

All end-user systems are shipped with a ``hosts`` file (usually located in
/etc). This file is normally configured to map the name **localhost** (the name
used by applications when they run locally) to the loopback address. It is
argued, reasonably, that a forward-mapped zone file for **localhost** is
therefore not strictly required. This manual does use the BIND 9 distribution
file ``localhost-forward.db`` (normally in /etc/namedb/master or
/usr/local/etc/namedb/master) in all configuration samples for the following
reasons:

1. Many users elect to delete the ``hosts`` file for security reasons (it is a
   potential target of serious domain name redirection/poisoning attacks).

2. Systems normally lookup any name (including domain names) using the
   ``hosts`` file first (if present), followed by DNS. However, the
   ``nsswitch.conf`` file (typically in /etc) controls this order (normally
   **hosts: file dns**), allowing the order to be changed or the **file** value
   to be deleted entirely depending on local needs.  Unless the BIND
   administrator controls this file and knows its values, it is unsafe to
   assume that **localhost** is forward-mapped correctly.

3. As a reminder to users that unnecessary queries for **localhost** form a
   non-trivial volume of DNS queries on the public network, which affects DNS
   performance for all users.

Users may, however, elect at their discretion not to implement this file since,
depending on the operational environment, it may not be essential.

The BIND 9 distribution file ``localhost-forward.db`` format is shown for
completeness and provides for both IPv4 and IPv6 localhost resolution. The zone
(domain) name is **localhost.**

.. code-block::

        $TTL 3h
        localhost.  SOA      localhost.  nobody.localhost. 42  1d  12h  1w  3h
                    NS       localhost.
                    A        127.0.0.1
                    AAAA     ::1

.. NOTE:: Readers of a certain age or disposition may note the reference in this file to the late,
	lamented Douglas Noel Adams.

localhost Reverse-Mapped Zone File
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This zone file allows any query requesting the name associated with the
loopback IP (127.0.0.1).  This file is required to prevent unnecessary queries
from reaching the public DNS hierarchy. The BIND 9 distribution file
``localhost.rev`` is shown for completeness:

.. code-block::

        $TTL 1D
        @        IN        SOA  localhost. root.localhost. (
                                2007091701 ; serial
                                30800      ; refresh
                                7200       ; retry
                                604800     ; expire
                                300 )      ; minimum
                 IN        NS    localhost.
        1        IN        PTR   localhost.
