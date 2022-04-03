/*	$NetBSD: site.h,v 1.3 2022/04/03 01:10:58 christos Exp $	*/

/* Site-specific definitions.

   For supported systems, you shouldn't need to make any changes here.
   However, you may want to, in order to deal with site-specific
   differences. */

/* Add any site-specific definitions and inclusions here... */

/* #include <site-foo-bar.h> */
/* #define SITE_FOOBAR */

/* Define this if you don't want dhcpd to run as a daemon and do want
   to see all its output printed to stdout instead of being logged via
   syslog().   This also makes dhcpd use the dhcpd.conf in its working
   directory and write the dhcpd.leases file there. */

/* #define DEBUG */

/* Define this to see what the parser is parsing.   You probably don't
   want to see this. */

/* #define DEBUG_TOKENS */

/* Define this to see dumps of incoming and outgoing packets.    This
   slows things down quite a bit... */

/* #define DEBUG_PACKET */

/* Define this if you want to see dumps of expression evaluation. */

/* #define DEBUG_EXPRESSIONS */

/* Define this if you want to see dumps of find_lease() in action. */

/* #define DEBUG_FIND_LEASE */

/* Define this if you want to see dumps of parsed expressions. */

/* #define DEBUG_EXPRESSION_PARSE */

/* Define this if you want to watch the class matching process. */

/* #define DEBUG_CLASS_MATCHING */

/* Define this if you want to track memory usage for the purpose of
   noticing memory leaks quickly. */

/* #define DEBUG_MEMORY_LEAKAGE */
/* #define DEBUG_MEMORY_LEAKAGE_ON_EXIT */

/* Define this if you want exhaustive (and very slow) checking of the
   malloc pool for corruption. */

/* #define DEBUG_MALLOC_POOL */

/* Define this if you want to see a message every time a lease's state
   changes. */
/* #define DEBUG_LEASE_STATE_TRANSITIONS */

/* Define this if you want to maintain a history of the last N operations
   that changed reference counts on objects.   This can be used to debug
   cases where an object is dereferenced too often, or not often enough. */

/* #define DEBUG_RC_HISTORY */

/* Define this if you want to see the history every cycle. */

/* #define DEBUG_RC_HISTORY_EXHAUSTIVELY */

/* This is the number of history entries to maintain - by default, 256. */

/* #define RC_HISTORY_MAX 10240 */

/* Define this if you want dhcpd to dump core when a non-fatal memory
   allocation error is detected (i.e., something that would cause a
   memory leak rather than a memory smash). */

/* #define POINTER_DEBUG */

/* Define this if you want debugging output for DHCP failover protocol
   messages. */

/* #define DEBUG_FAILOVER_MESSAGES */

/* Define this to include contact messages in failover message debugging.
   The contact messages are sent once per second, so this can generate a
   lot of log entries. */

/* #define DEBUG_FAILOVER_CONTACT_MESSAGES */

/* Define this if you want debugging output for DHCP failover protocol
   event timeout timing. */

/* #define DEBUG_FAILOVER_TIMING */

/* Define this if you want to include contact message timing, which is
   performed once per second and can generate a lot of log entries. */

/* #define DEBUG_FAILOVER_CONTACT_TIMING */

/* Define this if you want all leases written to the lease file, even if
   they are free leases that have never been used. */

/* #define DEBUG_DUMP_ALL_LEASES */

/* Define this if you want to see the requests and replies between the
   DHCP code and the DNS library code. */
/* #define DEBUG_DNS_UPDATES */

/* Define this if you want to debug the host part of the inform processing */
/* #define DEBUG_INFORM_HOST */

/* Define this if you want to debug the binary leases (lease_chain) code */
/* #define DEBUG_BINARY_LEASES */

/* Define this if you want to debug checksum calculations */
/* #define DEBUG_CHECKSUM */

/* Define this if you want to verbosely debug checksum calculations */
/* #define DEBUG_CHECKSUM_VERBOSE */


/* Define this if you want DHCP failover protocol support in the DHCP
   server. */

/* #define FAILOVER_PROTOCOL */

/* Define this if you want DNS update functionality to be available. */

#define NSUPDATE

/* Define this if you want to enable the DHCP server attempting to
   find a nameserver to use for DDNS updates. */
#define DNS_ZONE_LOOKUP

/* Define this if you want the dhcpd.pid file to go somewhere other than
   the default (which varies from system to system, but is usually either
   /etc or /var/run. */

/* #define _PATH_DHCPD_PID	"/var/run/dhcpd.pid" */

/* Define this if you want the dhcpd.leases file (the dynamic lease database)
   to go somewhere other than the default location, which is normally
   /etc/dhcpd.leases. */

