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

.. Security:

BIND 9 Security Considerations
==============================

.. _Access_Control_Lists:

Access Control Lists
--------------------

Access Control Lists (ACLs) are address match lists that can be set up
and nicknamed for future use in ``allow-notify``, ``allow-query``,
``allow-query-on``, ``allow-recursion``, ``blackhole``,
``allow-transfer``, ``match-clients``, etc.

ACLs give users finer control over who can access the
name server, without cluttering up configuration files with huge lists of
IP addresses.

It is a *good idea* to use ACLs, and to control access.
Limiting access to the server by outside parties can help prevent
spoofing and denial of service (DoS) attacks against the server.

ACLs match clients on the basis of up to three characteristics: 1) The
client's IP address; 2) the TSIG or SIG(0) key that was used to sign the
request, if any; and 3) an address prefix encoded in an EDNS
Client-Subnet option, if any.

Here is an example of ACLs based on client addresses:

::

   // Set up an ACL named "bogusnets" that blocks
   // RFC1918 space and some reserved space, which is
   // commonly used in spoofing attacks.
   acl bogusnets {
       0.0.0.0/8;  192.0.2.0/24; 224.0.0.0/3;
       10.0.0.0/8; 172.16.0.0/12; 192.168.0.0/16;
   };

   // Set up an ACL called our-nets. Replace this with the
   // real IP numbers.
   acl our-nets { x.x.x.x/24; x.x.x.x/21; };
   options {
     ...
     ...
     allow-query { our-nets; };
     allow-recursion { our-nets; };
     ...
     blackhole { bogusnets; };
     ...
   };

   zone "example.com" {
     type primary;
     file "m/example.com";
     allow-query { any; };
   };

This allows authoritative queries for ``example.com`` from any address,
but recursive queries only from the networks specified in ``our-nets``,
and no queries at all from the networks specified in ``bogusnets``.

In addition to network addresses and prefixes, which are matched against
the source address of the DNS request, ACLs may include ``key``
elements, which specify the name of a TSIG or SIG(0) key.

When BIND 9 is built with GeoIP support, ACLs can also be used for
geographic access restrictions. This is done by specifying an ACL
element of the form: ``geoip db database field value``.

The ``field`` parameter indicates which field to search for a match. Available fields
are ``country``, ``region``, ``city``, ``continent``, ``postal`` (postal code),
``metro`` (metro code), ``area`` (area code), ``tz`` (timezone), ``isp``,
``asnum``, and ``domain``.

``value`` is the value to search for within the database. A string may be quoted
if it contains spaces or other special characters. An ``asnum`` search for
autonomous system number can be specified using the string "ASNNNN" or the
integer NNNN. If a ``country`` search is specified with a string that is two characters
long, it must be a standard ISO-3166-1 two-letter country code; otherwise,
it is interpreted as the full name of the country.  Similarly, if
``region`` is the search term and the string is two characters long, it is treated as a
standard two-letter state or province abbreviation; otherwise, it is treated as the
full name of the state or province.

The ``database`` field indicates which GeoIP database to search for a match. In
most cases this is unnecessary, because most search fields can only be found in
a single database.  However, searches for ``continent`` or ``country`` can be
answered from either the ``city`` or ``country`` databases, so for these search
types, specifying a ``database`` forces the query to be answered from that
database and no other. If a ``database`` is not specified, these queries
are first answered from the ``city`` database if it is installed, and then from the ``country``
database if it is installed. Valid database names are ``country``,
``city``, ``asnum``, ``isp``, and ``domain``.

Some example GeoIP ACLs:

::

   geoip country US;
   geoip country JP;
   geoip db country country Canada;
   geoip region WA;
   geoip city "San Francisco";
   geoip region Oklahoma;
   geoip postal 95062;
   geoip tz "America/Los_Angeles";
   geoip org "Internet Systems Consortium";

