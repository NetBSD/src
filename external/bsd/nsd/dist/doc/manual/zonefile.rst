Zonefile example
================

On this page we give an example of a basic zone file and it's contents.

We recommend using the :command:`nsd-checkzone` tool to verify that you have a working zone.

Creating a zone
---------------

A minimal zone needs exactly one SOA (Source Of Authority) and one or more NS (Name Server) records. Refer to appropriate documentation if you need to learn about DNS basics.

.. code:: bash

	$ORIGIN example.com.
	$TTL 86400 ; default time-to-live for this zone

	example.com.   IN  SOA     ns.example.com. noc.dns.example.org. (
	        2020080302  ;Serial
	        7200        ;Refresh
	        3600        ;Retry
	        1209600     ;Expire
	        3600        ;Negative response caching TTL
	)

	; The nameservers that are authoritative for this zone.
					NS	example.com.

	; A and AAAA records are for IPv4 and IPv6 addresses respectively
	example.com.	A	192.0.2.1
					AAAA 2001:db8::3

	; A CNAME redirects from www.example.com to example.com
	www				CNAME   example.com.

	mail			MX	10	example.com.


.. could add this structure eventually: <name> <ttl> <class> <type> <rdata>


.. Note:: In the example above the ``SOA`` record class is set to: ``IN``. The record class is omitted in the remaining records, in which case the resulting value will be set from the preceding record.
	Meaning the ``NS``, ``A`` and ``MX`` source records have implicitly the ``IN`` record class.

.. Note:: The first domain in the ``SOA`` (start of authority) record, is the authoritative master name server for the zone. 
	The second domain is actually the administrator email addresss for this zone. Where the first dot (``.``) will be converted into an ``@`` sign. 
	So in this example above, the email is: ``noc@dns.example.org``. If you have a dot in the local-part of the email address, you need to backslash the dot.

.. Note:: You can use the ``@`` symbol in the zone file for a shortcut to the origin of the zone domain. 
	Meaning ``example.com. IN  SOA`` could also have been written to: ``@ IN  SOA``. Same can be applied to other records as well, like the A record: ``@  A  192.0.2.1``. Which is the same as: ``example.com.  A  192.0.2.1``
