.. 
   Copyright (C) Internet Systems Consortium, Inc. ("ISC")
   
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/.
   
   See the COPYRIGHT file distributed with this work for additional
   information regarding copyright ownership.

Notes for BIND 9.16.2
---------------------

Security Fixes
~~~~~~~~~~~~~~

-  DNS rebinding protection was ineffective when BIND 9 is configured as
   a forwarding DNS server. Found and responsibly reported by Tobias
   Klein. [GL #1574]

Known Issues
~~~~~~~~~~~~

-  We have received reports that in some circumstances, receipt of an
   IXFR can cause the processing of queries to slow significantly. Some
   of these were related to RPZ processing, which has been fixed in this
   release (see below). Others appear to occur where there are
   NSEC3-related changes (such as an operator changing the NSEC3 salt
   used in the hash calculation). These are being investigated. [GL
   #1685]

Feature Changes
~~~~~~~~~~~~~~~

-  The previous DNSSEC sign statistics used lots of memory. The number
   of keys to track is reduced to four per zone, which should be enough
   for 99% of all signed zones. [GL #1179]

Bug Fixes
~~~~~~~~~

-  When an RPZ policy zone was updated via zone transfer and a large
   number of records was deleted, ``named`` could become nonresponsive
   for a short period while deleted names were removed from the RPZ
   summary database. This database cleanup is now done incrementally
   over a longer period of time, reducing such delays. [GL #1447]

-  When trying to migrate an already-signed zone from
   ``auto-dnssec maintain`` to one based on ``dnssec-policy``, the
   existing keys were immediately deleted and replaced with new ones. As
   the key rollover timing constraints were not being followed, it was
   possible that some clients would not have been able to validate
   responses until all old DNSSEC information had timed out from caches.
   BIND now looks at the time metadata of the existing keys and
   incorporates it into its DNSSEC policy operation. [GL #1706]