ACLs use a "first-match" logic rather than "best-match"; if an address
prefix matches an ACL element, then that ACL is considered to have
matched even if a later element would have matched more specifically.
For example, the ACL ``{ 10/8; !10.0.0.1; }`` would actually match a
query from 10.0.0.1, because the first element indicates that the query
should be accepted, and the second element is ignored.

When using "nested" ACLs (that is, ACLs included or referenced within
other ACLs), a negative match of a nested ACL tells the containing ACL to
continue looking for matches. This enables complex ACLs to be
constructed, in which multiple client characteristics can be checked at
the same time. For example, to construct an ACL which allows a query
only when it originates from a particular network *and* only when it is
signed with a particular key, use:

::

   allow-query { !{ !10/8; any; }; key example; };

Within the nested ACL, any address that is *not* in the 10/8 network
prefix is rejected, which terminates the processing of the ACL.
Any address that *is* in the 10/8 network prefix is accepted, but
this causes a negative match of the nested ACL, so the containing ACL
continues processing. The query is accepted if it is signed by
the key ``example``, and rejected otherwise. The ACL, then, only
matches when *both* conditions are true.

.. _chroot_and_setuid:

``Chroot`` and ``Setuid``
-------------------------

On Unix servers, it is possible to run BIND in a *chrooted* environment
(using the ``chroot()`` function) by specifying the ``-t`` option for
``named``. This can help improve system security by placing BIND in a
"sandbox," which limits the damage done if a server is compromised.

Another useful feature in the Unix version of BIND is the ability to run
the daemon as an unprivileged user (``-u`` user). We suggest running
as an unprivileged user when using the ``chroot`` feature.

Here is an example command line to load BIND in a ``chroot`` sandbox,
``/var/named``, and to run ``named`` ``setuid`` to user 202:

``/usr/local/sbin/named -u 202 -t /var/named``

.. _chroot:

The ``chroot`` Environment
~~~~~~~~~~~~~~~~~~~~~~~~~~

For a ``chroot`` environment to work properly in a particular
directory (for example, ``/var/named``), the
environment must include everything BIND needs to run. From BIND's
point of view, ``/var/named`` is the root of the filesystem;
the values of options like ``directory`` and ``pid-file``
must be adjusted to account for this.

Unlike with earlier versions of BIND, 
``named`` does *not* typically need to be compiled statically, nor do shared libraries need to be installed under the new
root. However, depending on the operating system, it may be necessary to set
up locations such as ``/dev/zero``, ``/dev/random``, ``/dev/log``, and
``/etc/localtime``.

.. _setuid:

Using the ``setuid`` Function
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Prior to running the ``named`` daemon, use the ``touch`` utility (to
change file access and modification times) or the ``chown`` utility (to
set the user id and/or group id) on files where BIND should
write.

.. note::

   If the ``named`` daemon is running as an unprivileged user, it
   cannot bind to new restricted ports if the server is
   reloaded.

.. _dynamic_update_security:

Dynamic Update Security
-----------------------

Access to the dynamic update facility should be strictly limited. In
earlier versions of BIND, the only way to do this was based on the IP
address of the host requesting the update, by listing an IP address or
network prefix in the ``allow-update`` zone option. This method is
insecure, since the source address of the update UDP packet is easily
forged. Also note that if the IP addresses allowed by the
``allow-update`` option include the address of a secondary server which
performs forwarding of dynamic updates, the primary can be trivially
attacked by sending the update to the secondary, which forwards it to
the primary with its own source IP address - causing the primary to approve
it without question.

For these reasons, we strongly recommend that updates be
cryptographically authenticated by means of transaction signatures
(TSIG). That is, the ``allow-update`` option should list only TSIG key
names, not IP addresses or network prefixes. Alternatively, the
``update-policy`` option can be used.

Some sites choose to keep all dynamically updated DNS data in a
subdomain and delegate that subdomain to a separate zone. This way, the
top-level zone containing critical data, such as the IP addresses of
public web and mail servers, need not allow dynamic updates at all.
