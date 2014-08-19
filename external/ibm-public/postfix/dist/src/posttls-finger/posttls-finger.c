/*	$NetBSD: posttls-finger.c,v 1.1.1.1.6.2 2014/08/19 23:59:44 tls Exp $	*/

/*++
/* NAME
/*	posttls-finger 1
/* SUMMARY
/*	Probe the TLS properties of an ESMTP or LMTP server.
/* SYNOPSIS
/*	\fBposttls-finger\fR [\fIoptions\fR] [\fBinet:\fR]\fIdomain\fR[:\fIport\fR] [\fImatch ...\fR]
/* .br
/*	\fBposttls-finger\fR -S [\fIoptions\fR] \fBunix:\fIpathname\fR [\fImatch ...\fR]
/* DESCRIPTION
/*	\fBposttls-finger\fR(1) connects to the specified destination
/*	and reports TLS-related information about the server. With SMTP, the
/*	destination is a domainname; with LMTP it is either a domainname
/*	prefixed with \fBinet:\fR or a pathname prefixed with \fBunix:\fR.  If
/*	Postfix is built without TLS support, the resulting posttls-finger
/*	program has very limited functionality, and only the \fB-a\fR, \fB-c\fR,
/*	\fB-h\fR, \fB-o\fR, \fB-S\fR, \fB-t\fR, \fB-T\fR and \fB-v\fR options
/*	are available.
/*
/*	Note: this is an unsupported test program. No attempt is made
/*	to maintain compatibility between successive versions.
/*
/*	For SMTP servers that don't support ESMTP, only the greeting banner
/*	and the negative EHLO response are reported. Otherwise, the reported
/*	EHLO response details further server capabilities.
/*
/*	If TLS support is enabled when \fBposttls-finger\fR(1) is compiled, and
/*	the server supports \fBSTARTTLS\fR, a TLS handshake is attempted.
/*
/*	If DNSSEC support is available, the connection TLS security level
/*	(\fB-l\fR option) defaults to \fBdane\fR; see TLS_README for
/*	details. Otherwise, it defaults to \fBsecure\fR.  This setting
/*	determines the certificate matching policy.
/*
/*	If TLS negotiation succeeds, the TLS protocol and cipher details are
/*	reported. The server certificate is then verified in accordance with
/*	the policy at the chosen (or default) security level.  With public
/*	CA-based trust, when the \fB-L\fR option includes \fBcertmatch\fR,
/*	(true by default) name matching is performed even if the certificate
/*	chain is not trusted.  This logs the names found in the remote SMTP
/*	server certificate and which if any would match, were the certificate
/*	chain trusted.
/*
/*	Note: \fBposttls-finger\fR(1) does not perform any table lookups, so
/*	the TLS policy table and obsolete per-site tables are not consulted.
/*	It does not communicate with the \fBtlsmgr\fR(8) daemon (or any other
/*	Postfix daemons); its TLS session cache is held in private memory, and
/*	disappears when the process exits.
/*
/*	With the \fB-r \fIdelay\fR option, if the server assigns a TLS
/*	session id, the TLS session is cached. The connection is then closed
/*	and re-opened after the specified delay, and \fBposttls-finger\fR(1)
/*	then reports whether the cached TLS session was re-used.
/*
/*	When the destination is a load-balancer, it may be distributing
/*	load between multiple server caches. Typically, each server returns
/*	its unique name in its EHLO response. If, upon reconnecting with
/*	\fB-r\fR, a new server name is detected, another session is cached
/*	for the new server, and the reconnect is repeated up to a maximum
/*	number of times (default 5) that can be specified via the \fB-m\fR
/*	option.
/*
/*	The choice of SMTP or LMTP (\fB-S\fR option) determines the syntax of
/*	the destination argument. With SMTP, one can specify a service on a
/*	non-default port as \fIhost\fR:\fIservice\fR, and disable MX (mail
/*	exchanger) DNS lookups with [\fIhost\fR] or [\fIhost\fR]:\fIport\fR.
/*	The [] form is required when you specify an IP address instead of a
/*	hostname.  An IPv6 address takes the form [\fBipv6:\fIaddress\fR].
/*	The default port for SMTP is taken from the \fBsmtp/tcp\fR entry in
/*	/etc/services, defaulting to 25 if the entry is not found.
/*
/*	With LMTP, specify \fBunix:\fIpathname\fR to connect to a local server
/*	listening on a unix-domain socket bound to the specified pathname;
/*	otherwise, specify an optional \fBinet:\fR prefix followed by a
/*	\fIdomain\fR and an optional port, with the same syntax as for
/*	SMTP. The default TCP port for LMTP is 24.
/*
/*	Arguments:
/* .IP "\fB-a\fR \fIfamily\fR (default: \fBany\fR)"
/*	Address family preference: \fBipv4\fR, \fBipv6\fR or \fBany\fR.  When
/*	using \fBany\fR, posttls-finger will randomly select one of the two as
/*	the more preferred, and exhaust all MX preferences for the first
/*	address family before trying any addresses for the other.
/* .IP "\fB-A\fR \fItrust-anchor.pem\fR (default: none)"
/*	A list of PEM trust-anchor files that overrides CAfile and CApath
/*	trust chain verification.  Specify the option multiple times to
/*	specify multiple files.  See the main.cf documentation for
/*	smtp_tls_trust_anchor_file for details.
/* .IP "\fB-c\fR"
/*	Disable SMTP chat logging; only TLS-related information is logged.
/* .IP "\fB-C\fR"
/*	Print the remote SMTP server certificate trust chain in PEM format.
/*	The issuer DN, subject DN, certificate and public key fingerprints
/*	(see \fB-d \fImdalg\fR option below) are printed above each PEM
/*	certificate block.  If you specify \fB-F \fICAfile\fR or
/*	\fB-P \fICApath\fR, the OpenSSL library may augment the chain with
/*	missing issuer certificates.  To see the actual chain sent by the
/*	remote SMTP server leave \fICAfile\fR and \fICApath\fR unset.
/* .IP "\fB-d \fImdalg\fR (default: \fBsha1\fR)"
/*	The message digest algorithm to use for reporting remote SMTP server
/*	fingerprints and matching against user provided certificate
/*	fingerprints (with DANE TLSA records the algorithm is specified
/*	in the DNS).
/* .IP "\fB-f\fR"
/*	Lookup the associated DANE TLSA RRset even when a hostname is not an
/*	alias and its address records lie in an unsigned zone.  See
/*	smtp_tls_force_insecure_host_tlsa_lookup for details.
/* .IP "\fB-F \fICAfile.pem\fR (default: none)"
/*	The PEM formatted CAfile for remote SMTP server certificate
/*	verification.  By default no CAfile is used and no public CAs
/*	are trusted.
/* .IP "\fB-g \fIgrade\fR (default: medium)"
/*	The minimum TLS cipher grade used by posttls-finger.  See
/*	smtp_tls_mandatory_ciphers for details.
/* .IP "\fB-h \fIhost_lookup\fR (default: \fBdns\fR)"
/*	The hostname lookup methods used for the connection.  See the
/*	documentation of smtp_host_lookup for syntax and semantics.
/* .IP "\fB-l \fIlevel\fR (default: \fBdane\fR or \fBsecure\fR)"
/*	The security level for the connection, default \fBdane\fR or
/*	\fBsecure\fR depending on whether DNSSEC is available.  For syntax
/*	and semantics, see the documentation of smtp_tls_security_level.
/*	When \fBdane\fR or \fBdane-only\fR is supported and selected, if no
/*	TLSA records are found, or all the records found are unusable, the
/*	\fIsecure\fR level will be used instead.  The \fBfingerprint\fR
/*	security level allows you to test certificate or public-key
/*	fingerprint matches before you deploy them in the policy table.
/* .IP
/*	Note, since \fBposttls-finger\fR does not actually deliver any email,
/*	the \fBnone\fR, \fBmay\fR and \fBencrypt\fR security levels are not
/*	very useful.  Since \fBmay\fR and \fBencrypt\fR don't require peer
/*	certificates, they will often negotiate anonymous TLS ciphersuites,
/*	so you won't learn much about the remote SMTP server's certificates
/*	at these levels if it also supports anonymous TLS (though you may
/*	learn that the server supports anonymous TLS).
/* .IP "\fB-L \fIlogopts\fR (default: \fBroutine,certmatch\fR)"
/*	Fine-grained TLS logging options. To tune the TLS features logged
/*	during the TLS handshake, specify one or more of:
/* .RS
/* .IP "\fB0, none\fR"
/*	These yield no TLS logging; you'll generally want more, but this
/*	is handy if you just want the trust chain:
/* .RS
/* .ad
/* .nf
/*	$ posttls-finger -cC -L none destination
/* .fi
/* .RE
/* .IP "\fB1, routine, summary\fR"
/*	These synonymous values yield a normal one-line summary of the TLS
/*	connection.
/* .IP "\fB2, debug\fR"
/*	These synonymous values combine routine, ssl-debug, cache and verbose.
/* .IP "\fB3, ssl-expert\fR"
/*	These synonymous values combine debug with ssl-handshake-packet-dump.
/*	For experts only.
/* .IP "\fB4, ssl-developer\fR"
/*	These synonymous values combine ssl-expert with ssl-session-packet-dump.
/*	For experts only, and in most cases, use wireshark instead.
/* .IP "\fBssl-debug\fR"
/*	Turn on OpenSSL logging of the progress of the SSL handshake.
/* .IP "\fBssl-handshake-packet-dump\fR"
/*	Log hexadecimal packet dumps of the SSL handshake; for experts only.
/* .IP "\fBssl-session-packet-dump\fR"
/*	Log hexadecimal packet dumps of the entire SSL session; only useful
/*	to those who can debug SSL protocol problems from hex dumps.
/* .IP "\fBuntrusted\fR"
/*	Logs trust chain verification problems.  This is turned on
/*	automatically at security levels that use peer names signed
/*	by certificate authorities to validate certificates.  So while
/*	this setting is recognized, you should never need to set it
/*	explicitly.
/* .IP "\fBpeercert\fR"
/*	This logs a one line summary of the remote SMTP server certificate
/*	subject, issuer, and fingerprints.
/* .IP "\fBcertmatch\fR"
/*	This logs remote SMTP server certificate matching, showing the CN
/*	and each subjectAltName and which name matched.  With DANE, logs
/*	matching of TLSA record trust-anchor and end-entity certificates.
/* .IP "\fBcache\fR"
/*	This logs session cache operations, showing whether session caching
/*	is effective with the remote SMTP server.  Automatically used when
/*	reconnecting with the \fB-r\fR option; rarely needs to be set
/*	explicitly.
/* .IP "\fBverbose\fR"
/*	Enables verbose logging in the Postfix TLS driver; includes all of
/*	peercert..cache and more.
/* .RE
/* .IP
/*	The default is \fBroutine,certmatch\fR. After a reconnect,
/*	\fBpeercert\fR, \fBcertmatch\fR and \fBverbose\fR are automatically
/*	disabled while \fBcache\fR and \fBsummary\fR are enabled.
/* .IP "\fB-m \fIcount\fR (default: \fB5\fR)"
/*	When the \fB-r \fIdelay\fR option is specified, the \fB-m\fR option
/*	determines the maximum number of reconnect attempts to use with
/*	a server behind a load-balacer, to see whether connection caching
/*	is likely to be effective for this destination.  Some MTAs
/*	don't expose the underlying server identity in their EHLO
/*	response; with these servers there will never be more than
/*	1 reconnection attempt.
/* .IP "\fB-o \fIname=value\fR"
/*	Specify zero or more times to override the value of the main.cf
/*	parameter \fIname\fR with \fIvalue\fR.  Possible use-cases include
/*	overriding the values of TLS library parameters, or "myhostname" to
/*	configure the SMTP EHLO name sent to the remote server.
/* .IP "\fB-p \fIprotocols\fR (default: !SSLv2)"
/*	List of TLS protocols that posttls-finger will exclude or include.  See
/*	smtp_tls_mandatory_protocols for details.
/* .IP "\fB-P \fICApath/\fR (default: none)"
/*	The OpenSSL CApath/ directory (indexed via c_rehash(1)) for remote
/*	SMTP server certificate verification.  By default no CApath is used
/*	and no public CAs are trusted.
/* .IP "\fB-r \fIdelay\fR"
/*	With a cachable TLS session, disconnect and reconnect after \fIdelay\fR
/*	seconds. Report whether the session is re-used. Retry if a new server
/*	is encountered, up to 5 times or as specified with the \fB-m\fR option.
/*	By default reconnection is disabled, specify a positive delay to
/*	enable this behavior.
/* .IP "\fB-S\fR"
/*	Disable SMTP; that is, connect to an LMTP server. The default port for
/*	LMTP over TCP is 24.  Alternative ports can specified by appending
/*	"\fI:servicename\fR" or ":\fIportnumber\fR" to the destination
/*	argument.
/* .IP "\fB-t \fItimeout\fR (default: \fB30\fR)"
/*	The TCP connection timeout to use.  This is also the timeout for
/*	reading the remote server's 220 banner.
/* .IP "\fB-T \fItimeout\fR (default: \fB30\fR)"
/*	The SMTP/LMTP command timeout for EHLO/LHLO, STARTTLS and QUIT.
/* .IP "\fB-v\fR"
/*	Enable verose Postfix logging.  Specify more than once to increase
/*	the level of verbose logging.
/* .IP "[\fBinet:\fR]\fIdomain\fR[:\fIport\fR]"
/*	Connect via TCP to domain \fIdomain\fR, port \fIport\fR. The default
/*	port is \fBsmtp\fR (or 24 with LMTP).  With SMTP an MX lookup is
/*	performed to resolve the domain to a host, unless the domain is
/*	enclosed in \fB[]\fR.  If you want to connect to a specific MX host,
/*	for instance \fImx1.example.com\fR, specify [\fImx1.example.com\fR]
/*	as the destination and \fIexample.com\fR as a \fBmatch\fR argument.
/*	When using DNS, the destination domain is assumed fully qualified
/*	and no default domain or search suffixes are applied; you must use
/*	fully-qualified names or also enable \fBnative\fR host lookups
/*	(these don't support \fBdane\fR or \fBdane-only\fR as no DNSSEC
/*	validation information is available via \fBnative\fR lookups).
/* .IP "\fBunix:\fIpathname\fR"
/*	Connect to the UNIX-domain socket at \fIpathname\fR. LMTP only.
/* .IP "\fBmatch ...\fR"
/*	With no match arguments specified, certificate peername matching uses
/*	the compiled-in default strategies for each security level.  If you
/*	specify one or more arguments, these will be used as the list of
/*	certificate or public-key digests to match for the \fBfingerprint\fR
/*	level, or as the list of DNS names to match in the certificate at the
/*	\fBverify\fR and \fBsecure\fR levels.  If the security level is
/*	\fBdane\fR, or \fBdane-only\fR the match names are ignored, and
/*	\fBhostname, nexthop\fR strategies are used.
/* .ad
/* .fi
/* ENVIRONMENT
/* .ad
/* .fi
/* .IP \fBMAIL_CONFIG\fR
/*	Read configuration parameters from a non-default location.
/* .IP \fBMAIL_VERBOSE\fR
/*	Same as \fB-v\fR option.
/* SEE ALSO
/*	smtp-source(1), SMTP/LMTP message source
/*	smtp-sink(1), SMTP/LMTP message dump
/*
/* README FILES
/* .ad
/* .fi
/*	Use "\fBpostconf readme_directory\fR" or "\fBpostconf
/*	html_directory\fR" to locate this information.
/* .na
/* .nf
/*	TLS_README, Postfix STARTTLS howto
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Viktor Dukhovni
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>

 /*
  * Utility library.
  */
