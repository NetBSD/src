/*++
/* NAME
/*	smtp 3h
/* SUMMARY
/*	smtp client program
/* SYNOPSIS
/*	#include "smtp.h"
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
  * State information associated with each SMTP delivery. We're bundling the
  * state so that we can give meaningful diagnostics in case of problems.
  */
typedef struct SMTP_STATE {
    VSTREAM *src;			/* queue file stream */
    DELIVER_REQUEST *request;		/* envelope info, offsets */
    struct SMTP_SESSION *session;	/* network connection */
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
    off_t   size_limit;			/* server limit or unknown */
} SMTP_STATE;

#define SMTP_FEATURE_ESMTP	(1<<0)
#define SMTP_FEATURE_8BITMIME	(1<<1)
#define SMTP_FEATURE_PIPELINING	(1<<2)
#define SMTP_FEATURE_SIZE	(1<<3)
#define SMTP_FEATURE_STARTTLS	(1<<4)
#define SMTP_FEATURE_AUTH	(1<<5)
#define SMTP_FEATURE_MAYBEPIX	(1<<6)	/* PIX smtp fixup mode */

 /*
  * smtp.c
  */
extern int smtp_errno;			/* XXX can we get rid of this? */

 /*
  * smtp_session.c
  */
typedef struct SMTP_SESSION {
    VSTREAM *stream;			/* network connection */
    char   *host;			/* mail exchanger */
    char   *addr;			/* mail exchanger */
    char   *namaddr;			/* mail exchanger */
    int     best;			/* most preferred host */
} SMTP_SESSION;

extern SMTP_SESSION *smtp_session_alloc(VSTREAM *, char *, char *);
extern void smtp_session_free(SMTP_SESSION *);

 /*
  * smtp_connect.c
  */
extern SMTP_SESSION *smtp_connect(char *, VSTRING *);
extern SMTP_SESSION *smtp_connect_host(char *, unsigned, VSTRING *);
extern SMTP_SESSION *smtp_connect_domain(char *, unsigned, VSTRING *, int *);

 /*
  * smtp_proto.c
  */
extern int smtp_helo(SMTP_STATE *);
extern int smtp_xfer(SMTP_STATE *);
extern void smtp_quit(SMTP_STATE *);

 /*
  * smtp_chat.c
  */
typedef struct SMTP_RESP {		/* server response */
    int     code;			/* status */
    char   *str;			/* text */
    VSTRING *buf;			/* origin of text */
} SMTP_RESP;

extern void PRINTFLIKE(2, 3) smtp_chat_cmd(SMTP_STATE *, char *,...);
extern SMTP_RESP *smtp_chat_resp(SMTP_STATE *);
extern void smtp_chat_reset(SMTP_STATE *);
extern void smtp_chat_notify(SMTP_STATE *);

 /*
  * smtp_trouble.c
  */
extern int PRINTFLIKE(3, 4) smtp_conn_fail(SMTP_STATE *, int, char *,...);
extern int PRINTFLIKE(3, 4) smtp_site_fail(SMTP_STATE *, int, char *,...);
extern int PRINTFLIKE(3, 4) smtp_mesg_fail(SMTP_STATE *, int, char *,...);
extern void PRINTFLIKE(4, 5) smtp_rcpt_fail(SMTP_STATE *, int, RECIPIENT *, char *,...);
extern int smtp_stream_except(SMTP_STATE *, int, char *);

 /*
  * smtp_unalias.c
  */
extern const char *smtp_unalias_name(const char *);
extern VSTRING *smtp_unalias_addr(VSTRING *, const char *);

 /*
  * smtp_state.c
  */
extern SMTP_STATE *smtp_state_alloc(void);
extern void smtp_state_free(SMTP_STATE *);

 /*
  * Status codes. Errors must have negative codes so that they do not
  * interfere with useful counts of work done.
  */
#define SMTP_OK			0	/* so far, so good */
#define SMTP_RETRY		(-1)	/* transient error */
#define SMTP_FAIL		(-2)	/* hard error */

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
