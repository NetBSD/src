/*	$NetBSD: tls_misc.c,v 1.1.1.4 2007/05/19 16:28:31 heas Exp $	*/

/*++
/* NAME
/*	tls_misc 3
/* SUMMARY
/*	miscellaneous TLS support routines
/* SYNOPSIS
/*	#define TLS_INTERNAL
/*	#include <tls.h>
/*
/*	TLScontext_t *tls_alloc_context(log_level, peername)
/*	int	log_level;
/*	const char *peername;
/*
/*	void	tls_free_context(TLScontext)
/*	TLScontext_t *TLScontext;
/*
/*	void	tls_check_version()
/*
/*	long	tls_bug_bits()
/*
/*	const char *tls_set_cipher_list(ssl_ctx, cipher_list)
/*	SSL_CTX *ssl_ctx;
/*	char	*cipher_list;
/*
/*	const char *tls_cipher_list(cipher_level, ...)
/*	int	cipher_level;
/*
/*	void	tls_print_errors()
/*
/*	void    tls_info_callback(ssl, where, ret)
/*	const SSL *ssl; /* unused */
/*	int	where;
/*	int	ret;
/*
/*	long	tls_bio_dump_cb(bio, cmd, argp, argi, argl, ret)
/*	BIO	*bio;
/*	int	cmd;
/*	const char *argp;
/*	int	argi;
/*	long	argl; /* unused */
/*	long	ret;
/* DESCRIPTION
/*	This module implements routines that support the TLS client
/*	and server internals.
/*
/*	tls_alloc_context() creates an initialized TLScontext
/*	structure with the specified peer name and logging level.
/*
/*	tls_free_context() destroys a TLScontext structure
/*	together with OpenSSL structures that are attached to it.
/*
/*	tls_check_version() logs a warning when the run-time OpenSSL
/*	library differs in its major, minor or micro number from
/*	the compile-time OpenSSL headers.
/*
/*	tls_bug_bits() returns the bug compatibility mask appropriate
/*	for the run-time library. Some of the bug work-arounds are
/*	not appropriate for some library versions.
/*
/*	tls_set_cipher_list() updates the cipher list of the specified SSL
/*	context. Returns the new cipherlist on success, otherwise logs a
/*	suitable warning and returns 0. The storage for the return value
/*	is overwritted with each call.
/*
/*	tls_cipher_list() generates a cipher list from the specified
/*	grade, minus any ciphers specified via a null-terminated
/*	list of string-valued exclusions. The result is overwritten
/*	upon each call.
/*
/*	tls_print_errors() queries the OpenSSL error stack,
/*	logs the error messages, and clears the error stack.
/*
/*	tls_info_callback() is a call-back routine for the
/*	SSL_CTX_set_info_callback() routine. It logs SSL events
/*	to the Postfix logfile.
/*
/*	tls_bio_dump_cb() is a call-back routine for the
/*	BIO_set_callback() routine. It logs SSL content to the
/*	Postfix logfile.
/* LICENSE
/* .ad
/* .fi
/*	This software is free. You can do with it whatever you want.
/*	The original author kindly requests that you acknowledge
/*	the use of his software.
/* AUTHOR(S)
/*	Originally written by:
/*	Lutz Jaenicke
/*	BTU Cottbus
/*	Allgemeine Elektrotechnik
/*	Universitaetsplatz 3-4
/*	D-03044 Cottbus, Germany
/*
/*	Updated by:
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <ctype.h>
#include <string.h>

#ifdef USE_TLS

/* Utility library. */

#include <vstream.h>
#include <msg.h>
#include <mymalloc.h>
#include <vstring.h>
#include <stringops.h>
#include <argv.h>

/* TLS library. */

#define TLS_INTERNAL
#include <tls.h>

/* Application-specific. */

 /*
  * Index to attach TLScontext pointers to SSL objects, so that they can be
  * accessed by call-back routines.
  */
