.. 
   Copyright (C) Internet Systems Consortium, Inc. ("ISC")
   
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/.
   
   See the COPYRIGHT file distributed with this work for additional
   information regarding copyright ownership.

..
   Copyright (C) Internet Systems Consortium, Inc. ("ISC")

   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/.

   See the COPYRIGHT file distributed with this work for additional
   information regarding copyright ownership.

.. _catz-info:

Catalog Zones
-------------

A "catalog zone" is a special DNS zone that contains a list of other
zones to be served, along with their configuration parameters. Zones
listed in a catalog zone are called "member zones." When a catalog zone
is loaded or transferred to a secondary server which supports this
functionality, the secondary server creates the member zones
automatically. When the catalog zone is updated (for example, to add or
delete member zones, or change their configuration parameters), those
changes are immediately put into effect. Because the catalog zone is a
normal DNS zone, these configuration changes can be propagated using the
standard AXFR/IXFR zone transfer mechanism.

Catalog zones' format and behavior are specified as an Internet draft
for interoperability among DNS implementations. The
latest revision of the DNS catalog zones draft can be found here:
https://datatracker.ietf.org/doc/draft-toorop-dnsop-dns-catalog-zones/.

Principle of Operation
~~~~~~~~~~~~~~~~~~~~~~

Normally, if a zone is to be served by a secondary server, the
``named.conf`` file on the server must list the zone, or the zone must
be added using ``rndc addzone``. In environments with a large number of
secondary servers, and/or where the zones being served are changing
frequently, the overhead involved in maintaining consistent zone
configuration on all the secondary servers can be significant.

A catalog zone is a way to ease this administrative burden: it is a DNS
zone that lists member zones that should be served by secondary servers.
When a secondary server receives an update to the catalog zone, it adds,
removes, or reconfigures member zones based on the data received.

To use a catalog zone, it must first be set up as a normal zone on both the
primary and secondary servers that are configured to use it. It
must also be added to a ``catalog-zones`` list in the ``options`` or
``view`` statement in ``named.conf``. This is comparable to the way a
policy zone is configured as a normal zone and also listed in a
``response-policy`` statement.

To use the catalog zone feature to serve a new member zone:

-  Set up the the member zone to be served on the primary as normal. This
   can be done by editing ``named.conf`` or by running
   ``rndc addzone``.

-  Add an entry to the catalog zone for the new member zone. This can
   be done by editing the catalog zone's master file and running
   ``rndc reload``, or by updating the zone using ``nsupdate``.

The change to the catalog zone is propagated from the primary to all
secondaries using the normal AXFR/IXFR mechanism. When the secondary receives the
update to the catalog zone, it detects the entry for the new member
zone, creates an instance of that zone on the secondary server, and points
that instance to the ``masters`` specified in the catalog zone data. The
newly created member zone is a normal secondary zone, so BIND
immediately initiates a transfer of zone contents from the primary. Once
complete, the secondary starts serving the member zone.

Removing a member zone from a secondary server requires only
deleting the member zone's entry in the catalog zone; the change to the
catalog zone is propagated to the secondary server using the normal
AXFR/IXFR transfer mechanism. The secondary server, on processing the
update, notices that the member zone has been removed, stops
serving the zone, and removes it from its list of configured zones.
However, removing the member zone from the primary server must be done
by editing the configuration file or running
``rndc delzone``.

Configuring Catalog Zones
~~~~~~~~~~~~~~~~~~~~~~~~~

Catalog zones are configured with a ``catalog-zones`` statement in the
``options`` or ``view`` section of ``named.conf``. For example:

::

   catalog-zones {
       zone "catalog.example"
            default-masters { 10.53.0.1; }
            in-memory no
            zone-directory "catzones"
            min-update-interval 10;
   };

This statement specifies that the zone ``catalog.example`` is a catalog
zone. This zone must be properly configured in the same view. In most
configurations, it would be a secondary zone.

The options following the zone name are not required, and may be
specified in any order.

``default-masters``
   This option defines the default primaries for member
   zones listed in a catalog zone, and can be overridden by options within
   a catalog zone. If no such options are included, then member zones
   transfer their contents from the servers listed in this option.

``in-memory``
   This option, if set to ``yes``, causes member zones to be
   stored only in memory. This is functionally equivalent to configuring a
   secondary zone without a ``file`` option. The default is ``no``; member
   zones' content is stored locally in a file whose name is
   automatically generated from the view name, catalog zone name, and
   member zone name.