/* #define _PATH_DHCPD_DB	"/etc/dhcpd.leases" */

/* Define this if you want the dhcpd.conf file to go somewhere other than
   the default location.   By default, it goes in /etc/dhcpd.conf. */

/* #define _PATH_DHCPD_CONF	"/etc/dhcpd.conf" */

/* Network API definitions.   You do not need to choose one of these - if
   you don't choose, one will be chosen for you in your system's config
   header.    DON'T MESS WITH THIS UNLESS YOU KNOW WHAT YOU'RE DOING!!! */

/* Define USE_SOCKETS to use the standard BSD socket API.

   On many systems, the BSD socket API does not provide the ability to
   send packets to the 255.255.255.255 broadcast address, which can
   prevent some clients (e.g., Win95) from seeing replies.   This is
   not a problem on Solaris.

   In addition, the BSD socket API will not work when more than one
   network interface is configured on the server.

   However, the BSD socket API is about as efficient as you can get, so if
   the aforementioned problems do not matter to you, or if no other
   API is supported for your system, you may want to go with it. */

/* #define USE_SOCKETS */

/* Define this to use the Sun Streams NIT API.

   The Sun Streams NIT API is only supported on SunOS 4.x releases. */

/* #define USE_NIT */

/* Define this to use the Berkeley Packet Filter API.

   The BPF API is available on all 4.4-BSD derivatives, including
   NetBSD, FreeBSD and BSDI's BSD/OS.   It's also available on
   DEC Alpha OSF/1 in a compatibility mode supported by the Alpha OSF/1
   packetfilter interface. */

/* #define USE_BPF */

/* Define this to use the raw socket API.

   The raw socket API is provided on many BSD derivatives, and provides
   a way to send out raw IP packets.   It is only supported for sending
   packets - packets must be received with the regular socket API.
   This code is experimental - I've never gotten it to actually transmit
   a packet to the 255.255.255.255 broadcast address - so use it at your
   own risk. */

/* #define USE_RAW_SOCKETS */

/* Define this to keep the old program name (e.g., "dhcpd" for
   the DHCP server) in place of the (base) name the program was
   invoked with. */

/* #define OLD_LOG_NAME */

/* Define this to change the logging facility used by dhcpd. */

/* #define DHCPD_LOG_FACILITY LOG_DAEMON */


/* Define this if you want to be able to execute external commands
   during conditional evaluation. */

/* #define ENABLE_EXECUTE */

/* Define this if you aren't debugging and you want to save memory
   (potentially a _lot_ of memory) by allocating leases in chunks rather
   than one at a time. */

#define COMPACT_LEASES

/* Define this if you want to be able to save and playback server operational
   traces. */

/* #define TRACING */

/* Define this if you want the server to use the previous behavior
   when determining the DDNS TTL.  If the user has specified a ddns-ttl
   option that is used to detemine the ttl.  (If the user specifies
   an option that references the lease structure it is only usable
   for v4.  In that case v6 will use the default.) Otherwise when
   defined the defaults are: v4 - 1/2 the lease time,
   v6 - DEFAULT_DDNS_TTL.  When undefined the defaults are 1/2 the
   (preferred) lease time for both but with a cap on the maximum. */

/* #define USE_OLD_DDNS_TTL */

/* Define this if you want a DHCPv6 server to send replies to the
   source port of the message it received.  This is useful for testing
   but is only included for backwards compatibility. */
/* #define REPLY_TO_SOURCE_PORT */

/* Define this if you want to enable strict checks in DNS Updates mechanism.
   Do not enable this unless are DHCP developer. */
/* #define DNS_UPDATES_MEMORY_CHECKS */

/* Define this if you want to allow domain list in domain-name option.
   RFC2132 does not allow that behavior, but it is somewhat used due
   to historic reasons. Note that it may be removed some time in the
   future. */

#define ACCEPT_LIST_IN_DOMAIN_NAME

/* In previous versions of the code when the server generates a NAK
   it doesn't attempt to determine if the configuration included a
   server ID for that client.  Defining this option causes the server
   to make a modest effort to determine the server id when building
   a NAK as a response.  This effort will only check the first subnet
   and pool associated with a shared subnet and will not check for
   host declarations.  With some configurations the server id
   computed for a NAK may not match that computed for an ACK. */

#define SERVER_ID_FOR_NAK

/* NOTE:  SERVER_ID_CHECK switch has been removed. Enabling server id
 * checking is now done via the server-id-check statement. Please refer
 * to the dhcpd manpage (server/dhcpd.conf.5) */

