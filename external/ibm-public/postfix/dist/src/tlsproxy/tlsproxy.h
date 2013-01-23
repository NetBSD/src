/*	$NetBSD: tlsproxy.h,v 1.1.1.1.6.1 2013/01/23 00:05:15 yamt Exp $	*/

/*++
/* NAME
/*	tlsproxy 3h
/* SUMMARY
/*	tlsproxy internal interfaces
/* SYNOPSIS
/*	#include <tlsproxy.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstream.h>
#include <nbbio.h>

 /*
  * TLS library.
  */
#include <tls.h>

 /*
  * Internal interface.
  */
typedef struct {
    int     flags;			/* see below */
    int     req_flags;			/* request flags, see tls_proxy.h */
    char   *service;			/* argv[0] */
    VSTREAM *plaintext_stream;		/* local peer: postscreen(8), etc. */
    NBBIO  *plaintext_buf;		/* plaintext buffer */
    int     ciphertext_fd;		/* remote peer */
    EVENT_NOTIFY_FN ciphertext_timer;	/* kludge */
    int     timeout;			/* read/write time limit */
    char   *remote_endpt;		/* printable remote endpoint */
    char   *server_id;			/* cache management */
    TLS_SESS_STATE *tls_context;	/* llibtls state */
    int     ssl_last_err;		/* TLS I/O state */
} TLSP_STATE;

#define TLSP_FLAG_DO_HANDSHAKE	(1<<0)

extern TLSP_STATE *tlsp_state_create(const char *, VSTREAM *);
extern void tlsp_state_free(TLSP_STATE *);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/
