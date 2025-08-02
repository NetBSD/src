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

Notes for BIND 9.20.0
---------------------

.. note:: This section only lists changes since BIND 9.18.28, the most
          recent release on the previous stable branch of BIND at the
          time of the publication of BIND 9.20.0.

New Features
~~~~~~~~~~~~

- The :any:`forwarders` statement now supports the :any:`tls` argument,
  to be used to forward queries to DoT-enabled servers. :gl:`#3726`

- :iscman:`named` now supports forwarding Dynamic DNS updates through
  DNS-over-TLS (DoT). :gl:`#3512`

- The :iscman:`nsupdate` tool now supports DNS-over-TLS (DoT).
  :gl:`!6752`

- The :any:`tls` block was extended with a new :any:`cipher-suites` option
  that allows permitted cipher suites for TLSv1.3 to be set. Please
  consult the documentation for additional details.
  :gl:`#3504`

- Initial support for the PROXYv2 protocol was added. :iscman:`named`
  can now accept PROXYv2 headers over all currently implemented DNS
  transports and :iscman:`dig` can insert these headers into the queries
  it sends. Please consult the related documentation
  (:any:`allow-proxy`, :any:`allow-proxy-on`, :any:`listen-on`, and
  :any:`listen-on-v6` for :iscman:`named`, :option:`dig +proxy` and
  :option:`dig +proxy-plain` for :iscman:`dig`) for additional details.
  :gl:`#4388`

- The client-side support of the EDNS EXPIRE option has been expanded to
  include IXFR and AXFR query types. This enhancement enables
  :iscman:`named` to perform AXFR and IXFR queries while incorporating
  the EDNS EXPIRE option. :gl:`#4170`

- A new configuration option :any:`require-cookie` has been introduced.
  It specifies whether there should be a DNS COOKIE in the response for
  a given prefix; if not, :iscman:`named` falls back to TCP. This is
  useful if it is known that a given server supports DNS COOKIE. It can
  also be used to force all non-DNS COOKIE responses to fall back to
  TCP. :gl:`#2295`

- The :any:`check-svcb` option has been added to control the checking of
  additional constraints on SVCB records. This change affects
  :iscman:`named`, :iscman:`named-checkconf`, :iscman:`named-checkzone`,
  :iscman:`named-compilezone`, and :iscman:`nsupdate`. :gl:`#3576`

- The new :any:`resolver-use-dns64` option enables :iscman:`named` to
  apply :any:`dns64` rules to IPv4 server addresses when sending
  recursive queries, so that resolution can be performed over a NAT64
  connection. :gl:`#608`

- A new option to :any:`dnssec-policy` has been added, :any:`cdnskey`,
  that allows users to enable or disable the publication of CDNSKEY
  records. :gl:`#4050`

- When using :any:`dnssec-policy`, it is now possible to configure the
  digest type to use when CDS records need to be published with
  :any:`cds-digest-types`. Also, publication of specific CDNSKEY/CDS
  records can now be set with :option:`dnssec-signzone -G`. :gl:`#3837`

- Support for multi-signer model 2 (:rfc:`8901`) when using
  :any:`inline-signing` was added. :gl:`#2710`

- HSM support was added to :any:`dnssec-policy`. Keys can now be
  configured with a ``key-store`` that allows users to set the directory
  where key files are stored and to set a PKCS#11 URI string. The latter
  requires OpenSSL 3 and a valid PKCS#11 provider to be configured for
  OpenSSL. :gl:`#1129`

- A new DNSSEC tool :iscman:`dnssec-ksr` has been added to create Key
  Signing Request (KSR) and Signed Key Response (SKR) files. :gl:`#1128`

- :iscman:`dnssec-verify` and :iscman:`dnssec-signzone` now accept a
  ``-J`` option to specify a journal file to read when loading the zone
  to be verified or signed. :gl:`#2486`

- :iscman:`dnssec-keygen` now allows the options :option:`-k
  <dnssec-keygen -k>` and :option:`-f <dnssec-keygen -f>` to be used
  together. This allows the creation of keys for a given
  :any:`dnssec-policy` that match only the KSK (``-fK``) or ZSK (``-fZ``)
  roles. :gl:`#1128`

- The :any:`response-policy` statement was extended with a new argument
  ``ede``. It enables an :rfc:`8914` Extended DNS Error (EDE) code of choice to
  be set for responses which have been modified by a given RPZ. :gl:`#3410`

