Catalog zones
=============

Since version 4.9.0, NSD has support for Catalog zones version "2" as specified
in `RFC 9432 <https://www.rfc-editor.org/rfc/rfc9432>`_. NSD can be a producer
of catalog zones as well as a catalog zone consumer, but it is limited to
process only a single consumer zone.

Setting up NSD as a catalog consumer
------------------------------------

NSD will process a zone as a catalog consumer zone if the zone has the
``catalog: consumer`` option set. An example catalog consumer configuration
could look like this:

.. code:: bash

	pattern:
		name: "member-zone-config"
		request-xfr: 198.51.100.1 NOKEY
		allow-notify: 198.51.100.1 NOKEY

	key:
		name: tsig-key.name
		algorithm: hmac-sha256
		secret: "SXMgdGhpcyBhIHNlY3JldCBvciBqdXN0IHRleHQ/Pz8="

	tls-auth:
		name: primary.example
		auth-domain-name: primary.example

	zone:
		name: "catalog1.invalid"
		catalog: consumer
		catalog-member-pattern: "member-zone-config"

		request-xfr: 192.0.2.1@853 tsig-key.name primary.example
		allow-notify: 192.0.2.1 tsig-key.name

		allow-query: BLOCKED

The consumer zone ``catalog1.invalid`` is configured in the example as a
secondary zone. It transfers the catalog from the primary at ``192.0.2.1``.
The transfer is mutually authenticated :doc:`using TSIG<running/using-tsig>`.

The content of catalog zones are only relevant for the name servers handling
those zones. They contain a list of zones that are served from the name
servers and it is likely undesirable to expose that content. We have protected
the zone against queries from third-parties by setting the ``allow-query:
BLOCKED`` option. The transfer is protected against on-path eavesdroppers by
doing it over :doc:`authenticated TLS<running/xot>`.

.. Note:: Using privacy preserving option is RECOMMENDED for catalog zones
	(See `RFC 9432 Sections 6 and 7
	<https://www.rfc-editor.org/rfc/rfc9432#section-7>`_).

.. Note:: Catalog consumer zones do not need to be secondary, they may also
	process just zone files.

NSD supports the `group property
<https://www.rfc-editor.org/rfc/rfc9432#name-groups-group-property>`_. Member
zones from the catalog will be added with the pattern given by the group
property of that member. If a member does not have a group property or its
value is invalid or doesn't match a pattern, the pattern given by the
``catalog-member-pattern:`` option will be used.

Using nsd-control to get catalog zone status
--------------------------------------------

The status of catalog zones and catalog member zones can be consulted with
:command:`nsd-control zonestatus`.

.. code:: bash

	$ nsd-control zonestatus

	zone:   catalog1.invalid
		catalog: consumer (serial: 1708341939, # members: 2)
		state: ok
		served-serial: "1708341939 since 2024-02-19T15:19:44"
		commit-serial: "1708341939 since 2024-02-19T15:19:44"
		wait: "3461 sec between attempts"

	zone:   example.net
		pattern: member-zone-config
		catalog-member-id: a5b75379.zones.catalog1.invalid.
		state: ok
		served-serial: "2024013019 since 2024-02-19T14:25:43"
		commit-serial: "2024013019 since 2024-02-19T14:25:43"
		wait: "7195 sec between attempts"

	zone:   example.org
		pattern: group1
		catalog-member-id: 96143f7d.zones.catalog1.invalid.
		state: ok
		served-serial: "2024013016 since 2024-02-19T14:18:10"
		commit-serial: "2024013016 since 2024-02-19T14:18:10"
		wait: "6544 sec between attempts"

The first ``zone:`` entry in the example output above shows the status our
configured consumer zone ``catalog1.invalid``. Besides its role (``consumer``
or ``producer``) it show the last SOA serial number that was successfully
processed, and the number of member zones that were added by processing the
consumer zone.

.. Note:: If the catalog zone has become invalid and isn't processed
	anymore, :command:`nsd-control zonestatus` will show the reason why.

:command:`nsd-control zonestatus` will also show the ``catalog-member-id`` of
catalog member zones. In the example output of :command:`nsd-control
zonestatus` above we can see that ``example.net`` and ``example.org`` are
member zones from ``catalog1.invalid``. Apparently the ``example.net`` member
did not have a valid group property, because it has been added with the default
``catalog-member-pattern:`` ``member-zone-config``.

Setting up NSD as a catalog producer
------------------------------------
A catalog producer zone can be configured in NSD by setting the ``catalog:
producer`` option. Unlike consumer zones, multiple producer zones may be
configured. NSD creates the content of producer zones and therefore producer
zones cannot be configured as secondary zones.  Likewise, ``zonefile:`` options
are only used to write the zone, never to read it.

An example catalog producer configuration could look like this:

.. code:: bash

	server:
		interface: 192.0.2.1@853
		tls-port: 853
		tls-service-key: "primary.example.key.pem"
		tls-service-pem: "primary.example.cert.pem"

	pattern:
		name: "group0"
		catalog-producer-zone: "catalog1.invalid"

	pattern:
		name: "group1"
		catalog-producer-zone: "catalog1.invalid"

	key:
		name: tsig-key.name
		algorithm: hmac-sha256
		secret: "SXMgdGhpcyBhIHNlY3JldCBvciBqdXN0IHRleHQ/Pz8="

	zone:
		name: "catalog1.invalid"
		catalog: producer

		store-ixfr: yes
		provide-xfr: 203.0.113.1@853 tsig-key.name
		notify: 203.0.113.1 tsig-key.name

		allow-query: BLOCKED

The producer zone is configured as a primary and allows (in our example)
transfer of the zone over TLS only. Also, just like with the consumer zone
configuration example above, queries to this zone are ``BLOCKED`` to comply
with `RECOMMENDED <https://www.rfc-editor.org/rfc/rfc9432#section-7>`_ privacy
and security considerations. We also recommend - for primary zones in general -
to serve *incremental* transfers (configured with ``store-ixfr: yes``).

Zones can be added as member zones, by adding them to NSD with
:command:`nsd-control addzone` with a pattern that has the name of the producer
zone as value of a ``catalog-producer-zone:`` option. In the example
configuration above, patterns ``"group0"`` and ``"group1"`` both have that
option.

Here is an example on how to do that:

.. code:: bash

	$ nsd-control addzone example.net group0
	ok
	$ nsd-control addzone example.org group1
	ok

Like with consumer zones and consumer member zones, :command:`nsd-control
zonestatus` can be used to check on the status of catalog producer zones and
its members:

.. code:: bash

	$ nsd-control zonestatus

	zone:   catalog1.invalid
		catalog: producer (serial: 1708341939, # members: 2)
		state: primary

	zone:   example.net
		pattern: group0
		catalog-member-id: a5b75379.zones.catalog1.invalid.
		state: primary

	zone:   example.org
		pattern: group1
		catalog-member-id: 96143f7d.zones.catalog1.invalid.
		state: primary

Like with other zones added with :command:`nsd-control addzone`, the member
zones are persistently added to the zone list file (see the ``zonelistfile:``
configure option). The content of the catalog producer zone is not persistent
and will be reconstructed from the member zone entries in the zone list file.

.. code:: bash

	$ cat /var/db/nsd/zone.list
	# NSD zone list
	# name pattern
	cat example.net group0 a5b75379
	cat example.org group1 96143f7d