#include <msg.h>
#include <msg_vstream.h>
#include <vstring.h>
#include <vstream.h>
#include <vstring_vstream.h>
#include <mymalloc.h>
#include <stringops.h>
#include <argv.h>
#include <name_mask.h>
#include <name_code.h>
#include <chroot_uid.h>
#include <host_port.h>
#include <inet_proto.h>
#include <iostuff.h>
#include <timed_connect.h>
#include <sane_connect.h>
#include <myaddrinfo.h>
#include <sock_addr.h>

#define STR(x)		vstring_str(x)

 /*
  * Global library.
  */
#include <mail_params.h>
#include <mail_conf.h>
#include <smtp_stream.h>
#include <dsn_buf.h>

/* DNS library. */

#include <dns.h>

 /*
  * master library
  */
#include <mail_server.h>

 /*
  * TLS Library
  */
#define TLS_INTERNAL
#include <tls.h>

#ifdef USE_TLS
#include <openssl/engine.h>
#endif

 /*
  * Application specific
  */
#include "tlsmgrmem.h"

static int conn_tmout = 30;
static int smtp_tmout = 30;

#define HOST_FLAG_DNS		(1<<0)
#define HOST_FLAG_NATIVE	(1<<1)

#define MISC_FLAG_PREF_IPV6	(1<<0)
#define MISC_FLAG_PREF_IPV4	(1<<1)

