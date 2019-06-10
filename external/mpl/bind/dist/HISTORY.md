<!--
 - Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 -
 - This Source Code Form is subject to the terms of the Mozilla Public
 - License, v. 2.0. If a copy of the MPL was not distributed with this
 - file, You can obtain one at http://mozilla.org/MPL/2.0/.
 -
 - See the COPYRIGHT file distributed with this work for additional
 - information regarding copyright ownership.
-->
### Functional enhancements from prior major releases of BIND 9

#### BIND 9.11

BIND 9.11.0 includes a number of changes from BIND 9.10 and earlier
releases.  New features include:

- Added support for Catalog Zones, a new method for provisioning servers: a
  list of zones to be served is stored in a DNS zone, along with their
  configuration parameters. Changes to the catalog zone are propagated to
  slaves via normal AXFR/IXFR, whereupon the zones that are listed in it
  are automatically added, deleted or reconfigured.
- Added support for "dnstap", a fast and flexible method of capturing and
  logging DNS traffic.
- Added support for "dyndb", a new API for loading zone data from an
  external database, developed by Red Hat for the FreeIPA project.
- "fetchlimit" quotas are now compiled in by default.  These are for the
  use of recursive resolvers that are are under high query load for domains
  whose authoritative servers are nonresponsive or are experiencing a
  denial of service attack:
    - "fetches-per-server" limits the number of simultaneous queries that
      can be sent to any single authoritative server.  The configured value
      is a starting point; it is automatically adjusted downward if the
      server is partially or completely non-responsive. The algorithm used
      to adjust the quota can be configured via the "fetch-quota-params"
      option.
    - "fetches-per-zone" limits the number of simultaneous queries that can
      be sent for names within a single domain.  (Note: Unlike
      "fetches-per-server", this value is not self-tuning.)
    - New stats counters have been added to count queries spilled due to
      these quotas.
- Added a new "dnssec-keymgr" key mainenance utility, which can generate or
  update keys as needed to ensure that a zone's keys match a defined DNSSEC
  policy.
- The experimental "SIT" feature in BIND 9.10 has been renamed "COOKIE" and
  is no longer optional. EDNS COOKIE is a mechanism enabling clients to
  detect off-path spoofed responses, and servers to detect spoofed-source
  queries.  Clients that identify themselves using COOKIE options are not
  subject to response rate limiting (RRL) and can receive larger UDP
  responses.
- SERVFAIL responses can now be cached for a limited time (defaulting to 1
  second, with an upper limit of 30).  This can reduce the frequency of
  retries when a query is persistently failing.
- Added an "nsip-wait-recurse" switch to RPZ. This causes NSIP rules to be
  skipped if a name server IP address isn't in the cache yet; the address
  will be looked up and the rule will be applied on future queries.
- Added a Python RNDC module. This allows multiple commands to sent over a
  persistent RNDC channel, which saves time.
- The "controls" block in named.conf can now grant read-only "rndc" access
  to specified clients or keys. Read-only clients could, for example, check
  "rndc status" but could not reconfigure or shut down the server.
- "rndc" commands can now return arbitrarily large amounts of text to the
  caller.
- The zone serial number of a dynamically updatable zone can now be set via
  "rndc signing -serial <number> <zonename>".  This allows inline-signing
  zones to be set to a specific serial number.
- The new "rndc nta" command can be used to set a Negative Trust Anchor
  (NTA), disabling DNSSEC validation for a specific domain; this can be
  used when responses from a domain are known to be failing validation due
  to administrative error rather than because of a spoofing attack.
  Negative trust anchors are strictly temporary; by default they expire
  after one hour, but can be configured to last up to one week.
- "rndc delzone" can now be used on zones that were not originally created
  by "rndc addzone".
- "rndc modzone" reconfigures a single zone, without requiring the entire
  server to be reconfigured.
- "rndc showzone" displays the current configuration of a zone.
- "rndc managed-keys" can be used to check the status of RFC 5001 managed
  trust anchors, or to force trust anchors to be refreshed.
- "max-cache-size" can now be set to a percentage of available memory. The
  default is 90%.
- Update forwarding performance has been improved by allowing a single TCP
  connection to be shared by multiple updates.