int     TLScontext_index = -1;

 /*
  * Index to attach session cache names SSL_CTX objects.
  */
int     TLSscache_index = -1;

 /*
  * Protocol name <=> mask conversion.
  */
NAME_MASK tls_protocol_table[] = {
    SSL_TXT_SSLV2, TLS_PROTOCOL_SSLv2,
    SSL_TXT_SSLV3, TLS_PROTOCOL_SSLv3,
    SSL_TXT_TLSV1, TLS_PROTOCOL_TLSv1,
    0, 0,
};

char   *var_tls_high_clist;
char   *var_tls_medium_clist;
char   *var_tls_low_clist;
char   *var_tls_export_clist;
char   *var_tls_null_clist;

 /*
  * Ciphersuite name <=> code conversion.
  */
NAME_CODE tls_cipher_level_table[] = {
    "high", TLS_CIPHER_HIGH,
    "medium", TLS_CIPHER_MEDIUM,
    "low", TLS_CIPHER_LOW,
    "export", TLS_CIPHER_EXPORT,
    "null", TLS_CIPHER_NULL,
    0, TLS_CIPHER_NONE,
};

 /*
  * Parsed OpenSSL version number.
  */
typedef struct {
    int     major;
    int     minor;
    int     micro;
    int     patch;
    int     status;
} TLS_VINFO;

 /*
  * OpenSSL adopted the cipher selection patch, so we don't expect any more
  * broken ciphers other than AES and CAMELLIA.
  */
typedef struct {
    char   *ssl_name;
    int     alg_bits;
    char   *evp_name;
}       cipher_probe_t;

static cipher_probe_t cipher_probes[] = {
    "AES", 256, "AES-256-CBC",
    "CAMELLIA", 256, "CAMELLIA-256-CBC",
    0, 0, 0,
};

/* tls_exclude_missing - Append exclusions for missing ciphers */

static void tls_exclude_missing(SSL_CTX *ctx, VSTRING *buf)
{
    const char *myname = "tls_exclude_missing";
    static ARGV *exclude;		/* Cached */
    SSL    *s = 0;

    STACK_OF(SSL_CIPHER) * ciphers;
    SSL_CIPHER *c;
    cipher_probe_t *probe;
    int     alg_bits;
    int     num;
    int     i;

    /*
     * Process a list of probes which specify:
     * 
     * An SSL cipher-suite name for a family of ciphers that use the same
     * symmetric algorithm at two or more key sizes, typically 128/256 bits.
     * 
     * The key size (typically 256) that OpenSSL fails check, and assumes is
     * available when another key size (typically 128) is usable.
     * 
     * The OpenSSL name of the symmetric algorithm associated with the SSL
     * cipher-suite. Typically, this is MUMBLE-256-CBC, where "MUMBLE" is the
     * name of the SSL cipher-suite that use the MUMBLE symmetric algorithm.
     * On systems that support the required encryption algorithm, the name is
     * listed in the output of "openssl list-cipher-algorithms".
     * 
     * When an encryption algorithm is not available at the given key size but
     * the corresponding OpenSSL cipher-suite contains ciphers that have have
     * this key size, the problem ciphers are explicitly disabled in Postfix.
     * The list is cached in the static "exclude" array.
     */
    if (exclude == 0) {
	exclude = argv_alloc(1);

	/*
	 * Iterate over the probe list
	 */
	for (probe = cipher_probes; probe->ssl_name; ++probe) {
	    /* No exclusions if evp_name is a valid algorithm */
	    if (EVP_get_cipherbyname(probe->evp_name))
		continue;

	    /*
	     * Sadly there is no SSL_CTX_get_ciphers() interface, so we are
	     * forced to allocate and free an SSL object. Fatal error if we
	     * can't allocate the SSL object.
	     */
	    ERR_clear_error();
	    if (s == 0 && (s = SSL_new(ctx)) == 0) {
		tls_print_errors();
		msg_fatal("%s: error allocating SSL object", myname);
	    }

	    /*
	     * Cipher is not supported by libcrypto, nothing to do if also
	     * not supported by libssl. Flush the OpenSSL error stack.
	     * 
	     * XXX: There may be additional places in pre-existing code where
	     * SSL errors are generated and ignored, that require a similar
	     * "flush". Better yet, is to always flush before calls that run
	     * tls_print_errors() on failure.
	     * 
	     * Contrary to documentation, on SunOS 5.10 SSL_set_cipher_list()
	     * returns success with no ciphers selected, when this happens
	     * SSL_get_ciphers() produces a stack with 0 elements!
	     */
	    if (SSL_set_cipher_list(s, probe->ssl_name) == 0
		|| (ciphers = SSL_get_ciphers(s)) == 0
		|| (num = sk_SSL_CIPHER_num(ciphers)) == 0) {
		ERR_clear_error();		/* flush any generated errors */
		continue;
	    }
	    for (i = 0; i < num; ++i) {
		c = sk_SSL_CIPHER_value(ciphers, i);
		(void) SSL_CIPHER_get_bits(c, &alg_bits);
		if (alg_bits == probe->alg_bits)
		    argv_add(exclude, SSL_CIPHER_get_name(c), ARGV_END);
	    }
	}
	if (s != 0)
	    SSL_free(s);
    }
    for (i = 0; i < exclude->argc; ++i)
	vstring_sprintf_append(buf, ":!%s", exclude->argv[i]);
}

