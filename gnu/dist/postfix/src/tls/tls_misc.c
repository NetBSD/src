/*	$NetBSD: tls_misc.c,v 1.1.1.1 2005/08/18 21:11:06 rpaulo Exp $	*/

/*++
/* NAME
/*	tls_misc 3
/* SUMMARY
/*	miscellaneous TLS support routines
/* SYNOPSIS
/*	#define TLS_INTERNAL
/*	#include <tls.h>
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

#ifdef USE_TLS

/* Utility library. */

#include <vstream.h>
#include <msg.h>
#include <mymalloc.h>
#include <vstring.h>

/* TLS library. */

#define TLS_INTERNAL
#include <tls.h>

/* Application-specific. */

 /*
  * Indices to attach our own information to SSL and to SSL_SESSION objects,
  * so that it can be accessed by call-back routines.
  */
int     TLScontext_index = -1;

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

#endif
