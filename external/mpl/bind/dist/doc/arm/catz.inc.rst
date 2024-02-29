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
https://datatracker.ietf.org/doc/draft-toorop-dnsop-dns-catalog-zones/ .

Principle of Operation
~~~~~~~~~~~~~~~~~~~~~~

Normally, if a zone is to be served by a secondary server, the
:iscman:`named.conf` file on the server must list the zone, or the zone must
be added using :option:`rndc addzone`. In environments with a large number of
secondary servers, and/or where the zones being served are changing
frequently, the overhead involved in maintaining consistent zone
configuration on all the secondary servers can be significant.

A catalog zone is a way to ease this administrative burden: it is a DNS
zone that lists member zones that should be served by secondary servers.
When a secondary server receives an update to the catalog zone, it adds,
removes, or reconfigures member zones based on the data received.

To use a catalog zone, it must first be set up as a normal zone on both the
primary and secondary servers that are configured to use it. It
must also be added to a :any:`catalog-zones` list in the :namedconf:ref:`options` or
:any:`view` statement in :iscman:`named.conf`. This is comparable to the way a
policy zone is configured as a normal zone and also listed in a
:any:`response-policy` statement.

To use the catalog zone feature to serve a new member zone:

-  Set up the member zone to be served on the primary as normal. This
   can be done by editing :iscman:`named.conf` or by running
   :option:`rndc addzone`.

-  Add an entry to the catalog zone for the new member zone. This can
   be done by editing the catalog zone's zone file and running
   :option:`rndc reload`, or by updating the zone using :iscman:`nsupdate`.

The change to the catalog zone is propagated from the primary to all
secondaries using the normal AXFR/IXFR mechanism. When the secondary receives the
update to the catalog zone, it detects the entry for the new member
zone, creates an instance of that zone on the secondary server, and points
that instance to the :any:`primaries` specified in the catalog zone data. The
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
:option:`rndc delzone`.

Configuring Catalog Zones
~~~~~~~~~~~~~~~~~~~~~~~~~
.. namedconf:statement:: catalog-zones
   :tags: zone
   :short: Configures catalog zones in :iscman:`named.conf`.

Catalog zones are configured with a :any:`catalog-zones` statement in the
:namedconf:ref:`options` or :any:`view` section of :iscman:`named.conf`. For example:

