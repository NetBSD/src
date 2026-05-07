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

.. _troubleshooting:

Troubleshooting
===============

.. _common_problems:

Common Problems
---------------

It's Not Working; How Can I Figure Out What's Wrong?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The best solution to installation and configuration issues is to
take preventive measures by setting up logging files beforehand. The
log files provide hints and information that can be used to
identify anything that went wrong and fix the problem.

EDNS Compliance Issues
~~~~~~~~~~~~~~~~~~~~~~

EDNS (Extended DNS) is a standard that was first specified in 1999. It
is required for DNSSEC validation, DNS COOKIE options, and other
features. There are broken and outdated DNS servers and firewalls still
in use which misbehave when queried with EDNS; for example, they may
drop EDNS queries rather than replying with FORMERR. BIND and other
recursive name servers have traditionally employed workarounds in this
situation, retrying queries in different ways and eventually falling
back to plain DNS queries without EDNS.

Such workarounds cause unnecessary resolution delays, increase code
complexity, and prevent deployment of new DNS features. In February
2019, all major DNS software vendors removed these
workarounds; see https://www.dnsflagday.net/2019/ for further details. This change
was implemented in BIND as of release 9.14.0.

As a result, some domains may be non-resolvable without manual
intervention. In these cases, resolution can be restored by adding
:namedconf:ref:`server` clauses for the offending servers, or by specifying ``edns no`` or
``send-cookie no``, depending on the specific noncompliance.

To determine which :namedconf:ref:`server` clause to use, run the following commands
to send queries to the authoritative servers for the broken domain:

::

           dig soa <zone> @<server> +dnssec
           dig soa <zone> @<server> +dnssec +nocookie
           dig soa <zone> @<server> +noedns


If the first command fails but the second succeeds, the server most
likely needs ``send-cookie no``. If the first two fail but the third
succeeds, then the server needs EDNS to be fully disabled with
``edns no``.

Please contact the administrators of noncompliant domains and encourage
them to upgrade their broken DNS servers.

Inspecting Encrypted DNS Traffic
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. note::

   This feature requires support from the cryptographic library that
   BIND 9 is built against.  For OpenSSL, version 1.1.1 or newer is
   required (use :option:`named -V` to check).

By definition, TLS-encrypted traffic (e.g. DNS over TLS, DNS over HTTPS)
is opaque to packet sniffers, which makes debugging problems with
encrypted DNS close to impossible. However, Wireshark_ offers a
solution_ to this problem by being able to read key log files. In order
to make :iscman:`named` prepare such a file, set the ``SSLKEYLOGFILE``
environment variable to either:

- the string ``config`` (``SSLKEYLOGFILE=config``); this requires
  defining a :any:`logging` :any:`channel` which will
  handle messages belonging to the ``sslkeylog`` category,

- the path to the key file to write (``SSLKEYLOGFILE=/path/to/file``);
  this is equivalent to the following :any:`logging` configuration:

  ::

     channel default_sslkeylogfile {
         file "${SSLKEYLOGFILE}" versions 10 size 100m suffix timestamp;
     };

     category sslkeylog {
         default_sslkeylogfile;
     };

.. note::

   When using ``SSLKEYLOGFILE=config``, augmenting the log channel
   output using options like :any:`print-time` or :any:`print-severity` is
   strongly discouraged as it will likely make the key log file
   unusable.

When the ``SSLKEYLOGFILE`` environment variable is set, each TLS
connection established by :iscman:`named` (both incoming and outgoing) causes
about 1 kilobyte of data to be written to the key log file.

.. warning::

   Due to the limitations of the current logging code in BIND 9,
   enabling TLS pre-master secret logging adversely affects :iscman:`named`
   performance.

.. _Wireshark: https://www.wireshark.org/
.. _solution: https://wiki.wireshark.org/TLS

Incrementing and Changing the Serial Number
-------------------------------------------

Zone serial numbers are just numbers — they are not date-related. However, many
people set them to a number that represents a date, usually of the
form YYYYMMDDRR. Occasionally they make a mistake and set the serial number to a
date in the future, then try to correct it by setting it to the
current date. This causes problems because serial numbers are used to
indicate that a zone has been updated. If the serial number on the secondary
server is lower than the serial number on the primary, the secondary server
attempts to update its copy of the zone.

Setting the serial number to a lower number on the primary server than the one
on the secondary server means that the secondary will not perform updates to its
copy of the zone.

The solution to this is to add 2147483647 (2^31-1) to the number, reload
the zone and make sure all secondaries have updated to the new zone serial
number, then reset it to the desired number and reload the
zone again.

.. _more_help:

Where Can I Get Help?
---------------------
The BIND-users mailing list, at https://lists.isc.org/mailman/listinfo/bind-users, is an excellent resource for
peer user support. In addition, ISC maintains a Knowledgebase of helpful articles
at https://kb.isc.org.

Internet Systems Consortium (ISC) offers annual support agreements
for BIND 9, ISC DHCP, and Kea DHCP.
All paid support contracts include advance security notifications; some levels include
service level agreements (SLAs), premium software features, and increased priority on bug fixes
and feature requests.

Please contact info@isc.org or visit
https://www.isc.org/contact/ for more information.
