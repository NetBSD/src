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

Notes for BIND 9.20.20
----------------------

Security Fixes
~~~~~~~~~~~~~~

- Fix a use-after-free error in ``dns_client_resolve()`` triggered by a
  DNAME response.

  This issue only affected the :iscman:`delv` tool and it has now been
  fixed.

  ISC would like to thank Vitaly Simonovich for bringing this
  vulnerability to our attention. :gl:`#5728`

Feature Changes
~~~~~~~~~~~~~~~

- Record query time for all dnstap responses.

  Not all DNS responses had the query time set in their corresponding
  dnstap messages. This has been fixed. :gl:`#3695`

- Optimize TCP source port selection on Linux.

  Enable the ``IP_LOCAL_PORT_RANGE`` socket option on the outgoing TCP
  sockets to allow faster selection of the source <address,port> tuple
  for different destination <address,port> tuples, when nearing over
  70-80% of the source port utilization. :gl:`!11569`

Bug Fixes
~~~~~~~~~

- Fix an assertion failure triggered by non-minimal IXFRs.

  Processing an IXFR that included an RRset whose contents were not
  changed by the transfer triggered an assertion failure. This has been
  fixed. :gl:`#5759`

- Fix a crash when retrying a NOTIFY over TCP.

  Furthermore, do not attempt to retry over TCP at all if the source
  address is not available. :gl:`#5457`

- Fetch loop detection improvements.

  Fix a case where an in-domain nameserver with expired glue would fail
  to resolve. :gl:`#5588`

- Randomize nameserver selection.

  Since BIND 9.20.17, when selecting nameserver addresses to be looked
  up, :iscman:`named` selected them in DNSSEC order from the start of
  the NS RRset. This could lead to a resolution failure despite there
  being an address that could be resolved using the other nameserver
  names. :iscman:`named` now randomizes the order in which nameserver
  addresses are looked up. :gl:`#5695` :gl:`#5745`

- Fix dnstap logging of forwarded queries. :gl:`#5724`

- A stale answer could have been served in case of multiple upstream
  failures when following CNAME chains. This has been fixed. :gl:`#5751`

- Fail DNSKEY validation when supported but invalid DS is found.

  A regression was introduced in BIND 9.20.6 when adding the EDE code
  for unsupported DNSKEY and DS algorithms. When the parent had both
  supported and unsupported algorithms in the DS record, the validator
  would treat the supported DS algorithm as insecure instead of bogus
  when validating DNSKEY records. This has no security impact, as the
  rest of the child zone correctly ends with bogus status, but it is
  incorrect and thus the regression has been fixed. :gl:`#5757`

- Importing an invalid SKR file might corrupt stack memory.

  If an administrator imported an invalid SKR file, the local stack in
  the import function might overflow. This could lead to a memory
  corruption on the stack and ultimately a server crash. This has been
  fixed. :gl:`#5758`

- Return FORMERR for queries with the EDNS Client Subnet FAMILY field
  set to 0.

  :rfc:`7871` only defines families 1 (IPv4) and 2 (IPv6), and requires
  FORMERR to be returned for all unknown families. Queries with the EDNS
  Client Subnet FAMILY field set to 0 now elicit responses with
  RCODE=FORMERR. :gl:`!11565`