static const NAME_MASK lookup_masks[] = {
    "dns", HOST_FLAG_DNS,
    "native", HOST_FLAG_NATIVE,
    0,
};

static const NAME_CODE addr_pref_map[] = {
    INET_PROTO_NAME_IPV6, MISC_FLAG_PREF_IPV6,
    INET_PROTO_NAME_IPV4, MISC_FLAG_PREF_IPV4,
    INET_PROTO_NAME_ANY, 0,
    0, -1,
};

typedef struct OPTIONS {
    char   *logopts;
    char   *level;
    ARGV   *tas;
    char   *host_lookup;
    char   *addr_pref;
} OPTIONS;

 /*
  * Per-session data structure with state.
  * 
  * This software can maintain multiple parallel connections to the same SMTP
  * server. However, it makes no more than one connection request at a time
  * to avoid overwhelming the server with SYN packets and having to back off.
  * Back-off would screw up the benchmark. Pending connection requests are
  * kept in a linear list.
  */
typedef struct STATE {
    int     smtp;			/* SMTP or LMTP? */
    int     host_lookup;		/* dns|native|dns,native */
    int     addr_pref;			/* v4, v6, both */
    int     log_mask;			/* via tls_log_mask() */
    int     reconnect;			/* -r option */
    int     max_reconnect;		/* -m option */
    int     force_tlsa;			/* -f option */
    unsigned port;			/* TCP port */
    char   *dest;			/* Full destination spec */
    char   *addrport;			/* [addr]:port */
    char   *namaddrport;		/* name[addr]:port */
    char   *nexthop;			/* Nexthop domain for verification */
    char   *hostname;			/* Hostname for verification */
    DNS_RR *addr;			/* IPv[46] Address to (re)connect to */
    DNS_RR *mx;				/* MX RRset qname, rname, valid */
    int     pass;			/* Pass number, 2 for reconnect */
    int     nochat;			/* disable chat logging */
    char   *helo;			/* Server name from EHLO reply */
    DSN_BUF *why;			/* SMTP-style error message */
    VSTRING *buffer;			/* Response buffer */
    VSTREAM *stream;			/* Open connection */
    int     level;			/* TLS security level */
#ifdef USE_TLS
    char   *mdalg;			/* fingerprint digest algorithm */
    char   *CAfile;			/* Trusted public CAs */
    char   *CApath;			/* Trusted public CAs */
    ARGV   *match;			/* match arguments */
    int     print_trust;		/* -C option */
    BIO    *tls_bio;			/* BIO wrapper for stdout */
    TLS_APPL_STATE *tls_ctx;		/* Application TLS context */
    TLS_SESS_STATE *tls_context;	/* Session TLS context */
    TLS_DANE *dane;			/* DANE TLSA validation structure */
    TLS_DANE *ddane;			/* DANE TLSA from DNS */
    char   *grade;			/* Minimum cipher grade */
    char   *protocols;			/* Protocol inclusion/exclusion */
#endif
    OPTIONS options;			/* JCL */
} STATE;

static DNS_RR *host_addr(STATE *, const char *);

#define HNAME(addr) (addr->qname)

 /*
  * Structure with broken-up SMTP server response.
  */
typedef struct {			/* server response */
    int     code;			/* status */
    char   *str;			/* text */
    VSTRING *buf;			/* origin of text */
} RESPONSE;


/* command - send an SMTP command */

static void command(STATE *state, int verbose, char *fmt,...)
{
    VSTREAM *stream = state->stream;
    VSTRING *buf;
    va_list ap;
    char   *line;

    buf = vstring_alloc(100);
    va_start(ap, fmt);
    vstring_vsprintf(buf, fmt, ap);
    va_end(ap);
    line = vstring_str(buf);

    while (line && *line) {
	char   *nextline = strchr(line, '\n');

	if (nextline)
	    *nextline++ = '\0';
	if (verbose && !state->nochat)
	    msg_info("> %s", line);
	smtp_printf(stream, "%s", line);
	line = nextline;
    }

    vstring_free(buf);
}

/* response - read and process SMTP server response */

static RESPONSE *response(STATE *state, int verbose)
{
    VSTREAM *stream = state->stream;
    VSTRING *buf = state->buffer;
    static RESPONSE rdata;
    int     more;
    char   *cp;

    /*
     * Initialize the response data buffer. Defend against a denial of
     * service attack by limiting the amount of multi-line text that we are
     * willing to store.
     */
    if (rdata.buf == 0) {
	rdata.buf = vstring_alloc(100);
	vstring_ctl(rdata.buf, VSTRING_CTL_MAXLEN, (ssize_t) var_line_limit, 0);
    }

    /*
     * Censor out non-printable characters in server responses. Concatenate
     * multi-line server responses. Separate the status code from the text.
     * Leave further parsing up to the application.
     */
#define BUF ((char *) vstring_str(buf))
    VSTRING_RESET(rdata.buf);
    for (;;) {
	smtp_get(buf, stream, var_line_limit, SMTP_GET_FLAG_SKIP);
	for (cp = BUF; *cp != 0; cp++)
	    if (!ISPRINT(*cp) && !ISSPACE(*cp))
		*cp = '?';
	cp = BUF;
	if (verbose && !state->nochat)
	    msg_info("< %s", cp);
	while (ISDIGIT(*cp))
	    cp++;
	rdata.code = (cp - BUF == 3 ? atoi(BUF) : 0);
	if ((more = (*cp == '-')) != 0)
	    cp++;
	while (ISSPACE(*cp))
	    cp++;
	vstring_strcat(rdata.buf, cp);
	if (more == 0)
	    break;
	VSTRING_ADDCH(rdata.buf, '\n');
    }
    VSTRING_TERMINATE(rdata.buf);
    rdata.str = vstring_str(rdata.buf);
    return (&rdata);
}

/* exception_text - translate exceptions from the smtp_stream module */

static char *exception_text(int except)
{
    switch (except) {
	case SMTP_ERR_EOF:
	return ("lost connection");
    case SMTP_ERR_TIME:
	return ("timeout");
    default:
	msg_panic("exception_text: unknown exception %d", except);
    }
}

/* ehlo - send EHLO/LHLO */

static RESPONSE *ehlo(STATE *state)
{
    int     except;
    int     verbose;
    volatile char *ehlo = state->smtp ? "EHLO" : "LHLO";
    VSTREAM *stream = state->stream;
    RESPONSE *resp;

#ifdef USE_TLS
    verbose = (state->pass == 1 && state->nochat == 0);
#else
    verbose = 1;
#endif

    /*
     * Send the standard greeting with our hostname
     */
    if ((except = vstream_setjmp(stream)) != 0) {
	msg_info("%s while sending %s", exception_text(except), ehlo);
	return (0);
    }
    command(state, verbose, "%s %s", ehlo, var_myhostname);

    resp = response(state, verbose);
    if (resp->code / 100 != 2) {
	msg_info("%s rejected: %d %s", ehlo, resp->code, resp->str);
	return (0);
    }
    return resp;
}