::

   catalog-zones {
       zone "catalog.example"
            default-primaries { 10.53.0.1; }
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
   Synonym for ``default-primaries``.

``default-primaries``
   This option defines the default primaries for member
   zones listed in a catalog zone, and can be overridden by options within
   a catalog zone. If no such options are included, then member zones
   transfer their contents from the servers listed in this option.

``in-memory``
   This option, if set to ``yes``, causes member zones to be
   stored only in memory. This is functionally equivalent to configuring a
   secondary zone without a :any:`file` option. The default is ``no``; member
   zones' content is stored locally in a file whose name is
   automatically generated from the view name, catalog zone name, and
   member zone name.

``zone-directory``
   This option causes local copies of member zones' zone files to be
   stored in the specified directory, if ``in-memory`` is not set to
   ``yes``. The default is to store zone files in the server's working
   directory. A non-absolute pathname in ``zone-directory`` is assumed
   to be relative to the working directory.

``min-update-interval``
   This option sets the minimum interval between updates to catalog
   zones, in seconds. If an update to a catalog zone (for example, via
   IXFR) happens less than ``min-update-interval`` seconds after the
   most recent update, the changes are not carried out until this
   interval has elapsed. The default is 5 seconds.

Catalog zones are defined on a per-view basis. Configuring a non-empty
:any:`catalog-zones` statement in a view automatically turns on
:any:`allow-new-zones` for that view. This means that :option:`rndc addzone`
and :option:`rndc delzone` also work in any view that supports catalog
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
   catalog.example.    IN NS invalid.
   version.catalog.example.    IN TXT "2"

Note that this record must have the domain name
``version.catalog-zone-name``. The data
stored in a catalog zone is indicated by the domain name label
immediately before the catalog zone domain. Currently BIND supports catalog zone
schema versions "1" and "2".

Also note that the catalog zone must have an NS record in order to be a valid
DNS zone, and using the value "invalid." for NS is recommended.

A member zone is added by including a ``PTR`` resource record in the
``zones`` sub-domain of the catalog zone. The record label can be any unique label.
The target of the PTR record is the member zone name. For example, to add member zones
``domain.example`` and ``domain2.example``:

::

   5960775ba382e7a4e09263fc06e7c00569b6a05c.zones.catalog.example. IN PTR domain.example.
   uniquelabel.zones.catalog.example. IN PTR domain2.example.

The label is necessary to identify custom properties (see below) for a specific member zone.
Also, the zone state can be reset by changing its label, in which case BIND will remove
the member zone and add it back.

Catalog Zone Custom Properties
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

BIND uses catalog zones custom properties to define different properties which
can be set either globally for the whole catalog
zone or for a single member zone. Global custom properties override the settings
in the configuration file, and member zone custom properties override global
custom properties.

For the version "1" of the schema custom properties must be placed without a special suffix.

For the version "2" of the schema custom properties must be placed under the ".ext" suffix.

Global custom properties are set at the apex of the catalog zone, e.g.:

::

    primaries.ext.catalog.example.    IN AAAA 2001:db8::1

BIND currently supports the following custom properties:

-  A simple :any:`primaries` definition:

   ::

           primaries.ext.catalog.example.    IN A 192.0.2.1


   This custom property defines a primary server for the member zones, which can be
   either an A or AAAA record. If multiple primaries are set, the order in
   which they are used is random.

   Note: ``masters`` can be used as a synonym for :any:`primaries`.

-  A :any:`primaries` with a TSIG key defined:

   ::

               label.primaries.ext.catalog.example.     IN A 192.0.2.2
               label.primaries.ext.catalog.example.     IN TXT "tsig_key_name"


   This custom property defines a primary server for the member zone with a TSIG
   key set. The TSIG key must be configured in the configuration file.
   ``label`` can be any valid DNS label.

   Note: ``masters`` can be used as a synonym for :any:`primaries`.

-  :any:`allow-query` and :any:`allow-transfer` ACLs:

   ::

               allow-query.ext.catalog.example.   IN APL 1:10.0.0.1/24
               allow-transfer.ext.catalog.example.    IN APL !1:10.0.0.1/32 1:10.0.0.0/24


   These custom properties are the equivalents of :any:`allow-query` and
   :any:`allow-transfer` options in a zone declaration in the :iscman:`named.conf`
   configuration file. The ACL is processed in order; if there is no
   match to any rule, the default policy is to deny access. For the
   syntax of the APL RR, see :rfc:`3123`.

The member zone-specific custom properties are defined the same way as global
custom properties, but in the member zone subdomain:

::

   primaries.ext.5960775ba382e7a4e09263fc06e7c00569b6a05c.zones.catalog.example. IN A 192.0.2.2
   label.primaries.ext.5960775ba382e7a4e09263fc06e7c00569b6a05c.zones.catalog.example. IN AAAA 2001:db8::2
   label.primaries.ext.5960775ba382e7a4e09263fc06e7c00569b6a05c.zones.catalog.example. IN TXT "tsig_key_name"
   allow-query.ext.5960775ba382e7a4e09263fc06e7c00569b6a05c.zones.catalog.example. IN APL 1:10.0.0.0/24
   primaries.ext.uniquelabel.zones.catalog.example. IN A 192.0.2.3

Custom properties defined for a specific zone override the
global custom properties defined in the catalog zone. These in turn override the
global options defined in the :any:`catalog-zones` statement in the
configuration file.

Note that none of the global records for a custom property are inherited if any
records are defined for that custom property for the specific zone. For example,
if the zone had a :any:`primaries` record of type A but not AAAA, it
would *not* inherit the type AAAA record from the global custom property
or from the global option in the configuration file.

Change of Ownership (coo)
~~~~~~~~~~~~~~~~~~~~~~~~~

BIND supports the catalog zones "Change of Ownership" (coo) property. When the
same entry which exists in one catalog zone is added into another catalog zone,
the default behavior for BIND is to ignore it, and continue serving the zone
using the catalog zone where it was originally existed, unless it is removed
from there, then it can be added into the new one.

Using the ``coo`` property it is possible to gracefully move a zone from one
catalog zone into another, by letting the catalog consumers know that it is
permitted to do so. To do that, the original catalog zone should be updated with
a new record with ``coo`` custom property:

::

   uniquelabel.zones.catalog.example. IN PTR domain2.example.
   coo.uniquelabel.zones.catalog.example. IN PTR catalog2.example.

Here, the ``catalog.example`` catalog zone gives permission for the member zone
with label "uniquelabel" to be transferred into ``catalog2.example`` catalog
zone. Catalog consumers which support the ``coo`` property will then take note,
and when the zone is finally added into ``catalog2.example`` catalog zone,
catalog consumers will change the ownership of the zone from ``catalog.example``
to ``catalog2.example``. BIND's implementation simply deletes the zone from the
old catalog zone and adds it back into the new catalog zone, which also means
that all associated state for the just migrated zone will be reset, including
when the unique label is the same.

The record with ``coo`` custom property can be later deleted by the
catalog zone operator after confirming that all the consumers have received
it and have successfully changed the ownership of the zone.
