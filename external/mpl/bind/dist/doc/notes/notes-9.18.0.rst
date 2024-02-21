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

Notes for BIND 9.18.0
---------------------

.. note:: This section only lists changes since BIND 9.16.25, the most
          recent release on the previous stable branch of BIND before
          the publication of BIND 9.18.0.

Known Issues
~~~~~~~~~~~~

- ``rndc`` has been updated to use the new BIND network manager API. As
  the network manager currently has no support for UNIX-domain sockets,
  those cannot now be used with ``rndc``. This will be addressed in a
  future release, either by restoring UNIX-domain socket support or by
  formally declaring them to be obsolete in the control channel.
  :gl:`#1759`

- See :ref:`above <relnotes_known_issues>` for a list of all known
  issues affecting this BIND 9 branch.

New Features
~~~~~~~~~~~~

- ``named`` now supports securing DNS traffic using Transport Layer
  Security (TLS). TLS is used by both DNS over TLS (DoT) and
  DNS over HTTPS (DoH).

  ``named`` can use either a certificate provided by the user or an
  ephemeral certificate generated automatically upon startup. The
  :any:`tls` block allows fine-grained control over TLS
  parameters. :gl:`#1840` :gl:`#2795` :gl:`#2796`

  For debugging purposes, ``named`` logs TLS pre-master secrets when the
  ``SSLKEYLOGFILE`` environment variable is set. This enables
  troubleshooting of issues with encrypted traffic. :gl:`#2723`

- Support for DNS over TLS (DoT) has been added to ``named``. Network
  interfaces for DoT are configured using the existing
  :ref:`listen-on <interfaces>` directive, while TLS parameters are
  configured using the new :any:`tls` block. :gl:`#1840`

  ``named`` supports :rfc:`zone transfers over TLS <9103>`
  (XFR-over-TLS, XoT) for both incoming and outgoing zone transfers.

  Incoming zone transfers over TLS are enabled by adding the :any:`tls`
  keyword, followed by either the name of a previously configured
  :any:`tls` block or the string ``ephemeral``, to the
  addresses included in :any:`primaries` lists.
  :gl:`#2392`

  Similarly, the :any:`allow-transfer` option
  was extended to accept additional ``port`` and ``transport``
  parameters, to further restrict outgoing zone transfers to a
  particular port and/or DNS transport protocol. :gl:`#2776`

  Note that zone transfers over TLS (XoT) require the ``dot``
  Application-Layer Protocol Negotiation (ALPN) token to be selected in
  the TLS handshake, as required by :rfc:`9103` section 7.1. This might
  cause issues with non-compliant XoT servers. :gl:`#2794`

  The ``dig`` tool is now able to send DoT queries (``+tls`` option).
  :gl:`#1840`

  There is currently no support for forwarding DNS queries via DoT.

- Support for DNS over HTTPS (DoH) has been added to ``named``. Both
  TLS-encrypted and unencrypted connections are supported (the latter
  may be used to offload encryption to other software). Network
  interfaces for DoH are configured using the existing
  :ref:`listen-on <interfaces>` directive, while TLS parameters are
  configured using the new :any:`tls` block and HTTP
  parameters are configured using the new :any:`http` block.
  :gl:`#1144` :gl:`#2472`

  Server-side quotas on both the number of concurrent DoH connections
  and the number of active HTTP/2 streams per connection can be
  configured using the global :any:`http-listener-clients` and
  :any:`http-streams-per-connection` options, or the :any:`listener-clients`
  and :any:`streams-per-connection` parameters in an
  :any:`http block <http>`. :gl:`#2809`

  The ``dig`` tool is now able to send DoH queries (``+https`` option).
  :gl:`#1641`

  There is currently no support for forwarding DNS queries via DoH.

  DoH support can be disabled at compile time using a new build-time
  option, ``--disable-doh``. This allows BIND 9 to be built without the
  `libnghttp2`_ library. :gl:`#2478`

- A new logging category, ``rpz-passthru``, was added, which allows RPZ
  passthru actions to be logged into a separate channel. :gl:`#54`