#ifdef USE_TLS

static void print_stack(STATE *state, x509_stack_t *sk, int trustout)
{
    int     i;

    for (i = 0; i < sk_X509_num(sk); i++) {
	X509   *cert = sk_X509_value(sk, i);
	char    buf[CCERT_BUFSIZ];
	X509_NAME *xn;
	char   *digest;

	if ((xn = X509_get_subject_name(cert)) != 0) {
	    X509_NAME_oneline(xn, buf, sizeof buf);
	    BIO_printf(state->tls_bio, "%2d subject: %s\n", i, buf);
	}
	if ((xn = X509_get_issuer_name(cert)) != 0) {
	    X509_NAME_oneline(xn, buf, sizeof buf);
	    BIO_printf(state->tls_bio, "    issuer: %s\n", buf);
	}
	digest = tls_cert_fprint(cert, state->mdalg);
	BIO_printf(state->tls_bio, "   cert digest=%s\n", digest);
	myfree(digest);

	digest = tls_pkey_fprint(cert, state->mdalg);
	BIO_printf(state->tls_bio, "   pkey digest=%s\n", digest);
	myfree(digest);

	if (trustout)
	    PEM_write_bio_X509_AUX(state->tls_bio, cert);
	else
	    PEM_write_bio_X509(state->tls_bio, cert);
    }
}

static void print_trust_info(STATE *state)
{
    x509_stack_t *sk = SSL_get_peer_cert_chain(state->tls_context->con);

    if (sk != 0) {
	BIO_printf(state->tls_bio, "\n---\nCertificate chain\n");
	print_stack(state, sk, 0);
    }
#ifdef dane_verify_debug
    /* print internally constructed untrusted chain */
    if ((sk = state->tls_context->untrusted) != 0) {
	BIO_printf(state->tls_bio, "\n---\nUntrusted chain\n");
	print_stack(state, sk, 0);
    }
    /* print associated root CA */
    if ((sk = state->tls_context->trusted) != 0) {
	BIO_printf(state->tls_bio, "\n---\nTrusted chain\n");
	print_stack(state, sk, 1);
    }
#endif
}

/* starttls - SMTP STARTTLS handshake */

static int starttls(STATE *state)
{
    VSTRING *cipher_exclusions;
    int     except;
    RESPONSE *resp;
    VSTREAM *stream = state->stream;
    TLS_CLIENT_START_PROPS tls_props;

    /* SMTP stream with deadline timeouts */
    smtp_stream_setup(stream, smtp_tmout, 1);
    if ((except = vstream_setjmp(stream)) != 0) {
	msg_fatal("%s while sending STARTTLS", exception_text(except));
	return (1);
    }
    command(state, state->pass == 1, "STARTTLS");

    resp = response(state, state->pass == 1);
    if (resp->code / 100 != 2) {
	msg_info("STARTTLS rejected: %d %s", resp->code, resp->str);
	return (1);
    }

    /*
     * Discard any plain-text data that may be piggybacked after the server's
     * 220 STARTTLS reply. Should we abort the session instead?
     */
    vstream_fpurge(stream, VSTREAM_PURGE_READ);

#define ADD_EXCLUDE(vstr, str) \
    do { \
	if (*(str)) \
	    vstring_sprintf_append((vstr), "%s%s", \
				   VSTRING_LEN(vstr) ? " " : "", (str)); \
    } while (0)

    cipher_exclusions = vstring_alloc(10);
    ADD_EXCLUDE(cipher_exclusions, DEF_SMTP_TLS_EXCL_CIPH);
    if (TLS_REQUIRED(state->level))
	ADD_EXCLUDE(cipher_exclusions, DEF_SMTP_TLS_MAND_EXCL);

    /*
     * If we're authenticating suppress anonymous ciphersuites, otherwise at
     * least encrypt, not much point in doing neither.
     */
    if (TLS_MUST_MATCH(state->level))
	ADD_EXCLUDE(cipher_exclusions, "aNULL");
    else
	ADD_EXCLUDE(cipher_exclusions, "eNULL");

    state->tls_context =
	TLS_CLIENT_START(&tls_props,
			 ctx = state->tls_ctx,
			 stream = stream,
			 timeout = smtp_tmout,
			 tls_level = state->level,
			 nexthop = state->nexthop,
			 host = state->hostname,
			 namaddr = state->namaddrport,
			 serverid = state->addrport,
			 helo = state->helo ? state->helo : "",
			 protocols = state->protocols,
			 cipher_grade = state->grade,
			 cipher_exclusions
			 = vstring_str(cipher_exclusions),
			 matchargv = state->match,
			 mdalg = state->mdalg,
			 dane = state->ddane ? state->ddane : state->dane);
    vstring_free(cipher_exclusions);
    if (state->helo) {
	myfree(state->helo);
	state->helo = 0;
    }
    if (state->tls_context == 0) {
	/* We must avoid further I/O, the peer is in an undefined state. */
	(void) vstream_fpurge(stream, VSTREAM_PURGE_BOTH);
	(void) vstream_fclose(stream);
	state->stream = 0;
	return (1);
    }
    if (state->pass == 1) {
	ehlo(state);
	if (!TLS_CERT_IS_PRESENT(state->tls_context))
	    msg_info("Server is anonymous");
	else if (state->print_trust)
	    print_trust_info(state);
	state->log_mask &= ~(TLS_LOG_CERTMATCH | TLS_LOG_PEERCERT |
			     TLS_LOG_VERBOSE | TLS_LOG_UNTRUSTED);
	state->log_mask |= TLS_LOG_CACHE | TLS_LOG_SUMMARY;
	tls_update_app_logmask(state->tls_ctx, state->log_mask);
    }
    return (0);
}

#endif

/* doproto - do SMTP handshake */

static int doproto(STATE *state)
{
    VSTREAM *stream = state->stream;
    RESPONSE *resp;
    int     except;
    int     n;
    char   *lines;
    char   *words;
    char   *word;

    /*
     * Prepare for disaster.
     */
    smtp_stream_setup(stream, conn_tmout, 1);
    if ((except = vstream_setjmp(stream)) != 0)
	msg_fatal("%s while reading server greeting", exception_text(except));

    /*
     * Read and parse the server's SMTP greeting banner.
     */
    if (((resp = response(state, 1))->code / 100) != 2) {
	msg_info("SMTP service not available: %d %s", resp->code, resp->str);
	return (1);
    }

    /*
     * Send the standard greeting with our hostname
     */
    if ((resp = ehlo(state)) == 0)
	return (1);

    lines = resp->str;
    for (n = 0; (words = mystrtok(&lines, "\n")) != 0; ++n) {
	if ((word = mystrtok(&words, " \t=")) != 0) {
	    if (n == 0)
		state->helo = mystrdup(word);
	    if (strcasecmp(word, "STARTTLS") == 0)
		break;
	}
    }

#ifdef USE_TLS
    if (words && state->tls_ctx)
	if (starttls(state))
	    return (1);
#endif

    /*
     * Prepare for disaster.
     */
    smtp_stream_setup(stream, smtp_tmout, 1);
    if ((except = vstream_setjmp(stream)) != 0) {
	msg_warn("%s while sending QUIT command", exception_text(except));
	return (0);
    }
    command(state, 1, "QUIT");
    (void) response(state, 1);

    return (0);
}

/* connect_sock - connect a socket over some transport */