/* tls_set_cipher_list - Set SSL_CTX cipher list */

const char *tls_set_cipher_list(SSL_CTX *ssl_ctx, const char *spec)
{
    static VSTRING *buf;
    const char *ex_spec;

    if (buf == 0)
	buf = vstring_alloc(10);

    vstring_strcpy(buf, spec);
    tls_exclude_missing(ssl_ctx, buf);
    ex_spec = vstring_str(buf);

    ERR_clear_error();
    if (SSL_CTX_set_cipher_list(ssl_ctx, ex_spec) != 0)
	return (ex_spec);

    tls_print_errors();
    return (0);
}

/* tls_cipher_list - Cipherlist for given grade, less exclusions */

const char *tls_cipher_list(int cipher_level,...)
{
    const char *myname = "tls_cipher_list";
    static VSTRING *buf;
    va_list ap;
    const char *exclude;
    char   *tok;
    char   *save;
    char   *cp;

    buf = buf ? buf : vstring_alloc(10);
    VSTRING_RESET(buf);

    switch (cipher_level) {
    case TLS_CIPHER_HIGH:
	vstring_strcpy(buf, var_tls_high_clist);
	break;
    case TLS_CIPHER_MEDIUM:
	vstring_strcpy(buf, var_tls_medium_clist);
	break;
    case TLS_CIPHER_LOW:
	vstring_strcpy(buf, var_tls_low_clist);
	break;
    case TLS_CIPHER_EXPORT:
	vstring_strcpy(buf, var_tls_export_clist);
	break;
    case TLS_CIPHER_NULL:
	vstring_strcpy(buf, var_tls_null_clist);
	break;
    case TLS_CIPHER_NONE:
	return 0;
    default:

	/*
	 * The caller MUST provide a valid cipher grade
	 */
	msg_panic("%s: invalid cipher grade: %d", myname, cipher_level);
    }

    /*
     * The base lists for each grade can't be empty.
     */
    if (VSTRING_LEN(buf) == 0)
	msg_panic("%s: empty cipherlist", myname);

    va_start(ap, cipher_level);
    while ((exclude = va_arg(ap, char *)) != 0) {
	if (*exclude == '\0')
	    continue;
	save = cp = mystrdup(exclude);
	while ((tok = mystrtok(&cp, "\t\n\r ,:")) != 0) {

	    /*
	     * Can't exclude ciphers that start with modifiers.
	     */
	    if (strchr("!+-@", *tok)) {
		msg_warn("%s: can't exclude '!+-@' modifiers, '%s' ignored",
			 myname, tok);
		continue;
	    }
	    vstring_sprintf_append(buf, ":!%s", tok);
	}
	myfree(save);
    }
    va_end(ap);

    return (vstring_str(buf));
}