- A new option, ``nsdname-wait-recurse``, has been added to the
  :any:`response-policy` clause in the configuration file. When set to
  ``no``, RPZ NSDNAME rules are only applied if the authoritative
  nameservers for the query name have been looked up and are present in
  the cache. If this information is not present, the RPZ NSDNAME rules
  are ignored, but the information is looked up in the background and
  applied to subsequent queries. The default is ``yes``, meaning that
  RPZ NSDNAME rules should always be applied, even if the information
  needs to be looked up first. :gl:`#1138`

- Support for HTTPS and SVCB record types now also includes ADDITIONAL
  section processing for these record types. :gl:`#1132`

- New configuration options, :any:`tcp-receive-buffer`,
  :any:`tcp-send-buffer`, :any:`udp-receive-buffer`, and :any:`udp-send-buffer`,
  have been added. These options allow the operator to fine-tune the
  receiving and sending buffers in the operating system. On busy
  servers, increasing the size of the receive buffers can prevent the
  server from dropping packets during short traffic spikes, and
  decreasing it can prevent the server from becoming clogged with
  queries that are too old and have already timed out. :gl:`#2313`

- New finer-grained :any:`update-policy` rule types,
  ``krb5-subdomain-self-rhs`` and ``ms-subdomain-self-rhs``, were added.
  These rule types restrict updates to SRV and PTR records so that their
  content can only match the machine name embedded in the Kerberos
  principal making the change. :gl:`#481`

- Per-type record count limits can now be specified in :any:`update-policy`
  statements, to limit the number of records of a particular type that
  can be added to a domain name via dynamic update. :gl:`#1657`

- Support for OpenSSL 3.0 APIs was added. :gl:`#2843` :gl:`#3057`

- Extended DNS Error Code 18 - Prohibited (see :rfc:`8914` section
  4.19) is now set if query access is denied to the specific client.
  :gl:`#1836`

- ``ipv4only.arpa`` is now served when DNS64 is configured. :gl:`#385`

- ``dig`` can now report the DNS64 prefixes in use (``+dns64prefix``).
  This is useful when the host on which ``dig`` is run is behind an
  IPv6-only link, using DNS64/NAT64 or 464XLAT for IPv4aaS (IPv4 as a
  Service). :gl:`#1154`

- ``dig`` output now includes the transport protocol used (UDP, TCP,
  TLS, HTTPS). :gl:`#1144` :gl:`#1816`

- ``dig +qid=<num>`` allows the user to specify a particular query ID
  for testing purposes. :gl:`#1851`

.. _libnghttp2: https://nghttp2.org/

Removed Features
~~~~~~~~~~~~~~~~

- Support for the ``map`` zone file format (``masterfile-format map;``)
  has been removed. Users relying on the ``map`` format are advised to
  convert their zones to the ``raw`` format with ``named-compilezone``
  and change the configuration appropriately prior to upgrading BIND 9.
  :gl:`#2882`

- Old-style Dynamically Loadable Zones (DLZ) drivers that had to be
  enabled in ``named`` at build time have been removed. New-style DLZ
  modules should be used as a replacement. :gl:`#2814`

- Support for compiling and running BIND 9 natively on Windows has been
  completely removed. The last stable release branch that has working
  Windows support is BIND 9.16. :gl:`#2690`

- Native PKCS#11 support has been removed. :gl:`#2691`

  When built against OpenSSL 1.x, BIND 9 now
  :ref:`uses engine_pkcs11 for PKCS#11 <pkcs11>`. engine_pkcs11 is an
  OpenSSL engine which is part of the `OpenSC`_ project.

  As support for so-called "engines" was deprecated in OpenSSL 3.x,
  compiling BIND 9 against an OpenSSL 3.x build which does not retain
  support for deprecated APIs makes it impossible to use PKCS#11 in BIND
  9. A replacement for engine_pkcs11 which employs the new "provider"
  approach introduced in OpenSSL 3.x is in the making. :gl:`#2843`