- A new way of configuring the preferred source address when talking to
  remote servers, such as :any:`primaries` and :any:`parental-agents`,
  has been added: setting the ``source`` and/or ``source-v6`` arguments
  for a given statement is now possible. This new approach is intended
  to eventually replace statements such as :any:`parental-source`,
  :any:`parental-source-v6`, :any:`transfer-source`, etc. :gl:`#3762`

- The new command-line :option:`delv +ns` option activates name server
  mode, to more accurately reproduce the behavior of :iscman:`named`
  when resolving a query. In this mode, :iscman:`delv` uses an internal
  recursive resolver rather than an external server. All messages sent
  and received during the resolution and validation process are logged.
  This can be used in place of :option:`dig +trace`. :gl:`#3842`

- The read timeout in :iscman:`rndc` can now be specified on the command
  line using the :option:`-t <rndc -t>` option, allowing commands that
  take a long time to complete sufficient time to do so. :gl:`#4046`

- The statistics channel now includes information about incoming zone
  transfers that are currently in progress. :gl:`#3883`

- Information on incoming zone transfers in the statistics channel now
  also shows the zones' "first refresh" flag, which indicates that a zone
  is not fully ready and that its first ever refresh is pending or is in
  progress. The number of such zones is now also exposed by the
  :option:`rndc status` command. :gl:`#4241`

- Added a new statistics variable ``recursive high-water`` that reports
  the maximum number of simultaneous recursive clients BIND has handled
  while running. :gl:`#4668`

- A new command, :option:`rndc fetchlimit`, prints a list of name server
  addresses that are currently rate-limited due to
  :any:`fetches-per-server` and domain names that are rate-limited due
  to :any:`fetches-per-zone`. :gl:`#665`

- Queries and responses now emit distinct dnstap entries for DNS-over-TLS
  (DoT) and DNS-over-HTTPS (DoH), and :any:`dnstap-read` understands
  these entries. :gl:`#4523`

- :iscman:`dnstap-read` can now print long timestamps with millisecond
  precision. :gl:`#2360`

- Support for libsystemd's ``sd_notify()`` function was added, enabling
  :iscman:`named` to report its status to the init system. This allows
  systemd to wait until :iscman:`named` is fully ready before starting
  other services that depend on name resolution. :gl:`#1176`

- Support for User Statically Defined Tracing (USDT) probes has been
  added. These probes enable fine-grained application tracing and
  introduce no overhead when they are not enabled. :gl:`#4041`

Removed Features
~~~~~~~~~~~~~~~~

- Support for Red Hat Enterprise Linux version 7 (and clones) has been
  dropped. A C11-compliant compiler is now required to compile BIND 9.
  :gl:`#3729`

- Compiling with `jemalloc`_ versions older than 4.0.0 is no longer
  supported; those versions do not provide the features required by
  current BIND 9 releases. :gl:`#4296`

- The ``auto-dnssec`` configuration statement has been removed. Please
  use :any:`dnssec-policy` or manual signing instead.
  See article `how to migrate <https://kb.isc.org/docs/dnssec-key-and-signing-policy#migrate-to-dnssecpolicy>`_
  from ``auto-dnssec`` to :any:`dnssec-policy`.

  The following
  statements have become obsolete: :any:`dnskey-sig-validity`,
  :any:`dnssec-dnskey-kskonly`, :any:`dnssec-update-mode`,
  :any:`sig-validity-interval`, and :any:`update-check-ksk`.
  :gl:`#3672`

- Dynamic updates that add and remove DNSKEY and NSEC3PARAM records no
  longer trigger key rollovers and denial-of-existence operations. This
  also means that the :any:`dnssec-secure-to-insecure` option has been
  obsoleted. :gl:`#3686`

- The ``glue-cache`` *option* has been removed. The glue cache *feature*
  still works and is now permanently *enabled*. :gl:`#2147`

- Configuring the control channel to use a Unix domain socket has been a
  fatal error since BIND 9.18. The feature has now been completely
  removed and :iscman:`named-checkconf` now reports it as a
  configuration error. :gl:`#4311`

- The statements setting alternate local addresses for inbound zone
  transfers (``alt-transfer-source``, ``alt-transfer-source-v6``, and
  ``use-alt-transfer-source``) have been removed. :gl:`#3714`