/* tls_alloc_context - allocate TLScontext */

TLScontext_t *tls_alloc_context(int log_level, const char *peername)
{
    TLScontext_t *TLScontext;

    /*
     * PORTABILITY: Do not assume that null pointers are all-zero bits. Use
     * explicit assignments to initialize pointers.
     * 
     * See the C language FAQ item 5.17, or if you have time to burn,
     * http://www.google.com/search?q=zero+bit+null+pointer
     * 
     * However, it's OK to use memset() to zero integer values.
     */
    TLScontext = (TLScontext_t *) mymalloc(sizeof(TLScontext_t));
    memset((char *) TLScontext, 0, sizeof(*TLScontext));
    TLScontext->con = 0;
    TLScontext->internal_bio = 0;
    TLScontext->network_bio = 0;
    TLScontext->serverid = 0;
    TLScontext->peer_CN = 0;
    TLScontext->issuer_CN = 0;
    TLScontext->peer_fingerprint = 0;
    TLScontext->protocol = 0;
    TLScontext->cipher_name = 0;
    TLScontext->log_level = log_level;
    TLScontext->peername = lowercase(mystrdup(peername));

    return (TLScontext);
}

/* tls_free_context - deallocate TLScontext and members */

void    tls_free_context(TLScontext_t *TLScontext)
{

    /*
     * Free the SSL structure and the BIOs. Warning: the internal_bio is
     * connected to the SSL structure and is automatically freed with it. Do
     * not free it again (core dump)!! Only free the network_bio.
     */
    if (TLScontext->con != 0)
	SSL_free(TLScontext->con);
    if (TLScontext->network_bio)
	BIO_free(TLScontext->network_bio);

    if (TLScontext->peername)
	myfree(TLScontext->peername);
    if (TLScontext->serverid)
	myfree(TLScontext->serverid);

    if (TLScontext->peer_CN)
	myfree(TLScontext->peer_CN);
    if (TLScontext->issuer_CN)
	myfree(TLScontext->issuer_CN);
    if (TLScontext->peer_fingerprint)
	myfree(TLScontext->peer_fingerprint);

    myfree((char *) TLScontext);
}

/* tls_version_split - Split OpenSSL version number into major, minor, ... */

static void tls_version_split(long version, TLS_VINFO *info)
{

    /*
     * OPENSSL_VERSION_NUMBER(3):
     * 
     * OPENSSL_VERSION_NUMBER is a numeric release version identifier:
     * 
     * MMNNFFPPS: major minor fix patch status
     * 
     * The status nibble has one of the values 0 for development, 1 to e for
     * betas 1 to 14, and f for release. Parsed OpenSSL version number. for
     * example
     * 
     * 0x000906000 == 0.9.6 dev 0x000906023 == 0.9.6b beta 3 0x00090605f ==
     * 0.9.6e release
     * 
     * Versions prior to 0.9.3 have identifiers < 0x0930.  Versions between
     * 0.9.3 and 0.9.5 had a version identifier with this interpretation:
     * 
     * MMNNFFRBB major minor fix final beta/patch
     * 
     * for example
     * 
     * 0x000904100 == 0.9.4 release 0x000905000 == 0.9.5 dev
     * 
     * Version 0.9.5a had an interim interpretation that is like the current
     * one, except the patch level got the highest bit set, to keep continu-
     * ity.  The number was therefore 0x0090581f.
     */

    if (version < 0x0930) {
	info->status = 0;
	info->patch = version & 0x0f;
	version >>= 4;
	info->micro = version & 0x0f;
	version >>= 4;
	info->minor = version & 0x0f;
	version >>= 4;
	info->major = version & 0x0f;
    } else if (version < 0x00905800L) {
	info->patch = version & 0xff;
	version >>= 8;
	info->status = version & 0xf;
	version >>= 4;
	info->micro = version & 0xff;
	version >>= 8;
	info->minor = version & 0xff;
	version >>= 8;
	info->major = version & 0xff;
    } else {
	info->status = version & 0xf;
	version >>= 4;
	info->patch = version & 0xff;
	version >>= 8;
	info->micro = version & 0xff;
	version >>= 8;
	info->minor = version & 0xff;
	version >>= 8;
	info->major = version & 0xff;
	if (version < 0x00906000L)
	    info->patch &= ~0x80;
    }
}