- The utilities ``dnssec-checkds``, ``dnssec-coverage``, and
  ``dnssec-keymgr`` have been removed from the BIND distribution, as well
  as the ``isc`` Python package. DNSSEC features formerly provided
  by these utilities are now integrated into ``named``.
  See the :any:`dnssec-policy` configuration option
  for more details.

  An archival version of the Python utilities has been moved to
  the repository https://gitlab.isc.org/isc-projects/dnssec-keymgr/.
  Please note these tools are no longer supported by ISC.

- Since the old socket manager API has been removed, "socketmgr"
  statistics are no longer reported by the
  :any:`statistics-channels`. :gl:`#2926`

- The :any:`glue-cache` *option* has been marked as deprecated. The glue
  cache *feature* still works and will be permanently *enabled* in a
  future release. :gl:`#2146`

- A number of non-working configuration options that had been marked as
  obsolete in previous releases have now been removed completely. Using
  any of the following options is now considered a configuration
  failure: ``acache-cleaning-interval``, ``acache-enable``,
  ``additional-from-auth``, ``additional-from-cache``,
  ``allow-v6-synthesis``, ``cleaning-interval``, ``dnssec-enable``,
  ``dnssec-lookaside``, ``filter-aaaa``, ``filter-aaaa-on-v4``,
  ``filter-aaaa-on-v6``, ``geoip-use-ecs``, ``lwres``,
  ``max-acache-size``, ``nosit-udp-size``, ``queryport-pool-ports``,
  ``queryport-pool-updateinterval``, ``request-sit``, ``sit-secret``,
  ``support-ixfr``, ``use-queryport-pool``, ``use-ixfr``. :gl:`#1086`

- The ``dig`` option ``+unexpected`` has been removed. :gl:`#2140`

- IPv6 sockets are now explicitly restricted to sending and receiving
  IPv6 packets only. As this breaks the ``+mapped`` option for ``dig``,
  the option has been removed. :gl:`#3093`

- Disable and disallow static linking of BIND 9 binaries and libraries
  as BIND 9 modules require ``dlopen()`` support and static linking also
  prevents using security features like read-only relocations (RELRO) or
  address space layout randomization (ASLR) which are important for
  programs that interact with the network and process arbitrary user
  input. :gl:`#1933`

- The ``--with-gperftools-profiler`` ``configure`` option was removed.
  To use the gperftools profiler, the ``HAVE_GPERFTOOLS_PROFILER`` macro
  now needs to be manually set in ``CFLAGS`` and ``-lprofiler`` needs to
  be present in ``LDFLAGS``. :gl:`!4045`

.. _OpenSC: https://github.com/OpenSC/libp11

Feature Changes
~~~~~~~~~~~~~~~

- Aggressive Use of DNSSEC-Validated Cache (:any:`synth-from-dnssec`, see
  :rfc:`8198`) is now enabled by default again, after having been
  disabled in BIND 9.14.8. The implementation of this feature was
  reworked to achieve better efficiency and tuned to ignore certain
  types of broken NSEC records. Negative answer synthesis is currently
  only supported for zones using NSEC. :gl:`#1265`

- The default NSEC3 parameters for :any:`dnssec-policy` were updated to no
  extra SHA-1 iterations and no salt (``NSEC3PARAM 1 0 0 -``). This
  change is in line with the `latest NSEC3 recommendations`_.
  :gl:`#2956`

- The default for :any:`dnssec-dnskey-kskonly` was changed to ``yes``. This
  means that DNSKEY, CDNSKEY, and CDS RRsets are now only signed with
  the KSK by default. The additional signatures prepared using the ZSK
  when the option is set to ``no`` add to the DNS response payload
  without offering added value. :gl:`#1316`

- ``dnssec-cds`` now only generates SHA-2 DS records by default and
  avoids copying deprecated SHA-1 records from a child zone to its
  delegation in the parent. If the child zone does not publish SHA-2 CDS
  records, ``dnssec-cds`` will generate them from the CDNSKEY records.
  The ``-a algorithm`` option now affects the process of generating DS
  digest records from both CDS and CDNSKEY records. Thanks to Tony
  Finch. :gl:`#2871`