static VSTREAM *connect_sock(int sock, struct sockaddr *sa, int salen,
		           const char *name, const char *addr, STATE *state)
{
    DSN_BUF *why = state->why;
    int     conn_stat;
    int     saved_errno;
    VSTREAM *stream;

    if (conn_tmout > 0) {
	non_blocking(sock, NON_BLOCKING);
	conn_stat = timed_connect(sock, sa, salen, conn_tmout);
	saved_errno = errno;
	non_blocking(sock, BLOCKING);
	errno = saved_errno;
    } else {
	conn_stat = sane_connect(sock, sa, salen);
    }
    if (conn_stat < 0) {
	if (state->port)
	    dsb_simple(why, "4.4.1", "connect to %s[%s]:%d: %m",
		       name, addr, ntohs(state->port));
	else
	    dsb_simple(why, "4.4.1", "connect to %s[%s]: %m", name, addr);
	close(sock);
	return (0);
    }
    stream = vstream_fdopen(sock, O_RDWR);
    state->namaddrport =
	vstring_export(state->port == 0 ?
		  vstring_sprintf(vstring_alloc(10), "%s[%s]", name, addr) :
		       vstring_sprintf(vstring_alloc(10), "%s[%s]:%u",
				       name, addr, ntohs(state->port)));
    state->addrport =
	vstring_export(state->port == 0 ?
		       vstring_sprintf(vstring_alloc(10), "%s", addr) :
		       vstring_sprintf(vstring_alloc(10), "[%s]:%u",
				       addr, ntohs(state->port)));

    /*
     * Avoid poor performance when TCP MSS > VSTREAM_BUFSIZE.
     */
    if (sa->sa_family == AF_INET
#ifdef AF_INET6
	|| sa->sa_family == AF_INET6
#endif
	)
	vstream_tweak_tcp(stream);

    return (stream);
}

/* connect_unix - connect to a unix-domain socket */

static VSTREAM *connect_unix(STATE *state, const char *path)
{
    static const char *myname = "connect_unix";
    DSN_BUF *why = state->why;
    struct sockaddr_un sock_un;
    int     len = strlen(path);
    int     sock;

    if (!state->nexthop)
	state->nexthop = mystrdup(var_myhostname);
    state->hostname = mystrdup(var_myhostname);

    dsb_reset(why);				/* Paranoia */

    /*
     * Sanity checks.
     */
    if (len >= (int) sizeof(sock_un.sun_path)) {
	dsb_simple(why, "4.3.5", "unix-domain name too long: %s", path);
	return (0);
    }

    /*
     * Initialize.
     */
    memset((char *) &sock_un, 0, sizeof(sock_un));
    sock_un.sun_family = AF_UNIX;
#ifdef HAS_SUN_LEN
    sock_un.sun_len = len + 1;
#endif
    memcpy(sock_un.sun_path, path, len + 1);

    /*
     * Create a client socket.
     */
    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	msg_fatal("%s: socket: %m", myname);

    /*
     * Connect to the server.
     */
    if (msg_verbose)
	msg_info("%s: trying: %s...", myname, path);

    return (connect_sock(sock, (struct sockaddr *) &sock_un, sizeof(sock_un),
			 var_myhostname, path, state));
}

/* connect_addr - connect to explicit address */

static VSTREAM *connect_addr(STATE *state, DNS_RR *addr)
{
    static const char *myname = "connect_addr";
    DSN_BUF *why = state->why;
    struct sockaddr_storage ss;		/* remote */
    struct sockaddr *sa = (struct sockaddr *) &ss;
    SOCKADDR_SIZE salen = sizeof(ss);
    MAI_HOSTADDR_STR hostaddr;
    int     sock;

    dsb_reset(why);				/* Paranoia */

    /*
     * Sanity checks.
     */
    if (dns_rr_to_sa(addr, state->port, sa, &salen) != 0) {
	msg_warn("%s: skip address type %s: %m",
		 myname, dns_strtype(addr->type));
	dsb_simple(why, "4.4.0", "network address conversion failed: %m");
	return (0);
    }

    /*
     * Initialize.
     */
    if ((sock = socket(sa->sa_family, SOCK_STREAM, 0)) < 0)
	msg_fatal("%s: socket: %m", myname);

    if (inet_windowsize > 0)
	set_inet_windowsize(sock, inet_windowsize);

    /*
     * Connect to the server.
     */
    SOCKADDR_TO_HOSTADDR(sa, salen, &hostaddr, (MAI_SERVPORT_STR *) 0, 0);
    if (msg_verbose)
	msg_info("%s: trying: %s[%s] port %d...",
		 myname, HNAME(addr), hostaddr.buf, ntohs(state->port));

    return (connect_sock(sock, sa, salen, HNAME(addr), hostaddr.buf, state));
}

#define HAS_DSN(why)		(STR((why)->status)[0] != 0)
#define HAS_SOFT_DSN(why)	(STR((why)->status)[0] == '4')
#define HAS_HARD_DSN(why)	(STR((why)->status)[0] == '5')
#define HAS_LOOP_DSN(why) \
    (HAS_DSN(why) && strcmp(STR((why)->status) + 1, ".4.6") == 0)

#define SET_SOFT_DSN(why)	(STR((why)->status)[0] = '4')
#define SET_HARD_DSN(why)	(STR((why)->status)[0] = '5')

/* addr_one - address lookup for one host name */

static DNS_RR *addr_one(STATE *state, DNS_RR *addr_list, const char *host,
			        int res_opt, unsigned pref)
{
    static const char *myname = "addr_one";
    DSN_BUF *why = state->why;
    DNS_RR *addr = 0;
    DNS_RR *rr;
    int     aierr;
    struct addrinfo *res0;
    struct addrinfo *res;
    INET_PROTO_INFO *proto_info = inet_proto_info();
    int     found;

    if (msg_verbose)
	msg_info("%s: host %s", myname, host);

    /*
     * Interpret a numerical name as an address.
     */
    if (hostaddr_to_sockaddr(host, (char *) 0, 0, &res0) == 0
     && strchr((char *) proto_info->sa_family_list, res0->ai_family) != 0) {
	if ((addr = dns_sa_to_rr(host, pref, res0->ai_addr)) == 0)
	    msg_fatal("host %s: conversion error for address family %d: %m",
		    host, ((struct sockaddr *) (res0->ai_addr))->sa_family);
	addr_list = dns_rr_append(addr_list, addr);
	freeaddrinfo(res0);
	return (addr_list);
    }

    /*
     * Use DNS lookup, but keep the option open to use native name service.
     * 
     * XXX A soft error dominates past and future hard errors. Therefore we
     * should not clobber a soft error text and status code.
     */
    if (state->host_lookup & HOST_FLAG_DNS) {
	switch (dns_lookup_v(host, res_opt, &addr, (VSTRING *) 0,
			     why->reason, DNS_REQ_FLAG_NONE,
			     proto_info->dns_atype_list)) {
	case DNS_OK:
	    for (rr = addr; rr; rr = rr->next)
		rr->pref = pref;
	    addr_list = dns_rr_append(addr_list, addr);
	    return (addr_list);
	default:
	    dsb_status(why, "4.4.3");
	    return (addr_list);
	case DNS_FAIL:
	    dsb_status(why, HAS_SOFT_DSN(why) ? "4.4.3" : "5.4.3");
	    return (addr_list);
	case DNS_INVAL:
	    dsb_status(why, HAS_SOFT_DSN(why) ? "4.4.4" : "5.4.4");
	    return (addr_list);
	case DNS_NOTFOUND:
	    dsb_status(why, HAS_SOFT_DSN(why) ? "4.4.4" : "5.4.4");
	    /* maybe native naming service will succeed */
	    break;
	}
    }

    /*
     * Use the native name service which also looks in /etc/hosts.
     * 
     * XXX A soft error dominates past and future hard errors. Therefore we
     * should not clobber a soft error text and status code.
     */
#define RETRY_AI_ERROR(e) \
        ((e) == EAI_AGAIN || (e) == EAI_MEMORY || (e) == EAI_SYSTEM)
#ifdef EAI_NODATA
#define DSN_NOHOST(e) \
	((e) == EAI_AGAIN || (e) == EAI_NODATA || (e) == EAI_NONAME)
#else
#define DSN_NOHOST(e) \
	((e) == EAI_AGAIN || (e) == EAI_NONAME)
#endif

    if (state->host_lookup & HOST_FLAG_NATIVE) {
	if ((aierr = hostname_to_sockaddr(host, (char *) 0, 0, &res0)) != 0) {
	    dsb_simple(why, (HAS_SOFT_DSN(why) || RETRY_AI_ERROR(aierr)) ?
		       (DSN_NOHOST(aierr) ? "4.4.4" : "4.3.0") :
		       (DSN_NOHOST(aierr) ? "5.4.4" : "5.3.0"),
		       "unable to look up host %s: %s",
		       host, MAI_STRERROR(aierr));
	} else {
	    for (found = 0, res = res0; res != 0; res = res->ai_next) {
		if (strchr((char *) proto_info->sa_family_list, res->ai_family) == 0) {
		    msg_info("skipping address family %d for host %s",
			     res->ai_family, host);
		    continue;
		}
		found++;
		if ((addr = dns_sa_to_rr(host, pref, res->ai_addr)) == 0)
		    msg_fatal("host %s: conversion error for address family %d: %m",
		    host, ((struct sockaddr *) (res0->ai_addr))->sa_family);
		addr_list = dns_rr_append(addr_list, addr);
	    }
	    freeaddrinfo(res0);
	    if (found == 0) {
		dsb_simple(why, HAS_SOFT_DSN(why) ? "4.4.4" : "5.4.4",
			   "%s: host not found", host);
	    }
	    return (addr_list);
	}
    }

    /*
     * No further alternatives for host lookup.
     */
    return (addr_list);
}