- The EDNS Client Subnet (ECS) option is now supported for authoritative
  servers; if a query contains an ECS option then ACLs containing "geoip"
  or "ecs" elements can match against the the address encoded in the
  option.  This can be used to select a view for a query, so that different
  answers can be provided depending on the client network.
- The EDNS EXPIRE option has been implemented on the client side, allowing
  a slave server to set the expiration timer correctly when transferring
  zone data from another slave server.
- The key generation and manipulation tools (dnssec-keygen, dnssec-settime,
  dnssec-importkey, dnssec-keyfromlabel) now take "-Psync" and "-Dsync"
  options to set the publication and deletion times of CDS and CDNSKEY
  parent-synchronization records.  Both named and dnssec-signzone can now
  publish and remove these records at the scheduled times.
- A new "minimal-any" option reduces the size of UDP responses for query
  type ANY by returning a single arbitrarily selected RRset instead of all
  RRsets.
- A new "masterfile-style" zone option controls the formatting of text zone
  files:  When set to "full", a zone file is dumped in
  single-line-per-record format.
- "serial-update-method" can now be set to "date". On update, the serial
  number will be set to the current date in YYYYMMDDNN format.
- "dnssec-signzone -N date" sets the serial number to YYYYMMDDNN.
- "named -L <filename>" causes named to send log messages to the specified
  file by default instead of to the system log.
- "dig +ttlunits" prints TTL values with time-unit suffixes: w, d, h, m, s
  for weeks, days, hours, minutes, and seconds.
- "dig +unknownformat" prints dig output in RFC 3597 "unknown record"
  presentation format.
- "dig +ednsopt" allows dig to set arbitrary EDNS options on requests.
- "dig +ednsflags" allows dig to set yet-to-be-defined EDNS flags on
  requests.
- "mdig" is an alternate version of dig which sends multiple pipelined TCP
  queries to a server.  Instead of waiting for a response after sending a
  query, it sends all queries immediately and displays responses in the
  order received.
- "serial-query-rate" no longer controls NOTIFY messages.  These are
  separately controlled by "notify-rate" and "startup-notify-rate".
- "nsupdate" now performs "check-names" processing by default on records to
  be added.  This can be disabled with "check-names no".
- The statistics channel now supports DEFLATE compression, reducing the
  size of the data sent over the network when querying statistics.
- New counters have been added to the statistics channel to track the sizes
  of incoming queries and outgoing responses in histogram buckets, as
  specified in RSSAC002.
- A new NXDOMAIN redirect method (option "nxdomain-redirect") has been
  added, allowing redirection to a specified DNS namespace instead of a
  single redirect zone.
- When starting up, named now ensures that no other named process is
  already running.
- Files created by named to store information, including "mkeys" and "nzf"
  files, are now named after their corresponding views unless the view name
  contains characters incompatible with use as a filename. Old style
  filenames (based on the hash of the view name) will still work.

#### BIND 9.10.0

