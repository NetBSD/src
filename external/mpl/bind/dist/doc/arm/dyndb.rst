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

.. _dyndb-info:

Dynamic Database (DynDB)
------------------------

Dynamic Database, or DynDB, is an extension to BIND 9 which, like DLZ (see
:ref:`dlz-info`), allows zone data to be retrieved from an external
database. Unlike DLZ, a DynDB module provides a full-featured BIND zone
database interface. Where DLZ translates DNS queries into real-time
database lookups, resulting in relatively poor query performance, and is
unable to handle DNSSEC-signed data due to its limited API, a DynDB
module can pre-load an in-memory database from the external data source,
providing the same performance and functionality as zones served
natively by BIND.

A DynDB module supporting LDAP has been created by Red Hat and is
available from https://pagure.io/bind-dyndb-ldap.

A sample DynDB module for testing and developer guidance is included
with the BIND source code, in the directory
``bin/tests/system/dyndb/driver``.

Configuring DynDB
~~~~~~~~~~~~~~~~~

A DynDB database is configured with a ``dyndb`` statement in
``named.conf``:

::

       dyndb example "driver.so" {
           parameters
       };


The file ``driver.so`` is a DynDB module which implements the full DNS
database API. Multiple ``dyndb`` statements can be specified, to load
different drivers or multiple instances of the same driver. Zones
provided by a DynDB module are added to the view's zone table, and are
treated as normal authoritative zones when BIND responds to
queries. Zone configuration is handled internally by the DynDB module.

The parameters are passed as an opaque string to the DynDB module's
initialization routine. Configuration syntax differs depending on
the driver.

Sample DynDB Module
~~~~~~~~~~~~~~~~~~~

For guidance in the implementation of DynDB modules, the directory
``bin/tests/system/dyndb/driver`` contains a basic DynDB module. The
example sets up two zones, whose names are passed to the module as
arguments in the ``dyndb`` statement:

::

       dyndb sample "sample.so" { example.nil. arpa. };


In the above example, the module is configured to create a zone
"example.nil" which can answer queries and AXFR requests and accept
DDNS updates. At runtime, prior to any updates, the zone contains an
SOA, NS, and a single A record at the apex:

::

    example.nil.  86400    IN      SOA     example.nil. example.nil. (
                                                  0 28800 7200 604800 86400
                                          )
    example.nil.  86400    IN      NS      example.nil.
    example.nil.  86400    IN      A       127.0.0.1


When the zone is updated dynamically, the DynDB module determines
whether the updated RR is an address (i.e., type A or AAAA) and if so,
it automatically updates the corresponding PTR record in a reverse
zone. Note that updates are not stored permanently; all updates are lost when the
server is restarted.