- The ``resolver-nonbackoff-tries`` and ``resolver-retry-interval``
  statements have been removed. Using them is now a fatal error.
  :gl:`#4405`

- BIND 9 no longer supports non-zero :any:`stale-answer-client-timeout`
  values, when the feature is turned on. When using a non-zero value,
  :iscman:`named` now generates a warning log message, and treats the
  value as ``0``. :gl:`#4447`

- The Differentiated Services Code Point (DSCP) feature has been
  removed: configuring DSCP values in ``named.conf`` is now a
  configuration error. :gl:`#3789`

- The ``keep-response-order`` option has been declared obsolete and the
  functionality has been removed. :iscman:`named` expects DNS clients to
  be fully compliant with :rfc:`7766`. :gl:`#3140`

- Zone type ``delegation-only``, and the ``delegation-only`` and
  ``root-delegation-only`` statements, have been removed. Using them is
  a configuration error.

  These statements were created to address the SiteFinder controversy,
  in which certain top-level domains redirected misspelled queries to
  other sites instead of returning NXDOMAIN responses. Since top-level
  domains are now DNSSEC-signed, and DNSSEC validation is active by
  default, the statements are no longer needed. :gl:`#3953`

- The ``coresize``, ``datasize``, ``files``, and ``stacksize`` options
  have been removed. The limits these options set should be enforced
  externally, either by manual configuration (e.g. using ``ulimit``) or
  via the process supervisor (e.g. ``systemd``). :gl:`#3676`

- Support for using AES as the DNS COOKIE algorithm (``cookie-algorithm
  aes;``) has been removed. The only supported DNS COOKIE algorithm is
  now the current default, SipHash-2-4. :gl:`#4421`

- The TKEY Mode 2 (Diffie-Hellman Exchanged Keying Mode) has been
  removed and using TKEY Mode 2 is now a fatal error. Users are advised
  to switch to TKEY Mode 3 (GSS-API). :gl:`#3905`

- Special-case code that was originally added to allow GSS-TSIG to work
  around bugs in the Windows 2000 version of Active Directory has now
  been removed, since Windows 2000 is long past end-of-life. The
  :option:`-o <nsupdate -o>` option and the ``oldgsstsig`` command to
  :iscman:`nsupdate` have been deprecated, and are now treated as
  synonyms for :option:`-g <nsupdate -g>` and ``gsstsig`` respectively.
  :gl:`#4012`

- Support for the ``lock-file`` statement and the ``named -X``
  command-line option has been removed. An external process supervisor
  should be used instead. :gl:`#4391`

  Alternatively, the ``flock`` utility (part of util-linux) can be used
  on Linux systems to achieve the same effect as ``lock-file`` or
  ``named -X``:

  ::

    flock -n -x <directory>/named.lock <path>/named <arguments>

- The :iscman:`named` command-line option :option:`-U <named -U>`, which
  specified the number of UDP dispatches, has been removed. Using it now
  returns a warning. :gl:`#1879`

- The ``--with-tuning`` option for ``configure`` has been removed. Each
  of the compile-time settings that required different values based on
  the "workload" (which were previously affected by the value of the
  ``--with-tuning`` option) has either been removed or changed to a
  sensible default. :gl:`#3664`

- The functions that were in the ``libbind9`` shared library have been
  moved to the ``libisc`` and ``libisccfg`` libraries. The now-empty
  ``libbind9`` has been removed and is no longer installed. :gl:`#3903`

- The ``irs_resconf`` module has been moved to the ``libdns`` shared
  library. The now-empty ``libirs`` library has been removed and is no
  longer installed. :gl:`#3904`

.. _`jemalloc`: https://jemalloc.net/

Deprecated Features
~~~~~~~~~~~~~~~~~~~

Features listed in this section still work but are scheduled for eventual
removal.

- The use of the :any:`max-zone-ttl` option in :namedconf:ref:`options`
  and :namedconf:ref:`zone` blocks has been deprecated; it should now be
  configured as part of :any:`dnssec-policy`. A warning is logged if
  this option is used in :namedconf:ref:`options` or :any:`zone` blocks.
  In a future release, it will become nonoperational. :gl:`#2918`

- The :any:`sortlist` option has been deprecated and will be removed in a
  future BIND 9.21.x release. Users should not rely on a specific order
  of resource records in DNS messages.  :gl:`#4593`