BIND 9.10.0 includes a number of changes from BIND 9.9 and earlier
releases.  New features include:

 - DNS Response-rate limiting (DNS RRL), which blunts the
   impact of reflection and amplification attacks, is always
   compiled in and no longer requires a compile-time option
   to enable it.
 - An experimental "Source Identity Token" (SIT) EDNS option
   is now available.  Similar to DNS Cookies as invented by
   Donald Eastlake 3rd, these are designed to enable clients
   to detect off-path spoofed responses, and to enable servers
   to detect spoofed-source queries.  Servers can be configured
   to send smaller responses to clients that have not identified
   themselves using a SIT option, reducing the effectiveness of
   amplification attacks.  RRL processing has also been updated;
   clients proven to be legitimate via SIT are not subject to
   rate limiting.  Use "configure --enable-sit" to enable this
   feature in BIND.
 - A new zone file format, "map", stores zone data in a
   format that can be mapped directly into memory, allowing
   significantly faster zone loading.
 - "delv" (domain entity lookup and validation) is a new tool
   with dig-like semantics for looking up DNS data and performing
   internal DNSSEC validation.  This allows easy validation in
   environments where the resolver may not be trustworthy, and
   assists with troubleshooting of DNSSEC problems. (NOTE:
   In previous development releases of BIND 9.10, this utility
   was called "delve". The spelling has been changed to avoid
   confusion with the "delve" utility included with the Xapian
   search engine.)
 - Improved EDNS(0) processing for better resolver performance
   and reliability over slow or lossy connections.
 - A new "configure --with-tuning=large" option tunes certain
   compiled-in constants and default settings to values better
   suited to large servers with abundant memory.  This can
   improve performance on such servers, but will consume more
   memory and may degrade performance on smaller systems.
 - Substantial improvement in response-policy zone (RPZ)
   performance.  Up to 32 response-policy zones can be
   configured with minimal performance loss.
 - To improve recursive resolver performance, cache records
   which are still being requested by clients can now be
   automatically refreshed from the authoritative server
   before they expire, reducing or eliminating the time
   window in which no answer is available in the cache.
 - New "rpz-client-ip" triggers and drop policies allowing
   response policies based on the IP address of the client.
 - ACLs can now be specified based on geographic location
   using the MaxMind GeoIP databases.  Use "configure
   --with-geoip" to enable.
 - Zone data can now be shared between views, allowing
   multiple views to serve the same zones authoritatively
   without storing multiple copies in memory.
 - New XML schema (version 3) for the statistics channel
   includes many new statistics and uses a flattened XML tree
   for faster parsing. The older schema is now deprecated.
 - A new stylesheet, based on the Google Charts API, displays
   XML statistics in charts and graphs on javascript-enabled
   browsers.
 - The statistics channel can now provide data in JSON
   format as well as XML.
 - New stats counters track TCP and UDP queries received
   per zone, and EDNS options received in total.
 - The internal and export versions of the BIND libraries
   (libisc, libdns, etc) have been unified so that external
   library clients can use the same libraries as BIND itself.
 - A new compile-time option, "configure --enable-native-pkcs11",
   allows BIND 9 cryptography functions to use the PKCS#11 API
   natively, so that BIND can drive a cryptographic hardware
   service module (HSM) directly instead of using a modified
   OpenSSL as an intermediary. (Note: This feature requires an
   HSM to have a full implementation of the PKCS#11 API; many
   current HSMs only have partial implementations. The new
   "pkcs11-tokens" command can be used to check API completeness.
   Native PKCS#11 is known to work with the Thales nShield HSM
   and with SoftHSM version 2 from the Open DNSSEC project.)
 - The new "max-zone-ttl" option enforces maximum TTLs for
   zones. This can simplify the process of rolling DNSSEC keys
   by guaranteeing that cached signatures will have expired
   within the specified amount of time.
 - "dig +subnet" sends an EDNS CLIENT-SUBNET option when
   querying.
 - "dig +expire" sends an EDNS EXPIRE option when querying.
   When this option is sent with an SOA query to a server
   that supports it, it will report the expiry time of
   a slave zone.
 - New "dnssec-coverage" tool to check DNSSEC key coverage
   for a zone and report if a lapse in signing coverage has
   been inadvertently scheduled.
 - Signing algorithm flexibility and other improvements
   for the "rndc" control channel.
 - "named-checkzone" and "named-compilezone" can now read
   journal files, allowing them to process dynamic zones.
 - Multiple DLZ databases can now be configured.  Individual
   zones can be configured to be served from a specific DLZ
   database.  DLZ databases now serve zones of type "master"
   and "redirect".
 - "rndc zonestatus" reports information about a specified zone.
 - "named" now listens on IPv6 as well as IPv4 interfaces
   by default.
 - "named" now preserves the capitalization of names
   when responding to queries: for instance, a query for
   "example.com" may be answered with "example.COM" if the
   name was configured that way in the zone file.  Some
   clients have a bug causing them to depend on the older
   behavior, in which the case of the answer always matched
   the case of the query, rather than the case of the name
   configured in the DNS.  Such clients can now be specified
   in the new "no-case-compress" ACL; this will restore the
   older behavior of "named" for those clients only.
 - new "dnssec-importkey" command allows the use of offline
   DNSSEC keys with automatic DNSKEY management.
 - New "named-rrchecker" tool to verify the syntactic
   correctness of individual resource records.
 - When re-signing a zone, the new "dnssec-signzone -Q" option
   drops signatures from keys that are still published but are
   no longer active.
 - "named-checkconf -px" will print the contents of configuration
   files with the shared secrets obscured, making it easier to
   share configuration (e.g. when submitting a bug report)
   without revealing private information.
 - "rndc scan" causes named to re-scan network interfaces for
   changes in local addresses.
 - On operating systems with support for routing sockets,
   network interfaces are re-scanned automatically whenever
   they change.
 - "tsig-keygen" is now available as an alternate command
   name to use for "ddns-confgen".