/* mx_addr_list - address lookup for a list of mail exchangers */

static DNS_RR *mx_addr_list(STATE *state, DNS_RR *mx_names)
{
    static const char *myname = "mx_addr_list";
    DNS_RR *addr_list = 0;
    DNS_RR *rr;
    int     res_opt = mx_names->dnssec_valid ? RES_USE_DNSSEC : 0;

    for (rr = mx_names; rr; rr = rr->next) {
	if (rr->type != T_MX)
	    msg_panic("%s: bad resource type: %d", myname, rr->type);
	addr_list = addr_one(state, addr_list, (char *) rr->data, res_opt,
			     rr->pref);
    }
    return (addr_list);
}

/* smtp_domain_addr - mail exchanger address lookup */

static DNS_RR *domain_addr(STATE *state, char *domain)
{
    DNS_RR *mx_names;
    DNS_RR *addr_list = 0;
    int     r = 0;			/* Resolver flags */

    dsb_reset(state->why);

#if (RES_USE_DNSSEC != 0) && (RES_USE_EDNS0 != 0)
    r |= RES_USE_DNSSEC;
#endif

    switch (dns_lookup(domain, T_MX, r, &mx_names, (VSTRING *) 0,
		       state->why->reason)) {
    default:
	dsb_status(state->why, "4.4.3");
	break;
    case DNS_INVAL:
	dsb_status(state->why, "5.4.4");
	break;
    case DNS_FAIL:
	dsb_status(state->why, "5.4.3");
	break;
    case DNS_OK:
	mx_names = dns_rr_sort(mx_names, dns_rr_compare_pref_any);
	addr_list = mx_addr_list(state, mx_names);
	state->mx = dns_rr_copy(mx_names);
	dns_rr_free(mx_names);
	if (addr_list == 0) {
	    msg_warn("no MX host for %s has a valid address record", domain);
	    break;
	}
#define COMPARE_ADDR(flags) \
	((flags & MISC_FLAG_PREF_IPV6) ? dns_rr_compare_pref_ipv6 : \
	 (flags & MISC_FLAG_PREF_IPV4) ? dns_rr_compare_pref_ipv4 : \
	 dns_rr_compare_pref_any)
	if (addr_list && addr_list->next) {
	    addr_list = dns_rr_shuffle(addr_list);
	    addr_list = dns_rr_sort(addr_list, COMPARE_ADDR(state->addr_pref));
	}
	break;
    case DNS_NOTFOUND:
	addr_list = host_addr(state, domain);
	break;
    }

    return (addr_list);
}

/* host_addr - direct host lookup */

static DNS_RR *host_addr(STATE *state, const char *host)
{
    DSN_BUF *why = state->why;
    DNS_RR *addr_list;
    int     res_opt = 0;

    dsb_reset(why);				/* Paranoia */

#if (RES_USE_DNSSEC != 0) && (RES_USE_EDNS0 != 0)
    res_opt |= RES_USE_DNSSEC;
#endif

#define PREF0	0
    addr_list = addr_one(state, (DNS_RR *) 0, host, res_opt, PREF0);
    if (addr_list && addr_list->next) {
	addr_list = dns_rr_shuffle(addr_list);
	if (inet_proto_info()->ai_family_list[1] != 0)
	    addr_list = dns_rr_sort(addr_list, COMPARE_ADDR(state->addr_pref));
    }
    return (addr_list);
}

/* dane_host_level - canidate host "dane" or degraded security level */

static int dane_host_level(STATE *state, DNS_RR *addr)
{
    int     level = state->level;

#ifdef USE_TLS
    if (level == TLS_LEV_DANE) {
	if (state->mx == 0 || state->mx->dnssec_valid) {
	    if (state->log_mask & (TLS_LOG_CERTMATCH | TLS_LOG_VERBOSE))
		tls_dane_verbose(1);
	    else
		tls_dane_verbose(0);

	    /* See addr loop in connect_remote() */
	    if (state->ddane)
		tls_dane_free(state->ddane);

	    /*
	     * When TLSA lookups fail, next host.  If unusable or not found,
	     * fallback to "secure"
	     */
	    state->ddane = tls_dane_resolve(state->port, "tcp", addr,
					    state->force_tlsa);
	    if (!state->ddane) {
		dsb_simple(state->why, "4.7.5",
			   "TLSA lookup error for %s:%u",
			   HNAME(addr), ntohs(state->port));
		level = TLS_LEV_INVALID;
	    } else if (tls_dane_notfound(state->ddane)
		       || tls_dane_unusable(state->ddane)) {
		if (msg_verbose)
		    msg_info("no %sTLSA records found, "
			     "resorting to \"secure\"",
			     tls_dane_unusable(state->ddane) ?
			     "usable " : "");
		level = TLS_LEV_SECURE;
	    } else if (!TLS_DANE_HASTA(state->ddane)
		       && !TLS_DANE_HASEE(state->ddane)) {
		msg_panic("empty DANE match list");
	    } else {
		if (state->match)
		    argv_free(state->match);
		argv_add(state->match = argv_alloc(2),
			 state->ddane->base_domain, ARGV_END);
		if (state->mx) {
		    if (strcmp(state->mx->qname, state->mx->rname) == 0)
			argv_add(state->match, state->mx->qname, ARGV_END);
		    else
			argv_add(state->match, state->mx->rname,
				 state->mx->qname, ARGV_END);
		}
	    }
	} else {
	    level = TLS_LEV_SECURE;
	}
    }
#endif

    return (level);
}

/* parse_destination - parse host/port destination */

static char *parse_destination(char *destination, char *def_service,
			               char **hostp, unsigned *portp)
{
    char   *buf = mystrdup(destination);
    char   *service;
    struct servent *sp;
    char   *protocol = "tcp";
    unsigned port;
    const char *err;

    if (msg_verbose)
	msg_info("parse_destination: %s %s", destination, def_service);

    /*
     * Parse the host/port information. We're working with a copy of the
     * destination argument so the parsing can be destructive.
     */
    if ((err = host_port(buf, hostp, (char *) 0, &service, def_service)) != 0)
	msg_fatal("%s in server description: %s", err, destination);

    /*
     * Convert service to port number, network byte order.
     */
    if (alldig(service)) {
	if ((port = atoi(service)) >= 65536 || port == 0)
	    msg_fatal("bad network port in destination: %s", destination);
	*portp = htons(port);
    } else {
	if ((sp = getservbyname(service, protocol)) != 0)
	    *portp = sp->s_port;
	else if (strcmp(service, "smtp") == 0)
	    *portp = htons(25);
	else
	    msg_fatal("unknown service: %s/%s", service, protocol);
    }
    return (buf);
}

