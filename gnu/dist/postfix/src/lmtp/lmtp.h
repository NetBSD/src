/*++
/* NAME
/*	lmtp 3h
/* SUMMARY
/*	lmtp client program
/* SYNOPSIS
/*	#include "lmtp.h"
/* DESCRIPTION
/* .nf

 /*
  * SASL library.
  */
#ifdef USE_SASL_AUTH
#include <sasl.h>
#include <saslutil.h>
#endif

 /*
  * Utility library.
  */
#include <vstream.h>
#include <vstring.h>
#include <argv.h>

 /*
  * Global library.
  */
#include <deliver_request.h>

 /*
  * State information associated with each LMTP delivery. We're bundling the
  * state so that we can give meaningful diagnostics in case of problems.
  */
typedef struct LMTP_STATE {
    VSTREAM *src;			/* queue file stream */
    DELIVER_REQUEST *request;		/* envelope info, offsets */
    struct LMTP_SESSION *session;	/* network connection */
    VSTRING *buffer;			/* I/O buffer */
    VSTRING *scratch;			/* scratch buffer */
    VSTRING *scratch2;			/* scratch buffer */
    int     status;			/* delivery status */
    int     features;			/* server features */
    ARGV   *history;			/* transaction log */
    int     error_mask;			/* error classes */
#ifdef USE_SASL_AUTH
    char   *sasl_mechanism_list;	/* server mechanism list */
    char   *sasl_username;		/* client username */
    char   *sasl_passwd;		/* client password */
    sasl_conn_t *sasl_conn;		/* SASL internal state */
    VSTRING *sasl_encoded;		/* encoding buffer */
    VSTRING *sasl_decoded;		/* decoding buffer */
    sasl_callback_t *sasl_callbacks;	/* stateful callbacks */
#endif
    int     sndbufsize;			/* total window size */
    int     sndbuffree;			/* remaining window */
    int     reuse;			/* connection being reused */
} LMTP_STATE;

#define LMTP_FEATURE_ESMTP	(1<<0)
#define LMTP_FEATURE_8BITMIME	(1<<1)
#define LMTP_FEATURE_PIPELINING	(1<<2)
#define LMTP_FEATURE_SIZE	(1<<3)
#define SMTP_FEATURE_AUTH	(1<<5)

 /*
  * lmtp.c
  */
extern int lmtp_errno;			/* XXX can we get rid of this? */

 /*
  * lmtp_session.c
  */
typedef struct LMTP_SESSION {
    VSTREAM *stream;			/* network connection */
    char   *host;			/* mail exchanger, name */
    char   *addr;			/* mail exchanger, address */
    char   *namaddr;			/* mail exchanger, for logging */
    char   *dest;			/* remote endpoint name */
} LMTP_SESSION;

extern LMTP_SESSION *lmtp_session_alloc(VSTREAM *, const char *, const char *, const char *);
extern LMTP_SESSION *lmtp_session_free(LMTP_SESSION *);

 /*
  * lmtp_connect.c
  */
extern LMTP_SESSION *lmtp_connect(const char *, VSTRING *);

 /*
  * lmtp_proto.c
  */
extern int lmtp_lhlo(LMTP_STATE *);
extern int lmtp_xfer(LMTP_STATE *);
extern int lmtp_quit(LMTP_STATE *);
extern int lmtp_rset(LMTP_STATE *);

 /*
  * lmtp_chat.c
  */
typedef struct LMTP_RESP {		/* server response */
    int     code;			/* status */
    char   *str;			/* text */
    VSTRING *buf;			/* origin of text */
} LMTP_RESP;

extern void PRINTFLIKE(2, 3) lmtp_chat_cmd(LMTP_STATE *, char *,...);
extern LMTP_RESP *lmtp_chat_resp(LMTP_STATE *);
extern void lmtp_chat_reset(LMTP_STATE *);
extern void lmtp_chat_notify(LMTP_STATE *);

 /*
  * lmtp_trouble.c
  */
extern int PRINTFLIKE(3, 4) lmtp_conn_fail(LMTP_STATE *, int, char *,...);
extern int PRINTFLIKE(3, 4) lmtp_site_fail(LMTP_STATE *, int, char *,...);
extern int PRINTFLIKE(3, 4) lmtp_mesg_fail(LMTP_STATE *, int, char *,...);
extern void PRINTFLIKE(4, 5) lmtp_rcpt_fail(LMTP_STATE *, int, RECIPIENT *, char *,...);
extern int lmtp_stream_except(LMTP_STATE *, int, char *);

 /*
  * lmtp_state.c
  */
extern LMTP_STATE *lmtp_state_alloc(void);
extern void lmtp_state_free(LMTP_STATE *);

 /*
  * Status codes. Errors must have negative codes so that they do not
  * interfere with useful counts of work done.
  */
#define LMTP_OK			0	/* so far, so good */
#define LMTP_RETRY		(-1)	/* transient error */
#define LMTP_FAIL		(-2)	/* hard error */

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
/*      Alterations for LMTP by:
/*      Philip A. Prindeville
/*      Mirapoint, Inc.
/*      USA.
/*
/*      Additional work on LMTP by:
/*      Amos Gouaux
/*      University of Texas at Dallas
/*      P.O. Box 830688, MC34
/*      Richardson, TX 75083, USA
/*--*/
