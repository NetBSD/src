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
    int     space_left;			/* output length control */
    struct MIME_STATE *mime_state;	/* mime state machine */

    /*
     * Flags and counters to control the handling of mail delivery errors.
     * There is some redundancy for sanity checking. At the end of an SMTP
     * session all recipients should be marked one way or the other.
     */
    int     final_server;		/* final mail server */
    int     rcpt_left;			/* recipients left over */
    int     rcpt_drop;			/* recipients marked as drop */
    int     rcpt_keep;			/* recipients marked as keep */
} SMTP_STATE;

 /*
  * Server features.
  */
#define SMTP_FEATURE_ESMTP		(1<<0)
#define SMTP_FEATURE_8BITMIME		(1<<1)
#define SMTP_FEATURE_PIPELINING		(1<<2)
#define SMTP_FEATURE_SIZE		(1<<3)
#define SMTP_FEATURE_STARTTLS		(1<<4)
#define SMTP_FEATURE_AUTH		(1<<5)
#define SMTP_FEATURE_MAYBEPIX		(1<<6)	/* PIX smtp fixup mode */
#define SMTP_FEATURE_XFORWARD_NAME	(1<<7)
#define SMTP_FEATURE_XFORWARD_ADDR	(1<<8)
#define SMTP_FEATURE_XFORWARD_PROTO	(1<<9)
#define SMTP_FEATURE_XFORWARD_HELO	(1<<10)

 /*
  * Misc flags.
  */
#define SMTP_MISC_FLAG_LOOP_DETECT	(1<<0)

#define SMTP_MISC_FLAG_DEFAULT		SMTP_MISC_FLAG_LOOP_DETECT

 /*
  * smtp.c
  */
extern int smtp_errno;			/* XXX can we get rid of this? */

#define SMTP_ERR_NONE	0		/* no error */
#define SMTP_ERR_FAIL	1		/* permanent error */
#define SMTP_ERR_RETRY	2		/* temporary error */
#define SMTP_ERR_LOOP	3		/* mailer loop */

extern int smtp_host_lookup_mask;	/* host lookup methods to use */

#define SMTP_HOST_FLAG_DNS	(1<<0)
#define SMTP_HOST_FLAG_NATIVE	(1<<1)

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
extern int smtp_connect(SMTP_STATE *);

 /*
  * smtp_proto.c
  */
extern int smtp_helo(SMTP_STATE *, int);
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
  * These operations implement a redundant mark-and-sweep algorithm that
  * explicitly accounts for the fate of every recipient. The interface is
  * documented in smtp_rcpt.c, which also implements the sweeping. The
  * smtp_trouble.c module does most of the marking after failure.
  * 
  * When a delivery fails or succeeds, take one of the following actions:
  * 
  * - Mark the recipient as KEEP (deliver to alternate MTA) and do not update
  * the delivery request status.
  * 
  * - Mark the recipient as DROP (remove from delivery request), log whether
  * delivery succeeded or failed, delete the recipient from the queue file
  * and/or update defer or bounce logfiles, and update the delivery request
  * status.
  * 
  * At the end of a delivery attempt, all recipients must be marked one way or
  * the other. Failure to do so will trigger a panic.
  */
#define SMTP_RCPT_STATE_KEEP	1	/* send to backup host */
#define SMTP_RCPT_STATE_DROP	2	/* remove from request */
#define SMTP_RCPT_INIT(state) do { \
	    (state)->rcpt_drop = (state)->rcpt_keep = 0; \
	    (state)->rcpt_left = state->request->rcpt_list.len; \
	} while (0)

#define SMTP_RCPT_DROP(state, rcpt) do { \
	    (rcpt)->status = SMTP_RCPT_STATE_DROP; (state)->rcpt_drop++; \
	} while (0)

#define SMTP_RCPT_KEEP(state, rcpt) do { \
	    (rcpt)->status = SMTP_RCPT_STATE_KEEP; (state)->rcpt_keep++; \
	} while (0)

#define SMTP_RCPT_ISMARKED(rcpt) ((rcpt)->status != 0)

#define SMTP_RCPT_LEFT(state) (state)->rcpt_left

extern void smtp_rcpt_cleanup(SMTP_STATE *);
extern void smtp_rcpt_done(SMTP_STATE *, const char *, RECIPIENT *);

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