#### BIND 9.9.0

BIND 9.9.0 includes a number of changes from BIND 9.8 and earlier
releases.  New features include:

- Inline signing, allowing automatic DNSSEC signing of
  master zones without modification of the zonefile, or
  "bump in the wire" signing in slaves.
- NXDOMAIN redirection.
- New 'rndc flushtree' command clears all data under a given
  name from the DNS cache.
- New 'rndc sync' command dumps pending changes in a dynamic
  zone to disk without a freeze/thaw cycle.
- New 'rndc signing' command displays or clears signing status
  records in 'auto-dnssec' zones.
- NSEC3 parameters for 'auto-dnssec' zones can now be set prior
  to signing, eliminating the need to initially sign with NSEC.
- Startup time improvements on large authoritative servers.
- Slave zones are now saved in raw format by default.
- Several improvements to response policy zones (RPZ).
- Improved hardware scalability by using multiple threads
  to listen for queries and using finer-grained client locking
- The 'also-notify' option now takes the same syntax as
  'masters', so it can used named masterlists and TSIG keys.
- 'dnssec-signzone -D' writes an output file containing only DNSSEC
  data, which can be included by the primary zone file.
- 'dnssec-signzone -R' forces removal of signatures that are
  not expired but were created by a key which no longer exists.
- 'dnssec-signzone -X' allows a separate expiration date to
  be specified for DNSKEY signatures from other signatures.
- New '-L' option to dnssec-keygen, dnssec-settime, and
  dnssec-keyfromlabel sets the default TTL for the key.
- dnssec-dsfromkey now supports reading from standard input,
  to make it easier to convert DNSKEY to DS.
- RFC 1918 reverse zones have been added to the empty-zones
  table per RFC 6303.
- Dynamic updates can now optionally set the zone's SOA serial
  number to the current UNIX time.
- DLZ modules can now retrieve the source IP address of
  the querying client.
- 'request-ixfr' option can now be set at the per-zone level.
- 'dig +rrcomments' turns on comments about DNSKEY records,
  indicating their key ID, algorithm and function
- Simplified nsupdate syntax and added readline support

#### BIND 9.8.0

BIND 9.8.0 includes a number of changes from BIND 9.7 and earlier
releases.  New features include:

- Built-in trust anchor for the root zone, which can be
  switched on via "dnssec-validation auto;"
- Support for DNS64.
- Support for response policy zones (RPZ).
- Support for writable DLZ zones.
- Improved ease of configuration of GSS/TSIG for
  interoperability with Active Directory
- Support for GOST signing algorithm for DNSSEC.
- Removed RTT Banding from server selection algorithm.
- New "static-stub" zone type.
- Allow configuration of resolver timeouts via
  "resolver-query-timeout" option.
- The DLZ "dlopen" driver is now built by default.
- Added a new include file with function typedefs
  for the DLZ "dlopen" driver.
- Made "--with-gssapi" default.
- More verbose error reporting from DLZ LDAP.

#### BIND 9.7.0

BIND 9.7.0 includes a number of changes from BIND 9.6 and earlier
releases.  Most are intended to simplify DNSSEC configuration.
New features include:

- Fully automatic signing of zones by "named".
- Simplified configuration of DNSSEC Lookaside Validation (DLV).
- Simplified configuration of Dynamic DNS, using the "ddns-confgen"
  command line tool or the "local" update-policy option.  (As a side
  effect, this also makes it easier to configure automatic zone
  re-signing.)
- New named option "attach-cache" that allows multiple views to
  share a single cache.
- DNS rebinding attack prevention.
- New default values for dnssec-keygen parameters.
- Support for RFC 5011 automated trust anchor maintenance
- Smart signing: simplified tools for zone signing and key
  maintenance.
- The "statistics-channels" option is now available on Windows.
- A new DNSSEC-aware libdns API for use by non-BIND9 applications
- On some platforms, named and other binaries can now print out
  a stack backtrace on assertion failure, to aid in debugging.
- A "tools only" installation mode on Windows, which only installs
  dig, host, nslookup and nsupdate.