- Previously, ``named`` accepted FORMERR responses both with and without
  an OPT record, as an indication that a given server did not support
  EDNS. To implement full compliance with :rfc:`6891`, only FORMERR
  responses without an OPT record are now accepted. This intentionally
  breaks communication with servers that do not support EDNS and that
  incorrectly echo back the query message with the RCODE field set to
  FORMERR and the QR bit set to 1. :gl:`#2249`

- The question section is now checked when processing AXFR, IXFR, and
  SOA replies while transferring a zone in. :gl:`#1683`

- DNS Flag Day 2020: the EDNS buffer size probing code, which made the
  resolver adjust the EDNS buffer size used for outgoing queries based
  on the successful query responses and timeouts observed, was removed.
  The resolver now always uses the EDNS buffer size set in
  :any:`edns-udp-size` for all outgoing queries. :gl:`#2183`

- Keeping stale answers in cache (:any:`stale-cache-enable`) has been
  disabled by default. :gl:`#1712`

- Overall memory use by ``named`` has been optimized and significantly
  reduced, especially for resolver workloads. :gl:`#2398` :gl:`#3048`

- Memory allocation is now based on the memory allocation API provided
  by the `jemalloc`_ library, on platforms where it is available. Use of
  this library is now recommended when building BIND 9; although it is
  optional, it is enabled by default. :gl:`#2433`

- Internal data structures maintained for each cache database are now
  grown incrementally when they need to be expanded. This helps maintain
  a steady response rate on a loaded resolver while these internal data
  structures are resized. :gl:`#2941`

- The interface handling code has been refactored to use fewer
  resources, which should lead to less memory fragmentation and better
  startup performance. :gl:`#2433`

- When reporting zone types in the statistics channel, the terms
  :any:`primary <type primary>` and :any:`secondary <type secondary>` are now used instead of ``master`` and
  ``slave``, respectively. :gl:`#1944`

- The ``rndc nta -dump`` and ``rndc secroots`` commands now both include
  :any:`validate-except` entries when listing negative trust anchors. These
  are indicated by the keyword ``permanent`` in place of the expiry
  date. :gl:`#1532`

- The output of ``rndc serve-stale status`` has been clarified. It now
  explicitly reports whether retention of stale data in the cache is
  enabled (:any:`stale-cache-enable`), and whether returning such data in
  responses is enabled (:any:`stale-answer-enable`). :gl:`#2742`

- Previously, using ``dig +bufsize=0`` had the side effect of disabling
  EDNS, and there was no way to test the remote server's behavior when
  it had received a packet with EDNS0 buffer size set to 0. This is no
  longer the case; ``dig +bufsize=0`` now sends a DNS message with EDNS
  version 0 and buffer size set to 0. To disable EDNS, use ``dig
  +noedns``. :gl:`#2054`

- BIND 9 binaries which are neither daemons nor administrative programs
  were moved to ``$bindir``. Only ``ddns-confgen``, ``named``, ``rndc``,
  ``rndc-confgen``, and ``tsig-confgen`` were left in ``$sbindir``.
  :gl:`#1724`

- The BIND 9 build system has been changed to use a typical
  autoconf+automake+libtool stack. This should not make any difference
  for people building BIND 9 from release tarballs, but when building
  BIND 9 from the Git repository, ``autoreconf -fi`` needs to be run
  first. Extra attention is also needed when using non-standard
  ``configure`` options. :gl:`#4`

.. _latest NSEC3 recommendations: https://datatracker.ietf.org/doc/html/draft-ietf-dnsop-nsec3-guidance-02

.. _jemalloc: http://jemalloc.net/

Bug Fixes
~~~~~~~~~

- Log files using ``timestamp``-style suffixes were not always correctly
  removed when the number of files exceeded the limit set by
  ``versions``. This has been fixed. :gl:`#828`
