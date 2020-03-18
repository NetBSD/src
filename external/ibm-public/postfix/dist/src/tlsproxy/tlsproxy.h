/*	$NetBSD: tlsproxy.h,v 1.2 2020/03/18 19:05:21 christos Exp $	*/

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
    int     is_server_role;		/* avoid clumsy handler code */
    char   *service;			/* argv[0] */
    VSTREAM *plaintext_stream;		/* local peer: postscreen(8), etc. */
    NBBIO  *plaintext_buf;		/* plaintext buffer */
    int     ciphertext_fd;		/* remote peer */
    EVENT_NOTIFY_FN ciphertext_timer;	/* kludge */
    int     timeout;			/* read/write time limit */
    int     handshake_timeout;		/* in-handshake time limit */
    int     session_timeout;		/* post-handshake time limit */
    char   *remote_endpt;		/* printable remote endpoint */
    char   *server_id;			/* cache management */
    TLS_APPL_STATE *appl_state;		/* libtls state */
    TLS_SESS_STATE *tls_context;	/* libtls state */
    int     ssl_last_err;		/* TLS I/O state */
    TLS_CLIENT_PARAMS *tls_params;	/* globals not part of init_props */
    TLS_SERVER_INIT_PROPS *server_init_props;
    TLS_SERVER_START_PROPS *server_start_props;
    TLS_CLIENT_INIT_PROPS *client_init_props;
    TLS_CLIENT_START_PROPS *client_start_props;
} TLSP_STATE;

#define TLSP_FLAG_DO_HANDSHAKE	(1<<0)
#define TLSP_FLAG_NO_MORE_CIPHERTEXT_IO (1<<1)	/* overrides DO_HANDSHAKE */

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
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/
