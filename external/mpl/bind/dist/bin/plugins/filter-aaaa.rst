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


.. highlight: console

.. _man_filter-aaaa:

filter-aaaa.so - filter AAAA in DNS responses when A is present
---------------------------------------------------------------

Synopsis
~~~~~~~~

:program:`plugin query` "filter-aaaa.so" [{ parameters }];

Description
~~~~~~~~~~~

``filter-aaaa.so`` is a query plugin module for ``named``, enabling
``named`` to omit some IPv6 addresses when responding to clients.

Until BIND 9.12, this feature was implemented natively in ``named`` and
enabled with the ``filter-aaaa`` ACL and the ``filter-aaaa-on-v4`` and
``filter-aaaa-on-v6`` options. These options are now deprecated in
``named.conf``, but can be passed as parameters to the
``filter-aaaa.so`` plugin, for example:

::

   plugin query "/usr/local/lib/filter-aaaa.so" {
           filter-aaaa-on-v4 yes;
           filter-aaaa-on-v6 yes;
           filter-aaaa { 192.0.2.1; 2001:db8:2::1; };
   };

This module is intended to aid transition from IPv4 to IPv6 by
withholding IPv6 addresses from DNS clients which are not connected to
the IPv6 Internet, when the name being looked up has an IPv4 address
available. Use of this module is not recommended unless absolutely
necessary.

Note: This mechanism can erroneously cause other servers not to give
AAAA records to their clients. If a recursing server with both IPv6 and
IPv4 network connections queries an authoritative server using this
mechanism via IPv4, it will be denied AAAA records even if its client is
using IPv6.

Options
~~~~~~~

``filter-aaaa``
   Specifies a list of client addresses for which AAAA filtering is to
   be applied. The default is ``any``.

``filter-aaaa-on-v4``
   If set to ``yes``, the DNS client is at an IPv4 address, in
   ``filter-aaaa``, and if the response does not include DNSSEC
   signatures, then all AAAA records are deleted from the response. This
   filtering applies to all responses and not only authoritative
   responses.

   If set to ``break-dnssec``, then AAAA records are deleted even when
   DNSSEC is enabled. As suggested by the name, this causes the response
   to fail to verify, because the DNSSEC protocol is designed to detect
   deletions.

   This mechanism can erroneously cause other servers not to give AAAA
   records to their clients. A recursing server with both IPv6 and IPv4
   network connections that queries an authoritative server using this
   mechanism via IPv4 will be denied AAAA records even if its client is
   using IPv6.

``filter-aaaa-on-v6``
   Identical to ``filter-aaaa-on-v4``, except it filters AAAA responses
   to queries from IPv6 clients instead of IPv4 clients. To filter all
   responses, set both options to ``yes``.

See Also
~~~~~~~~

BIND 9 Administrator Reference Manual.