- The ``fixed`` value for the :any:`rrset-order` option and the
  corresponding ``configure`` script option have been deprecated and will
  be removed in a future BIND 9.21.x release. Users should not rely on a
  specific order of resource records in DNS messages.  :gl:`#4446`

Feature Changes
~~~~~~~~~~~~~~~

- BIND now depends on `liburcu`_, Userspace RCU, for lock-free data
  structures. :gl:`#3934`

- On Linux, `libcap`_ is now a required dependency to help :iscman:`named`
  keep needed privileges. :gl:`#3583`

- Compiling BIND 9 now requires at least libuv version 1.34.0 or higher.
  libuv should be available on all supported platforms either as a
  native package or as a backport. :gl:`#3567`

- Outgoing zone transfers are no longer enabled by default. An explicit
  :any:`allow-transfer` ACL must now be set at the :any:`zone`,
  :any:`view`, or :namedconf:ref:`options` level to enable outgoing
  transfers. :gl:`#4728`

- DNS zones signed using :any:`dnssec-policy` now automatically detect
  their parent servers, and BIND queries them to check the content of the
  DS RRset. This allows DNSSEC key rollovers to safely and automatically
  proceed when the parent zone is updated with new DNSSEC keys, i.e.
  using the CDS/CDNSKEY mechanism. This behavior is facilitated by the
  new :any:`checkds` feature, which automatically populates
  :any:`parental-agents` by resolving the parent NS records. These parent
  name servers are queried to check the DS RRset during a KSK rollover
  initiated by :any:`dnssec-policy`. :gl:`#3901`

- The responsiveness of :iscman:`named` was improved, when serving as an
  authoritative DNS server for a delegation-heavy zone(s) shortly after
  loading such zone(s). :gl:`#4045`

- To improve query-processing latency under load, the uninterrupted time
  spent on resolving long chains of cached domain names has been
  reduced. :gl:`#4185`

- QNAME minimization is now used when looking up the addresses of name
  servers during the recursive resolution process. :gl:`#4209`

- BIND now returns BADCOOKIE for out-of-date or otherwise bad but
  well-formed DNS server cookies. :gl:`#4194`

- The DNS name compression algorithm used in BIND 9 has been revised: it
  now compresses more thoroughly than before, so responses containing
  names with many labels might have a smaller encoding than before.
  :gl:`#3661`

- Processing large incremental transfers (IXFR) has been offloaded to a
  separate work thread so that it does not prevent networking threads
  from processing regular traffic in the meantime. :gl:`#4367`

- Querying the statistics channel no longer blocks DNS communication on
  the networking event loop level. :gl:`#4680`

- The :any:`inline-signing` zone option is now ignored if there is no
  :any:`dnssec-policy` configured for the zone. This means that unsigned
  zones no longer create redundant signed versions of the zone.
  :gl:`#4349`

- The :any:`inline-signing` statement can now also be set inside
  :any:`dnssec-policy`. The default is to use :any:`inline-signing`.
  This also applies to the built-in policies ``default` and ``insecure``.
  If  :any:`inline-signing` is set at the ``zone`` level, it overrides the
  value set in :any:`dnssec-policy`. :gl:`#3677`

- Due to the change in default value from ``no`` to ``yes``,
  DNSSEC-enabled dynamic zones that do not have :any:`inline-signing`
  explicitly set must now add the option to their configuration with the
  value ``no`` if they do not want their zone also to be inline-signed.

- Following :rfc:`9276` recommendations, :any:`dnssec-policy` now only
  allows an NSEC3 iteration count of 0 for the DNSSEC-signed zones using
  NSEC3 that the policy manages. :gl:`#4363`

- The maximum number of NSEC3 iterations allowed for validation purposes
  has been lowered from 150 to 50. DNSSEC responses containing NSEC3
  records with iteration counts greater than 50 are now treated as
  insecure. :gl:`#4363`

- The ``dnssec-validation yes`` option now requires an explicitly
  configured :any:`trust-anchors` statement. If using manual trust
  anchors is not operationally required, then please consider using
  ``dnssec-validation auto`` instead. :gl:`#4373`

- :iscman:`named-compilezone` no longer performs zone integrity checks
  by default; this allows faster conversion of a zone file from one
  format to another. :gl:`#4364`

  Zone checks can be performed by running :iscman:`named-checkzone`
  separately, or the previous default behavior can be restored by using:

  ::

    named-compilezone -i full -k fail -n fail -r warn -m warn -M warn -S warn -T warn -W warn -C check-svcb:fail