/* connect_remote - connect to TCP destination or log an error */

static void connect_remote(STATE *state, char *dest)
{
    DNS_RR *addr;
    char   *buf;
    char   *domain;

    /* When reconnecting use IP address of previous session */
    if (state->addr == 0) {
	buf = parse_destination(dest, state->smtp ? "smtp" : "24",
				&domain, &state->port);
	if (!state->nexthop)
	    state->nexthop = mystrdup(domain);
	if (state->smtp == 0 || *dest == '[')
	    state->addr = host_addr(state, domain);
	else
	    state->addr = domain_addr(state, domain);
	myfree(buf);

	if (state->addr == 0) {
	    msg_info("Destination address lookup failed: %s",
		     vstring_str(state->why->reason));
	    return;
	}
    }
    for (addr = state->addr; addr; addr = addr->next) {
	int     level = dane_host_level(state, addr);

	if (level == TLS_LEV_INVALID
	    || (state->stream = connect_addr(state, addr)) == 0) {
	    msg_info("Failed to establish session to %s via %s: %s",
		     dest, HNAME(addr), vstring_str(state->why->reason));
	    continue;
	}
	/* We have a connection */
	state->level = level;
	state->hostname = mystrdup(HNAME(addr));

	/* We use the same address when reconnecting, so flush the rest. */
	addr = dns_rr_copy(addr);
	dns_rr_free(state->addr);
	state->addr = addr;
	break;
    }
}

/* connect_dest - connect to given inet: or unix: destination */

static int connect_dest(STATE *state)
{
    char   *dest = state->dest;

    /*
     * With LMTP we have direct-to-host delivery only. The destination may
     * have multiple IP addresses.
     */
    if (state->smtp == 0) {
	if (strncmp(dest, "unix:", 5) == 0) {
	    connect_unix(state, dest + 5);
	    if (!state->stream)
		msg_info("Failed to establish session to %s: %s",
			 dest, vstring_str(state->why->reason));
	    return (1);
	}
	if (strncmp(dest, "inet:", 5) == 0)
	    dest += 5;
    }
    connect_remote(state, dest);

    return (state->stream == 0);
}

static void disconnect_dest(STATE *state)
{
#ifdef USE_TLS
    if (state->tls_context)
	tls_client_stop(state->tls_ctx, state->stream,
			smtp_tmout, 0, state->tls_context);
    state->tls_context = 0;
    if (state->ddane)
	tls_dane_free(state->ddane);
    state->ddane = 0;
#endif

    if (state->stream)
	vstream_fclose(state->stream);
    state->stream = 0;

    if (state->namaddrport)
	myfree(state->namaddrport);
    state->namaddrport = 0;

    if (state->addrport)
	myfree(state->addrport);
    state->addrport = 0;

    /* Reused on reconnect */
    if (state->reconnect <= 0) {
	if (state->addr)
	    dns_rr_free(state->addr);
	state->addr = 0;
	if (state->mx)
	    dns_rr_free(state->mx);
	state->mx = 0;

	if (state->nexthop)
	    myfree(state->nexthop);
	state->nexthop = 0;
    }
    if (state->hostname)
	myfree(state->hostname);
    state->hostname = 0;

    dsb_free(state->why);
    vstring_free(state->buffer);
}

static int finger(STATE *state)
{
    int     err;

    /*
     * Make sure the SMTP server cannot run us out of memory by sending
     * never-ending lines of text.
     */
    state->buffer = vstring_alloc(100);
    vstring_ctl(state->buffer, VSTRING_CTL_MAXLEN,
		(ssize_t) var_line_limit, 0);
    state->why = dsb_create();

    if (!(err = connect_dest(state))) {
	if (state->pass == 1 && !state->nochat)
	    msg_info("Connected to %s", state->namaddrport);
	err = doproto(state);
    }
    disconnect_dest(state);

    if (err != 0)
	return (1);

#ifdef USE_TLS
    if (state->reconnect > 0) {
	int     cache_enabled;
	int     cache_count;
	int     cache_hits;

	tlsmgrmem_status(&cache_enabled, &cache_count, &cache_hits);
	if (cache_enabled && cache_count == 0) {
	    msg_info("Server declined session caching. Done reconnecting.");
	    state->reconnect = 0;
	} else if (cache_hits > 0 && (state->log_mask & TLS_LOG_SESSTKT) != 0) {
	    msg_info("Found a previously used server.  Done reconnecting.");
	    state->reconnect = 0;
	} else if (state->max_reconnect-- <= 0) {
	    msg_info("Maximum reconnect count reached.");
	    state->reconnect = 0;
	}
    }
#endif

    return (0);
}

#ifdef USE_TLS

/* ssl_cleanup - free memory allocated in the OpenSSL library */

static void ssl_cleanup(void)
{
#if OPENSSL_VERSION_NUMBER >= 0x10000000L
    ERR_remove_thread_state(0);		/* Thread-id is now a pointer */
#else
    ERR_remove_state(0);		/* Deprecated with OpenSSL 1.0.0 */
#endif
    ENGINE_cleanup();
    CONF_modules_unload(1);
    ERR_free_strings();
    EVP_cleanup();
    CRYPTO_cleanup_all_ex_data();
}

#endif

/* run - do what we were asked to do. */

static int run(STATE *state)
{

    while (1) {
	if (finger(state) != 0)
	    break;
	if (state->reconnect <= 0)
	    break;
	msg_info("Reconnecting after %d seconds", state->reconnect);
	++state->pass;
	sleep(state->reconnect);
    }

    return (0);
}

/* cleanup - free memory allocated in main */

static void cleanup(STATE *state)
{
#ifdef USE_TLS
    if (state->tls_ctx != 0)
	tls_free_app_context(state->tls_ctx);
    if (state->tls_bio)
	(void) BIO_free(state->tls_bio);
    state->tls_bio = 0;

    myfree(state->mdalg);
    myfree(state->CApath);
    myfree(state->CAfile);
    if (state->options.level)
	myfree(state->options.level);
    myfree(state->options.logopts);
    if (state->match)
	argv_free(state->match);
    if (state->options.tas)
	argv_free(state->options.tas);
    if (state->dane)
	tls_dane_free(state->dane);

    /* Flush and free DANE TLSA cache */
    tls_dane_flush();
    /* Flush and free memory tlsmgr cache */
    tlsmgrmem_flush();
    myfree(state->grade);
    myfree(state->protocols);
#endif
    myfree(state->options.host_lookup);
    myfree(state->dest);

    mail_conf_flush();
}

/* usage - explain */

static void usage(void)
{
#ifdef USE_TLS
    fprintf(stderr, "usage: %s %s \\\n\t%s \\\n\t%s \\\n\t%s"
	    " destination [match ...]\n", var_procname,
	    "[-acCfSv] [-t conn_tmout] [-T cmd_tmout] [-L logopts]",
	 "[-h host_lookup] [-l level] [-d mdalg] [-g grade] [-p protocols]",
	    "[-A tafile] [-F CAfile.pem] [-P CApath/] [-m count] [-r delay]",
	    "[-o name=value]");
#else
    fprintf(stderr, "usage: %s [-acStTv] [-h host_lookup] [-o name=value] destination\n",
	    var_procname);
#endif
    exit(1);
}

/* tls_init - initialize application TLS library context */

static void tls_init(STATE *state)
{
#ifdef USE_TLS
    TLS_CLIENT_INIT_PROPS props;

    if (state->level <= TLS_LEV_NONE)
	return;

    state->tls_ctx =
	TLS_CLIENT_INIT(&props,
			log_param = "-L option",
			log_level = state->options.logopts,
			verifydepth = DEF_SMTP_TLS_SCERT_VD,
			cache_type = "memory",
			cert_file = "",
			key_file = "",
			dcert_file = "",
			dkey_file = "",
			eccert_file = "",
			eckey_file = "",
			CAfile = state->CAfile,
			CApath = state->CApath,
			mdalg = state->mdalg);
#endif
}

/* override - update main.cf parameter */

