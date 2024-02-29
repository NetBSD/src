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

.. highlight: console

.. iscman:: filter-a
.. _man_filter-a:

filter-a.so - filter A in DNS responses when AAAA is present
---------------------------------------------------------------

Synopsis
~~~~~~~~

:program:`plugin query` "filter-a.so" [{ parameters }];

Description
~~~~~~~~~~~

:program:`filter-a.so` is a query plugin module for :iscman:`named`, enabling
:iscman:`named` to omit some IPv4 addresses when responding to clients.

For example:

::

   plugin query "filter-a.so" {
           filter-a-on-v6 yes;
           filter-a-on-v4 yes;
           filter-a { 192.0.2.1; 2001:db8:2::1; };
   };

This module is intended to aid transition from IPv4 to IPv6 by
withholding IPv4 addresses from DNS clients which are not connected to
the IPv4 Internet, when the name being looked up has an IPv6 address
available. Use of this module is not recommended unless absolutely
necessary.

Note: This mechanism can erroneously cause other servers not to give
A records to their clients. If a recursing server with both IPv6 and
IPv4 network connections queries an authoritative server using this
mechanism via IPv6, it is denied A records even if its client is
using IPv4.

Options
~~~~~~~

``filter-a``
   This option specifies a list of client addresses for which A filtering is to
   be applied. The default is ``any``.

``filter-a-on-v6``
   If set to ``yes``, this option indicates that the DNS client is at an IPv6 address, in
   ``filter-a``. If the response does not include DNSSEC
   signatures, then all A records are deleted from the response. This
   filtering applies to all responses, not only authoritative
   ones.

   If set to ``break-dnssec``, then A records are deleted even when
   DNSSEC is enabled. As suggested by the name, this causes the response
   to fail to verify, because the DNSSEC protocol is designed to detect
   deletions.

   This mechanism can erroneously cause other servers not to give A
   records to their clients. If a recursing server with both IPv6 and IPv4
   network connections queries an authoritative server using this
   mechanism via IPv6, it is denied A records even if its client is
   using IPv4.

``filter-a-on-v4``
   This option is identical to ``filter-a-on-v6``, except that it filters A responses
   to queries from IPv4 clients instead of IPv6 clients. To filter all
   responses, set both options to ``yes``.

See Also
~~~~~~~~

BIND 9 Administrator Reference Manual.