- Improved PKCS#11 support, including Keyper support and explicit
  OpenSSL engine selection.

#### BIND 9.6.0

- Full NSEC3 support
- Automatic zone re-signing
- New update-policy methods tcp-self and 6to4-self
- The BIND 8 resolver library, libbind, has been removed from the BIND 9
  distribution and is now available as a separate download.
- Change the default pid file location from /var/run to
  /var/run/{named,lwresd} for improved chroot/setuid support.

#### BIND 9.5.0

- GSS-TSIG support (RFC 3645).
- DHCID support.
- Experimental http server and statistics support for named via xml.
- More detailed statistics counters including those supported in BIND 8.
- Faster ACL processing.
- Use Doxygen to generate internal documentation.
- Efficient LRU cache-cleaning mechanism.
- NSID support.

BIND 9.4.0

- Implemented "additional section caching (or acache)", an internal cache
  framework for additional section content to improve response performance.
  Several configuration options were provided to control the behavior.
- New notify type 'master-only'.  Enable notify for master zones only.
- Accept 'notify-source' style syntax for query-source.
- rndc now allows addresses to be set in the server clauses.
- New option "allow-query-cache".  This lets "allow-query" be used to
  specify the default zone access level rather than having to have every
  zone override the global value.  "allow-query-cache" can be set at both
  the options and view levels.  If "allow-query-cache" is not set then
  "allow-recursion" is used if set, otherwise "allow-query" is used if set
  unless "recursion no;" is set in which case "none;" is used, otherwise
  the default (localhost; localnets;) is used.
- rndc: the source address can now be specified.
- ixfr-from-differences now takes master and slave in addition to yes and
  no at the options and view levels.
- Allow the journal's name to be changed via named.conf.
- 'rndc notify zone [class [view]]' resend the NOTIFY messages for the
  specified zone.
- 'dig +trace' now randomly selects the next servers to try.  Report if
  there is a bad delegation.
- Improve check-names error messages.
- Make public the function to read a key file, dst_key_read_public().
- dig now returns the byte count for axfr/ixfr.
- allow-update is now settable at the options / view level.
- named-checkconf now checks the logging configuration.
- host now can turn on memory debugging flags with '-m'.
- Don't send notify messages to self.
- Perform sanity checks on NS records which refer to 'in zone' names.
- New zone option "notify-delay".  Specify a minimum delay between sets of
  NOTIFY messages.
- Extend adjusting TTL warning messages.
- Named and named-checkzone can now both check for non-terminal wildcard
  records.
- "rndc freeze/thaw" now freezes/thaws all zones.
- named-checkconf now check acls to verify that they only refer to existing
  acls.
- The server syntax has been extended to support a range of servers.
- Report differences between hints and real NS rrset and associated address
  records.
- Preserve the case of domain names in rdata during zone transfers.
- Restructured the data locking framework using architecture dependent
  atomic operations (when available), improving response performance on
  multi-processor machines significantly.  x86, x86_64, alpha, powerpc, and
  mips are currently supported.
- UNIX domain controls are now supported.
- Add support for additional zone file formats for improving loading
  performance.  The masterfile-format option in named.conf can be used to
  specify a non-default format.  A separate command named-compilezone was
  provided to generate zone files in the new format.  Additionally, the -I
  and -O options for dnssec-signzone specify the input and output formats.
- dnssec-signzone can now randomize signature end times (dnssec-signzone -j
  jitter).
- Add support for CH A record.
- Add additional zone data constancy checks.  named-checkzone has extended
  checking of NS, MX and SRV record and the hosts they reference.  named
  has extended post zone load checks.  New zone options: check-mx and
  integrity-check.
- edns-udp-size can now be overridden on a per server basis.
- dig can now specify the EDNS version when making a query.
- Added framework for handling multiple EDNS versions.
- Additional memory debugging support to track size and mctx arguments.
- Detect duplicates of UDP queries we are recursing on and drop them.  New
  stats category "duplicates".
- "USE INTERNAL MALLOC" is now runtime selectable.
- The lame cache is now done on a <qname,qclass,qtype> basis as some
  servers only appear to be lame for certain query types.
- Limit the number of recursive clients that can be waiting for a single
  query (<qname,qtype,qclass>) to resolve.  New options clients-per-query
  and max-clients-per-query.