/* tls_check_version - Detect mismatch between headers and library. */

void    tls_check_version(void)
{
    TLS_VINFO hdr_info;
    TLS_VINFO lib_info;

    tls_version_split(OPENSSL_VERSION_NUMBER, &hdr_info);
    tls_version_split(SSLeay(), &lib_info);

    if (lib_info.major != hdr_info.major
	|| lib_info.minor != hdr_info.minor
	|| lib_info.micro != hdr_info.micro)
	msg_warn("run-time library vs. compile-time header version mismatch: "
	     "OpenSSL %d.%d.%d may not be compatible with OpenSSL %d.%d.%d",
		 lib_info.major, lib_info.minor, lib_info.micro,
		 hdr_info.major, hdr_info.minor, hdr_info.micro);
}

/* tls_bug_bits - SSL bug compatibility bits for this OpenSSL version */

long    tls_bug_bits(void)
{
    long    bits = SSL_OP_ALL;		/* Work around all known bugs */

#if OPENSSL_VERSION_NUMBER >= 0x00908000L
    long    lib_version = SSLeay();

    /*
     * In OpenSSL 0.9.8[ab], enabling zlib compression breaks the padding bug
     * work-around, leading to false positives and failed connections. We may
     * not interoperate with systems with the bug, but this is better than
     * breaking on all 0.9.8[ab] systems that have zlib support enabled.
     */
    if (lib_version >= 0x00908000L && lib_version <= 0x0090802fL) {
	STACK_OF(SSL_COMP) * comp_methods;

	comp_methods = SSL_COMP_get_compression_methods();
	if (comp_methods != 0 && sk_SSL_COMP_num(comp_methods) > 0)
	    bits &= ~SSL_OP_TLS_BLOCK_PADDING_BUG;
    }
#endif
    return (bits);
}

/* tls_print_errors - print and clear the error stack */

void    tls_print_errors(void)
{
    unsigned long err;
    char    buffer[1024];		/* XXX */
    const char *file;
    const char *data;
    int     line;
    int     flags;
    unsigned long thread;

    thread = CRYPTO_thread_id();
    while ((err = ERR_get_error_line_data(&file, &line, &data, &flags)) != 0) {
	ERR_error_string_n(err, buffer, sizeof(buffer));
	if (flags & ERR_TXT_STRING)
	    msg_warn("TLS library problem: %lu:%s:%s:%d:%s:",
		     thread, buffer, file, line, data);
	else
	    msg_warn("TLS library problem: %lu:%s:%s:%d:",
		     thread, buffer, file, line);
    }
}

/* tls_info_callback - callback for logging SSL events via Postfix */

