.. 
   Copyright (C) Internet Systems Consortium, Inc. ("ISC")
   
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/.
   
   See the COPYRIGHT file distributed with this work for additional
   information regarding copyright ownership.

   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/.

   See the COPYRIGHT file distributed with this work for additional
   information regarding copyright ownership.

.. _dlz-info:

Dynamically Loadable Zones (DLZ)
--------------------------------

Dynamically Loadable Zones (DLZ) are an extension to BIND 9 that allows
zone data to be retrieved directly from an external database. There is
no required format or schema. DLZ drivers exist for several different
database backends, including PostgreSQL, MySQL, and LDAP, and can be
written for any other.

Historically, DLZ drivers had to be statically linked with the ``named``
binary and were turned on via a configure option at compile time (for
example, ``configure --with-dlz-ldap``). The drivers
provided in the BIND 9 tarball in ``contrib/dlz/drivers`` are still
linked this way.

In BIND 9.8 and higher, it is possible to link some DLZ modules
dynamically at runtime, via the DLZ "dlopen" driver, which acts as a
generic wrapper around a shared object implementing the DLZ API. The
"dlopen" driver is linked into ``named`` by default, so configure
options are no longer necessary when using these dynamically linkable
drivers, but are still needed for the older drivers in
``contrib/dlz/drivers``.

The DLZ module provides data to ``named`` in text
format, which is then converted to DNS wire format by ``named``. This
conversion, and the lack of any internal caching, places significant
limits on the query performance of DLZ modules. Consequently, DLZ is not
recommended for use on high-volume servers. However, it can be used in a
hidden primary (master) configuration, with secondaries retrieving zone updates via
AXFR. Note, however, that DLZ has no built-in support for DNS notify;
secondary servers are not automatically informed of changes to the zones in the
database.

Configuring DLZ
~~~~~~~~~~~~~~~

A DLZ database is configured with a ``dlz`` statement in ``named.conf``:

::

       dlz example {
       database "dlopen driver.so args";
       search yes;
       };


This specifies a DLZ module to search when answering queries; the module
is implemented in ``driver.so`` and is loaded at runtime by the dlopen
DLZ driver. Multiple ``dlz`` statements can be specified; when answering
a query, all DLZ modules with ``search`` set to ``yes`` are queried
to see whether they contain an answer for the query name. The best
available answer is returned to the client.

The ``search`` option in the above example can be omitted, because
``yes`` is the default value.

If ``search`` is set to ``no``, this DLZ module is *not* searched
for the best match when a query is received. Instead, zones in this DLZ
must be separately specified in a zone statement. This allows users to
configure a zone normally using standard zone-option semantics, but
specify a different database backend for storage of the zone's data.
For example, to implement NXDOMAIN redirection using a DLZ module for
backend storage of redirection rules:

::

       dlz other {
       database "dlopen driver.so args";
       search no;
       };

       zone "." {
       type redirect;
       dlz other;
       };


Sample DLZ Driver
~~~~~~~~~~~~~~~~~

For guidance in the implementation of DLZ modules, the directory
``contrib/dlz/example`` contains a basic dynamically linkable DLZ
module - i.e., one which can be loaded at runtime by the "dlopen" DLZ
driver. The example sets up a single zone, whose name is passed to the
module as an argument in the ``dlz`` statement:

::

       dlz other {
       database "dlopen driver.so example.nil";
       };


In the above example, the module is configured to create a zone
"example.nil", which can answer queries and AXFR requests and accept
DDNS updates. At runtime, prior to any updates, the zone contains an
SOA, NS, and a single A record at the apex:

::

    example.nil.  3600    IN      SOA     example.nil. hostmaster.example.nil. (
                              123 900 600 86400 3600
                          )
    example.nil.  3600    IN      NS      example.nil.
    example.nil.  1800    IN      A       10.53.0.1


The sample driver can retrieve information about the
querying client and alter its response on the basis of this
information. To demonstrate this feature, the example driver responds to
queries for "source-addr.``zonename``>/TXT" with the source address of
the query. Note, however, that this record will *not* be included in
AXFR or ANY responses. Normally, this feature would be used to alter
responses in some other fashion, e.g., by providing different address
records for a particular name depending on the network from which the
query arrived.

Documentation of the DLZ module API can be found in
``contrib/dlz/example/README``. This directory also contains the header
file ``dlz_minimal.h``, which defines the API and should be included by
any dynamically linkable DLZ module.