- dig: report the number of extra bytes still left in the packet after
  processing all the records.
- Support for IPSECKEY rdata type.
- Raise the UDP recieve buffer size to 32k if it is less than 32k.
- x86 and x86_64 now have seperate atomic locking implementations.
- named-checkconf now validates update-policy entries.
- Attempt to make the amount of work performed in a iteration self tuning.
  The covers nodes clean from the cache per iteration, nodes written to
  disk when rewriting a master file and nodes destroyed per iteration when
  destroying a zone or a cache.
- ISC string copy API.
- Automatic empty zone creation for D.F.IP6.ARPA and friends.  Note: RFC
  1918 zones are not yet covered by this but are likely to be in a future
  release.
- New options: empty-server, empty-contact, empty-zones-enable and
  disable-empty-zone.
- dig now has a '-q queryname' and '+showsearch' options.
- host/nslookup now continue (default)/fail on SERVFAIL.
- dig now warns if 'RA' is not set in the answer when 'RD' was set in the
  query.  host/nslookup skip servers that fail to set 'RA' when 'RD' is set
  unless a server is explicitly set.
- Integrate contibuted DLZ code into named.
- Integrate contibuted IDN code from JPNIC.
- libbind: corresponds to that from BIND 8.4.7.

#### BIND 9.3.0

- DNSSEC is now DS based (RFC 3658).
- DNSSEC lookaside validation.
- check-names is now implemented.
- rrset-order is more complete.
- IPv4/IPv6 transition support, dual-stack-servers.
- IXFR deltas can now be generated when loading master files,
  ixfr-from-differences.
- It is now possible to specify the size of a journal, max-journal-size.
- It is now possible to define a named set of master servers to be used in
  masters clause, masters.
- The advertised EDNS UDP size can now be set, edns-udp-size.
- allow-v6-synthesis has been obsoleted.
- Zones containing MD and MF will now be rejected.
- dig, nslookup name. now report "Not Implemented" as NOTIMP rather than
  NOTIMPL.  This will have impact on scripts that are looking for NOTIMPL.
- libbind: corresponds to that from BIND 8.4.5.

#### BIND 9.2.0

- The size of the cache can now be limited using the "max-cache-size"
  option.
- The server can now automatically convert RFC1886-style recursive lookup
  requests into RFC2874-style lookups, when enabled using the new option
  "allow-v6-synthesis".  This allows stub resolvers that support AAAA
  records but not A6 record chains or binary labels to perform lookups in
  domains that make use of these IPv6 DNS features.
- Performance has been improved.
- The man pages now use the more portable "man" macros rather than the
  "mandoc" macros, and are installed by "make install".
- The named.conf parser has been completely rewritten.  It now supports
  "include" directives in more places such as inside "view" statements, and
  it no longer has any reserved words.
- The "rndc status" command is now implemented.
- rndc can now be configured automatically.
- A BIND 8 compatible stub resolver library is now included in lib/bind.
- OpenSSL has been removed from the distribution.  This means that to use
  DNSSEC, OpenSSL must be installed and the --with-openssl option must be
  supplied to configure.  This does not apply to the use of TSIG, which
  does not require OpenSSL.
- The source distribution now builds on Windows.  See
  win32utils/readme1.txt and win32utils/win32-build.txt for details.
- This distribution also includes a new lightweight stub resolver library
  and associated resolver daemon that fully support forward and reverse
  lookups of both IPv4 and IPv6 addresses.  This library is considered
  experimental and is not a complete replacement for the BIND 8 resolver
  library.  Applications that use the BIND 8 `res_*` functions to perform
  DNS lookups or dynamic updates still need to be linked against the BIND 8
  libraries.  For DNS lookups, they can also use the new "getrrsetbyname()"
  API.
- BIND 9.2 is capable of acting as an authoritative server for DNSSEC
  secured zones.  This functionality is believed to be stable and complete
  except for lacking support for verifications involving wildcard records
  in secure zones.
- When acting as a caching server, BIND 9.2 can be configured to perform
  DNSSEC secure resolution on behalf of its clients.  This part of the
  DNSSEC implementation is still considered experimental.  For detailed
  information about the state of the DNSSEC implementation, see the file
  doc/misc/dnssec.