void    tls_info_callback(const SSL *s, int where, int ret)
{
    char   *str;
    int     w;

    /* Adapted from OpenSSL apps/s_cb.c. */

    w = where & ~SSL_ST_MASK;

    if (w & SSL_ST_CONNECT)
	str = "SSL_connect";
    else if (w & SSL_ST_ACCEPT)
	str = "SSL_accept";
    else
	str = "unknown";

    if (where & SSL_CB_LOOP) {
	msg_info("%s:%s", str, SSL_state_string_long((SSL *) s));
    } else if (where & SSL_CB_ALERT) {
	str = (where & SSL_CB_READ) ? "read" : "write";
	if ((ret & 0xff) != SSL3_AD_CLOSE_NOTIFY)
	    msg_info("SSL3 alert %s:%s:%s", str,
		     SSL_alert_type_string_long(ret),
		     SSL_alert_desc_string_long(ret));
    } else if (where & SSL_CB_EXIT) {
	if (ret == 0)
	    msg_info("%s:failed in %s",
		     str, SSL_state_string_long((SSL *) s));
	else if (ret < 0) {
	    msg_info("%s:error in %s",
		     str, SSL_state_string_long((SSL *) s));
	}
    }
}

 /*
  * taken from OpenSSL crypto/bio/b_dump.c.
  * 
  * Modified to save a lot of strcpy and strcat by Matti Aarnio.
  * 
  * Rewritten by Wietse to elimate fixed-size stack buffer, array index
  * multiplication and division, sprintf() and strcpy(), and lots of strlen()
  * calls. We could make it a little faster by using a fixed-size stack-based
  * buffer.
  * 
  * 200412 - use %lx to print pointers, after casting them to unsigned long.
  */

#define TRUNCATE_SPACE_NULL
#define DUMP_WIDTH	16
#define VERT_SPLIT	7

static void tls_dump_buffer(const unsigned char *start, int len)
{
    VSTRING *buf = vstring_alloc(100);
    const unsigned char *last = start + len - 1;
    const unsigned char *row;
    const unsigned char *col;
    int     ch;

#ifdef TRUNCATE_SPACE_NULL
    while (last >= start && (*last == ' ' || *last == 0))
	last--;
#endif

    for (row = start; row <= last; row += DUMP_WIDTH) {
	VSTRING_RESET(buf);
	vstring_sprintf(buf, "%04lx ", (unsigned long) (row - start));
	for (col = row; col < row + DUMP_WIDTH; col++) {
	    if (col > last) {
		vstring_strcat(buf, "   ");
	    } else {
		ch = *col;
		vstring_sprintf_append(buf, "%02x%c",
				   ch, col - row == VERT_SPLIT ? '|' : ' ');
	    }
	}
	VSTRING_ADDCH(buf, ' ');
	for (col = row; col < row + DUMP_WIDTH; col++) {
	    if (col > last)
		break;
	    ch = *col;
	    if (!ISPRINT(ch))
		ch = '.';
	    VSTRING_ADDCH(buf, ch);
	    if (col - row == VERT_SPLIT)
		VSTRING_ADDCH(buf, ' ');
	}
	VSTRING_TERMINATE(buf);
	msg_info("%s", vstring_str(buf));
    }
#ifdef TRUNCATE_SPACE_NULL
    if ((last + 1) - start < len)
	msg_info("%04lx - <SPACES/NULLS>",
		 (unsigned long) ((last + 1) - start));
#endif
    vstring_free(buf);
}

/* taken from OpenSSL apps/s_cb.c */

long    tls_bio_dump_cb(BIO *bio, int cmd, const char *argp, int argi,
			        long unused_argl, long ret)
{
    if (cmd == (BIO_CB_READ | BIO_CB_RETURN)) {
	msg_info("read from %08lX [%08lX] (%d bytes => %ld (0x%lX))",
		 (unsigned long) bio, (unsigned long) argp, argi,
		 ret, (unsigned long) ret);
	tls_dump_buffer((unsigned char *) argp, (int) ret);
    } else if (cmd == (BIO_CB_WRITE | BIO_CB_RETURN)) {
	msg_info("write to %08lX [%08lX] (%d bytes => %ld (0x%lX))",
		 (unsigned long) bio, (unsigned long) argp, argi,
		 ret, (unsigned long) ret);
	tls_dump_buffer((unsigned char *) argp, (int) ret);
    }
    return (ret);
}

#else

 /*
  * Broken linker workaround.
  */
int     tls_dummy_for_broken_linkers;

#endif