static void override(const char *nameval)
{
    char   *param_name;
    char   *param_value;
    char   *save = mystrdup(nameval);

    if (split_nameval(save, &param_name, &param_value) != 0)
	usage();
    mail_conf_update(param_name, param_value);
    myfree(save);
}

/* parse_options - (argc, argv) -> state */

static void parse_options(STATE *state, int argc, char *argv[])
{
    int     c;

    state->smtp = 1;
    state->pass = 1;
    state->reconnect = -1;
    state->max_reconnect = 5;
#ifdef USE_TLS
    state->protocols = mystrdup("!SSLv2");
    state->grade = mystrdup("medium");
#endif
    memset((char *) &state->options, 0, sizeof(state->options));
    state->options.host_lookup = mystrdup("dns");

#define OPTS "a:ch:o:St:T:v"
#ifdef USE_TLS
#define TLSOPTS "A:Cd:fF:g:l:L:m:p:P:r:"

    state->mdalg = mystrdup("sha1");
    state->CApath = mystrdup("");
    state->CAfile = mystrdup("");
    state->options.tas = argv_alloc(1);
    state->options.logopts = 0;
    state->level = TLS_LEV_DANE;
#else
#define TLSOPTS ""
    state->level = TLS_LEV_NONE;
#endif

    while ((c = GETOPT(argc, argv, OPTS TLSOPTS)) > 0) {
	switch (c) {
	default:
	    usage();
	    break;
	case 'a':
	    state->options.addr_pref = mystrdup(optarg);
	    break;
	case 'c':
	    state->nochat = 1;
	    break;
	case 'h':
	    myfree(state->options.host_lookup);
	    state->options.host_lookup = mystrdup(optarg);
	    break;
	case 'o':
	    override(optarg);
	    break;
	case 'S':
	    state->smtp = 0;
	    break;
	case 't':
	    conn_tmout = atoi(optarg);
	    break;
	case 'T':
	    smtp_tmout = atoi(optarg);
	    break;
	case 'v':
	    msg_verbose++;
	    break;
#ifdef USE_TLS
	case 'A':
	    argv_add(state->options.tas, optarg, ARGV_END);
	    break;
	case 'C':
	    state->print_trust = 1;
	    break;
	case 'd':
	    myfree(state->mdalg);
	    state->mdalg = mystrdup(optarg);
	    break;
	case 'f':
	    state->force_tlsa = 1;
	    break;
	case 'F':
	    myfree(state->CAfile);
	    state->CAfile = mystrdup(optarg);
	    break;
	case 'g':
	    myfree(state->grade);
	    state->grade = mystrdup(optarg);
	    break;
	case 'l':
	    if (state->options.level)
		myfree(state->options.level);
	    state->options.level = mystrdup(optarg);
	    break;
	case 'L':
	    if (state->options.logopts)
		myfree(state->options.logopts);
	    state->options.logopts = mystrdup(optarg);
	    break;
	case 'm':
	    state->max_reconnect = atoi(optarg);
	    break;
	case 'p':
	    myfree(state->protocols);
	    state->protocols = mystrdup(optarg);
	    break;
	case 'P':
	    myfree(state->CApath);
	    state->CApath = mystrdup(optarg);
	    break;
	case 'r':
	    state->reconnect = atoi(optarg);
	    break;
#endif
	}
    }

    /*
     * Address family preference.
     */
    state->addr_pref =
	name_code(addr_pref_map, NAME_CODE_FLAG_NONE, state->options.addr_pref ?
		  state->options.addr_pref : "any");
    if (state->addr_pref < 0)
	msg_fatal("bad '-a' option value: %s", state->options.addr_pref);

    /*
     * Select hostname lookup mechanisms.
     */
    state->host_lookup =
	name_mask("-h option", lookup_masks, state->options.host_lookup ?
		  state->options.host_lookup : "dns");

#ifdef USE_TLS

    if (state->reconnect < 0)
	tlsmgrmem_disable();

    if (state->options.logopts == 0)
	state->options.logopts = mystrdup("routine,certmatch");
    state->log_mask = tls_log_mask("-L option", state->options.logopts);

    if (state->options.level) {
	state->level = tls_level_lookup(state->options.level);

	switch (state->level) {
	case TLS_LEV_DANE_ONLY:
	    state->level = TLS_LEV_DANE;
	    break;
	case TLS_LEV_NONE:
	    return;
	case TLS_LEV_INVALID:
	    msg_fatal("Invalid TLS level \"%s\"", state->options.level);
	}
    }

    /*
     * We first call tls_init(), which ultimately calls SSL_library_init(),
     * since otherwise we can't tell whether we have the message digests
     * required for DANE support.
     */
    tls_init(state);
    if (state->level == TLS_LEV_DANE && !tls_dane_avail()) {
	msg_warn("The \"dane\" TLS security level is not available");
	state->level = TLS_LEV_SECURE;
    }
    state->tls_bio = 0;
    if (state->print_trust)
	state->tls_bio = BIO_new_fp(stdout, BIO_NOCLOSE);

#endif
}

/* parse_match - process match arguments */

static void parse_match(STATE *state, int argc, char *argv[])
{
#ifdef USE_TLS

    switch (state->level) {
	case TLS_LEV_SECURE:
	state->match = argv_alloc(2);
	while (*argv)
	    argv_split_append(state->match, *argv++, "");
	if (state->match->argc == 0)
	    argv_add(state->match, "nexthop", "dot-nexthop", ARGV_END);
	break;
    case TLS_LEV_VERIFY:
	state->match = argv_alloc(1);
	while (*argv)
	    argv_split_append(state->match, *argv++, "");
	if (state->match->argc == 0)
	    argv_add(state->match, "hostname", ARGV_END);
	break;
    case TLS_LEV_FPRINT:
	state->dane = tls_dane_alloc();
	while (*argv)
	    tls_dane_add_ee_digests((TLS_DANE *) state->dane,
				    state->mdalg, *argv++, "");
	break;
    case TLS_LEV_DANE:
	state->match = argv_alloc(2);
	argv_add(state->match, "nexthop", "hostname", ARGV_END);
	break;
    }
#endif
}

/* parse_tas - process '-A' trust anchor file option */

static void parse_tas(STATE *state)
{
#ifdef USE_TLS
    char  **file;

    if (!state->options.tas->argc)
	return;

    switch (state->level) {
    default:
	return;
    case TLS_LEV_SECURE:
    case TLS_LEV_VERIFY:
	state->dane = tls_dane_alloc();
	for (file = state->options.tas->argv; *file; ++file) {
	    if (!tls_dane_load_trustfile((TLS_DANE *) state->dane, *file))
		break;
	}
	if (*file)
	    msg_fatal("Failed to load trust anchor file: %s", *file);
	break;
    }
#endif
}


int     main(int argc, char *argv[])
{
    static STATE state;
    char   *loopenv = getenv("VALGRINDLOOP");
    int     loop = loopenv ? atoi(loopenv) : 1;

    /* Don't die when a peer goes away unexpectedly. */
    signal(SIGPIPE, SIG_IGN);

    /* We're a diagnostic utility, so diagnostic messages go to stdout. */
    var_procname = mystrdup(basename(argv[0]));
    set_mail_conf_str(VAR_PROCNAME, var_procname);
    msg_vstream_init(var_procname, VSTREAM_OUT);

    /*
     * Load main.cf, parse command-line options, then process main.cf
     * settings plus any command-line "-o" overrides.
     */
    mail_conf_suck();
    parse_options(&state, argc, argv);
    mail_params_init();
    parse_tas(&state);

    argc -= optind;
    argv += optind;

    /* The first non-option argument is the destination. */
    if (!argc)
	usage();

    state.dest = mystrdup(argv[0]);
    parse_match(&state, --argc, ++argv);

    /* Don't talk to remote systems as root */
    if (!geteuid())
	chroot_uid(0, var_mail_owner);

    while (loop-- > 0)
	run(&state);

    /* Be valgrind friendly and clean-up */
    cleanup(&state);
#ifdef USE_TLS
    ssl_cleanup();
#endif

    return (0);
}