/* Include code to do a slow transition of DDNS records
   from the interim to the standard version, or backwards.
   The normal code will handle removing an old style record
   when the name on a lease is being changed.  This adds code
   to handle the case where the name isn't being changed but
   the old record should be removed to allow a new record to
   be added.  This is the slow transition as leases are only
   updated as a client touches them.  A fast transition would
   entail updating all the records at once, probably at start
   up. */
#define DDNS_UPDATE_SLOW_TRANSITION

/* Define the default prefix length passed from the client to
   the script when modifying an IPv6 IA_NA or IA_TA address.
   The two most useful values are 128 which is what the current
   specifications call for or 64 which is what has been used in
   the past.  For most OSes 128 will indicate that the address
   is a host address and doesn't include any on-link information.
   64 indicates that the first 64 bits are the subnet or on-link
   prefix. */
#define DHCLIENT_DEFAULT_PREFIX_LEN 128

/* Enable the gentle shutdown signal handling.  Currently this
   means that on SIGINT or SIGTERM a client will release its
   address and a server in a failover pair will go through
   partner down.  Both of which can be undesireable in some
   situations.  We plan to revisit this feature and may
   make non-backwards compatible changes including the
   removal of this define.  Use at your own risk.  */
/* #define ENABLE_GENTLE_SHUTDOWN */

/* Include old error codes.  This is provided in case you
   are building an external program similar to omshell for
   which you need the ISC_R_* error codes.  You should switch
   to DHCP_R_* error codes for those that have been defined
   (see includes/omapip/result.h).  The extra defines and
   this option will be removed at some time. */
/* #define INCLUDE_OLD_DHCP_ISC_ERROR_CODES */

/* Use the older factors for scoring a lease in the v6 client code.
   The new factors cause the client to choose more bindings (IAs)
   over more addresse within a binding.  Most uses will get a
   single address in a single binding and only get an adverstise
   from a single server and there won't be a difference. */
/* #define USE_ORIGINAL_CLIENT_LEASE_WEIGHTS */

/* Print out specific error messages for dhclient, dhcpd
   or dhcrelay when processing an incorrect command line.  This
   is included for those that might require the exact error
   messages, as we don't expect that is necessary it is on by
   default. */
#define PRINT_SPECIFIC_CL_ERRORS

/* Limit the value of a file descriptor the serve will use
   when accepting a connecting request.  This can be used to
   limit the number of TCP connections that the server will
   allow at one time.  A value of 0 means there is no limit.*/
#define MAX_FD_VALUE 200

/* Enable EUI-64 Address assignment policy.  Instructs the server
 * to use EUI-64 addressing instead of dynamic address allocation
 * for IA_NA pools, if the parameter use-eui-64 is true for the
 * pool.  Can be at all scopes down to the pool level.  Not
 * supported by the configure script. */
/* #define EUI_64 */

/* Enable enforcement of the require option statement as documented
 * in man page.  Instructs the dhclient, when in -6 mode, to discard
 * offered leases that do not contain all options specified as required
 * in the client's configuration file. The client already enforces this
 * in -4 mode. */
#define ENFORCE_DHCPV6_CLIENT_REQUIRE

/* Enable the invocation of the client script with a FAIL state code
 * by dhclient when running in one-try mode (-T) and the attempt to
 * obtain the desired lease(s) fails. Applies to IPv4 mode only. */
/* #define CALL_SCRIPT_ON_ONETRY_FAIL */

/* Include definitions for various options.  In general these
   should be left as is, but if you have already defined one
   of these and prefer your definition you can comment the
   RFC define out to avoid conflicts */
#define RFC2563_OPTIONS
#define RFC2937_OPTIONS
#define RFC4776_OPTIONS
#define RFC4578_OPTIONS
#define RFC4833_OPTIONS
#define RFC4994_OPTIONS
#define RFC5071_OPTIONS
#define RFC5192_OPTIONS
#define RFC5223_OPTIONS
#define RFC5417_OPTIONS
#define RFC5460_OPTIONS
#define RFC5859_OPTIONS
#define RFC5969_OPTIONS
#define RFC5970_OPTIONS
#define RFC5986_OPTIONS
#define RFC6011_OPTIONS
#define RFC6011_OPTIONS
#define RFC6153_OPTIONS
#define RFC6334_OPTIONS
#define RFC6440_OPTIONS
#define RFC6731_OPTIONS
#define RFC6939_OPTIONS
#define RFC6977_OPTIONS
#define RFC7083_OPTIONS
#define RFC7341_OPTIONS
#define RFC7618_OPTIONS
#define RFC7710_OPTIONS
#define RFC8925_OPTIONS