- The red-black tree data structure used in the RBTDB (the default
  database implementation for cache and zone databases), has been
  replaced with QP-tries.  This is expected to improve performance and
  scalability, though in the current implementation large zones require
  roughly 15% more memory than the old red-black tree data structure.

  A side effect of this change is that zone files that are created with
  :any:`masterfile-style` ``relative`` - for example, the output of
  :any:`dnssec-signzone` - will no longer have multiple different
  `$ORIGIN` statements. There should be no other changes to server
  behavior.

  The old RBT-based database still exists for now, and can be used by
  specifying ``database rbt`` in a ``zone`` statement in ``named.conf``,
  or by compiling with ``configure --with-zonedb=rbt
  --with-cachedb=rbt``. :gl:`#4411` :gl:`#4614`

- Multiple RNDC messages are now processed when sent in a single TCP
  message.

  ISC would like to thank Dominik Thalhammer for reporting the issue and
  preparing the initial patch. :gl:`#4416`

- The DNSSEC signing data included in zone statistics identified
  keys only by the key ID; this caused confusion when two keys using
  different algorithms had the same ID. Zone statistics now identify
  keys using the algorithm number, followed by "+", followed by the
  key ID: for example, ``8+54274``. :gl:`#3525`

- The TTL of the NSEC3PARAM record for every NSEC3-signed zone was
  previously set to 0. It is now changed to match the SOA MINIMUM value
  for the given zone. :gl:`#3570`

- On startup, :iscman:`named` now sets the limit on the number of open
  files to the maximum allowed by the operating system, instead of
  trying to set it to "unlimited". :gl:`#3676`

- When an international domain name is not valid according to IDNA2008,
  :iscman:`dig` now tries to convert it according to IDNA2003 rules, or
  pass it through unchanged, instead of stopping with an error message.
  The ``idna2`` utility can be used to check IDNA syntax. :gl:`#3527`

- The memory statistics have been reduced to a single counter,
  ``InUse``; ``Malloced`` is an alias that holds the same value. The
  other counters were usable with the old BIND 9 internal memory
  allocator, but they are unnecessary now that the latter has been
  removed. :gl:`#3718`

- The log message ``resolver priming query complete`` has been moved
  from the INFO log level to the DEBUG(1) log level, to prevent
  :iscman:`delv` from emitting that message when setting up its internal
  resolver. :gl:`#3842`

- Worker threads' event loops are now managed by a new "loop manager"
  API, significantly changing the architecture of the task, timer, and
  networking subsystems for improved performance and code flow.
  :gl:`#3508`

- The code for DNS over TCP and DNS over TLS transports has been
  replaced with a new, unified transport implementation. :gl:`#3374`

.. _`liburcu`: https://liburcu.org/
.. _`libcap`: https://sites.google.com/site/fullycapable/

Bug Fixes
~~~~~~~~~

- When the same :any:`notify-source` address and port number was
  configured for multiple destinations and zones, an unresponsive server
  could tie up the relevant network socket until it timed out; in the
  meantime, NOTIFY messages for other servers silently failed.
  :iscman:`named` will now retry sending such NOTIFY messages over TCP.
  Furthermore, NOTIFY failures are now logged at the INFO level.
  :gl:`#4001` :gl:`#4002`

- DNS compression is no longer applied to the root name (``.``) if it is
  repeatedly used in the same RRset. :gl:`#3423`

- :iscman:`named` could incorrectly return non-truncated, glueless
  referrals for responses whose size was close to the UDP packet size
  limit. This has been fixed. :gl:`#1967`

Known Issues
~~~~~~~~~~~~

- On some platforms, including FreeBSD, :iscman:`named` must be run as
  root to use the :iscman:`rndc` control channel on a privileged port
  (i.e., with a port number less than 1024; this includes the default
  :iscman:`rndc` :rndcconf:ref:`port`, 953). Currently, using the
  :option:`named -u` option to switch to an unprivileged user makes
  :iscman:`rndc` unusable. This will be fixed in a future release; in
  the meantime, ``mac_portacl`` can be used as a workaround, as
  documented in https://kb.isc.org/docs/aa-00621. :gl:`#4793`

- See :ref:`above <relnotes_known_issues>` for a list of all known issues
  affecting this BIND 9 branch.