``zone-directory``
   This option causes local copies of member zones'
   master files to be stored in
   the specified directory, if ``in-memory`` is not set to ``yes``. The default is to store zone files in the
   server's working directory. A non-absolute pathname in
   ``zone-directory`` is assumed to be relative to the working directory.

``min-update-interval``
   This option sets the minimum interval between
   processing of updates to catalog zones, in seconds. If an update to a
   catalog zone (for example, via IXFR) happens less than
   ``min-update-interval`` seconds after the most recent update, the
   changes are not carried out until this interval has elapsed. The
   default is 5 seconds.

Catalog zones are defined on a per-view basis. Configuring a non-empty
``catalog-zones`` statement in a view automatically turns on
``allow-new-zones`` for that view. This means that ``rndc addzone``
and ``rndc delzone`` also work in any view that supports catalog
zones.

Catalog Zone Format
~~~~~~~~~~~~~~~~~~~

A catalog zone is a regular DNS zone; therefore, it must have a single
``SOA`` and at least one ``NS`` record.

A record stating the version of the catalog zone format is also
required. If the version number listed is not supported by the server,
then a catalog zone may not be used by that server.

::

   catalog.example.    IN SOA . . 2016022901 900 600 86400 1
   catalog.example.    IN NS nsexample.
   version.catalog.example.    IN TXT "1"

Note that this record must have the domain name
version.catalog-zone-name. The data
stored in a catalog zone is indicated by the domain name label
immediately before the catalog zone domain.

Catalog zone options can be set either globally for the whole catalog
zone or for a single member zone. Global options override the settings
in the configuration file, and member zone options override global
options.

Global options are set at the apex of the catalog zone, e.g.:

::

    masters.catalog.example.    IN AAAA 2001:db8::1

BIND currently supports the following options:

-  A simple ``masters`` definition:

   ::

           masters.catalog.example.    IN A 192.0.2.1


   This option defines a primary server for the member zones - it can be
   either an A or AAAA record. If multiple primaries are set, the order in
   which they are used is random.

-  A ``masters`` with a TSIG key defined:

   ::

               label.masters.catalog.example.     IN A 192.0.2.2
               label.masters.catalog.example.     IN TXT "tsig_key_name"


   This option defines a primary server for the member zone with a TSIG
   key set. The TSIG key must be configured in the configuration file.
   ``label`` can be any valid DNS label.

-  ``allow-query`` and ``allow-transfer`` ACLs:

   ::

               allow-query.catalog.example.   IN APL 1:10.0.0.1/24
               allow-transfer.catalog.example.    IN APL !1:10.0.0.1/32 1:10.0.0.0/24


   These options are the equivalents of ``allow-query`` and
   ``allow-transfer`` in a zone declaration in the ``named.conf``
   configuration file. The ACL is processed in order; if there is no
   match to any rule, the default policy is to deny access. For the
   syntax of the APL RR, see :rfc:`3123`.

A member zone is added by including a ``PTR`` resource record in the
``zones`` sub-domain of the catalog zone. The record label is a
``SHA-1`` hash of the member zone name in wire format. The target of the
PTR record is the member zone name. For example, to add the member zone
``domain.example``:

::

   5960775ba382e7a4e09263fc06e7c00569b6a05c.zones.catalog.example. IN PTR domain.example.

The hash is necessary to identify options for a specific member zone.
The member zone-specific options are defined the same way as global
options, but in the member zone subdomain:

::

   masters.5960775ba382e7a4e09263fc06e7c00569b6a05c.zones.catalog.example. IN A 192.0.2.2
   label.masters.5960775ba382e7a4e09263fc06e7c00569b6a05c.zones.catalog.example. IN AAAA 2001:db8::2
   label.masters.5960775ba382e7a4e09263fc06e7c00569b6a05c.zones.catalog.example. IN TXT "tsig_key"
   allow-query.5960775ba382e7a4e09263fc06e7c00569b6a05c.zones.catalog.example. IN APL 1:10.0.0.0/24

Options defined for a specific zone override the
global options defined in the catalog zone. These in turn override the
global options defined in the ``catalog-zones`` statement in the
configuration file.

Note that none of the global records for an option are inherited if any
records are defined for that option for the specific zone. For example,
if the zone had a ``masters`` record of type A but not AAAA, it
would *not* inherit the type AAAA record from the global option.
