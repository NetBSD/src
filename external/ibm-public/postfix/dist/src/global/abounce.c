/*	$NetBSD: abounce.c,v 1.3 2022/10/08 16:12:45 christos Exp $	*/

/*++
/* NAME
/*	abounce 3
/* SUMMARY
/*	asynchronous bounce/defer/trace service client
/* SYNOPSIS
/*	#include <abounce.h>
/*
/*	void	abounce_flush(flags, queue, id, encoding, smtputf8, sender,
/*				dsn_envid, dsn_ret, callback, context)
/*	int	flags;
/*	const char *queue;
/*	const char *id;
/*	const char *encoding;
/*	int	smtputf8;
/*	const char *sender;
/*	const char *dsn_envid;
/*	int	dsn_ret;
/*	void	(*callback)(int status, void *context);
/*	void	*context;
/*
/*	void	abounce_flush_verp(flags, queue, id, encoding, smtputf8, sender,
/*				dsn_envid, dsn_ret, verp, callback, context)
/*	int	flags;
/*	const char *queue;
/*	const char *id;
/*	const char *encoding;
/*	int	smtputf8;
/*	const char *sender;
/*	const char *dsn_envid;
/*	int	dsn_ret;
/*	const char *verp;
/*	void	(*callback)(int status, void *context);
/*	void	*context;
/*
/*	void	adefer_flush(flags, queue, id, encoding, smtputf8, sender,
/*				dsn_envid, dsn_ret, callback, context)
/*	int	flags;
/*	const char *queue;
/*	const char *id;
/*	const char *encoding;
/*	int	smtputf8;
/*	const char *sender;
/*	const char *dsn_envid;
/*	int	dsn_ret;
/*	void	(*callback)(int status, void *context);
/*	void	*context;
/*
/*	void	adefer_flush_verp(flags, queue, id, encoding, smtputf8, sender,
/*				dsn_envid, dsn_ret, verp, callback, context)
/*	int	flags;
/*	const char *queue;
/*	const char *id;
/*	const char *encoding;
/*	int	smtputf8;
/*	const char *sender;
/*	const char *dsn_envid;
/*	int	dsn_ret;
/*	const char *verp;
/*	void	(*callback)(int status, void *context);
/*	void	*context;
/*
/*	void	adefer_warn(flags, queue, id, encoding, smtputf8, sender,
/*				dsn_envid, dsn_ret, callback, context)
/*	int	flags;
/*	const char *queue;
/*	const char *id;
/*	const char *encoding;
/*	int	smtputf8;
/*	const char *sender;
/*	const char *dsn_envid;
/*	int	dsn_ret;
/*	void	(*callback)(int status, void *context);
/*	void	*context;
/*
/*	void	atrace_flush(flags, queue, id, encoding, smtputf8, sender,
/*				dsn_envid, dsn_ret, callback, context)
/*	int	flags;
/*	const char *queue;
/*	const char *id;
/*	const char *encoding;
/*	int	smtputf8;
/*	const char *sender;
/*	const char *dsn_envid;
/*	int	dsn_ret;
/*	void	(*callback)(int status, void *context);
/*	void	*context;
/* DESCRIPTION
/*	This module implements an asynchronous interface to the
/*	bounce/defer/trace service for submitting sender notifications
/*	without waiting for completion of the request.
/*
/*	abounce_flush() bounces the specified message to
/*	the specified sender, including the bounce log that was
/*	built with bounce_append().
/*
/*	abounce_flush_verp() is like abounce_flush() but sends
/*	one VERP style notification per undeliverable recipient.
/*
/*	adefer_flush() bounces the specified message to
/*	the specified sender, including the defer log that was
/*	built with defer_append().
/*	adefer_flush() requests that the deferred recipients are deleted
/*	from the original queue file.
/*
/*	adefer_flush_verp() is like adefer_flush() but sends
/*	one VERP style notification per undeliverable recipient.
/*
/*	adefer_warn() sends a "mail is delayed" notification to
/*	the specified sender, including the defer log that was
/*	built with defer_append().
/*
/*	atrace_flush() returns the specified message to the specified
/*	sender, including the message delivery record log that was
/*	built with vtrace_append().
/*
/*	Arguments:
/* .IP flags
/*	The bitwise OR of zero or more of the following (specify
/*	BOUNCE_FLAG_NONE to request no special processing):
/* .RS
/* .IP BOUNCE_FLAG_CLEAN
/*	Delete the bounce log in case of an error (as in: pretend
/*	that we never even tried to bounce this message).
/* .IP BOUNCE_FLAG_DELRCPT
/*	When specified with a flush operation, request that
/*	recipients be deleted from the queue file.
/*
/*	Note: the bounce daemon ignores this request when the
/*	recipient queue file offset is <= 0.
/* .IP BOUNCE_FLAG_COPY
/*	Request that a postmaster copy is sent.
/* .RE
/* .IP queue
/*	The message queue name of the original message file.
/* .IP id
/*	The message queue id if the original message file. The bounce log
/*	file has the same name as the original message file.
/* .IP encoding
/*	The body content encoding: MAIL_ATTR_ENC_{7BIT,8BIT,NONE}.
/* .IP smtputf8
/*	The level of SMTPUTF8 support (to be defined).
/* .IP sender
/*	The sender envelope address.
/* .IP dsn_envid
/*	Optional DSN envelope ID.
/* .IP ret
/*	Optional DSN return full/headers option.
/* .IP verp
/*	VERP delimiter characters.
/* .IP callback
/*	Name of a routine that receives the notification status as
/*	documented for bounce_flush() or defer_flush().
/* .IP context
/*	Application-specific context that is passed through to the
/*	callback routine. Use proper casts or the world will come
/*	to an end.
/* DIAGNOSTICS
/*	In case of success, these functions log the action, and return a
/*	zero result via the callback routine. Otherwise, the functions
/*	return a non-zero result via the callback routine, and when
/*	BOUNCE_FLAG_CLEAN is disabled, log that message delivery is deferred.
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

/* System library. */

#include <sys_defs.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <events.h>
#include <vstream.h>

/* Global library. */

#include <mail_params.h>
#include <mail_proto.h>
#include <abounce.h>

/* Application-specific. */

 /*
  * Each bounce/defer flush/warn request is implemented by sending the
  * request to the bounce/defer server, and by creating a pseudo thread that
  * suspends itself until the server replies (or dies). Upon wakeup, the
  * pseudo thread delivers the request completion status to the application
  * and destroys itself. The structure below maintains all the necessary
  * request state while the pseudo thread is suspended.
  */
typedef struct {
    int     command;			/* bounce request type */
    int     flags;			/* bounce options */
    char   *id;				/* queue ID for logging */
    VSTRING *request;			/* serialized request */
    ABOUNCE_FN callback;		/* application callback */
    void   *context;			/* application context */
    VSTREAM *fp;			/* server I/O handle */
} ABOUNCE_STATE;

 /*
  * Encapsulate common code.
  */
#define ABOUNCE_EVENT_ENABLE(fd, callback, context, timeout) do { \
	event_enable_read((fd), (callback), (context)); \
	event_request_timer((callback), (context), (timeout)); \
    } while (0)

 /*
  * If we set the reply timeout too short, then we make the problem worse by
  * increasing overload. With 1000s timeout mail will keep flowing, but there
  * will be a large number of blocked bounce processes, and some resource is
  * likely to run out.
  */
#define ABOUNCE_TIMEOUT	1000

 /*
  * The initial buffer size for a serialized request.
  */
#define ABOUNCE_BUFSIZE	VSTREAM_BUFSIZE

 /*
  * We share most of the verp and non-verp code paths.
  */
#define ABOUNCE_NO_VERP	((char *) 0)

 /*
  * SLMs.
  */
#define STR(x) vstring_str(x)
#define LEN(x) VSTRING_LEN(x)

/* abounce_done - deliver status to application and clean up pseudo thread */

static void abounce_done(ABOUNCE_STATE *ap, int status)
{
    if (ap->fp) {
	event_disable_readwrite(vstream_fileno(ap->fp));
	(void) vstream_fclose(ap->fp);
    }
    if (status != 0 && (ap->flags & BOUNCE_FLAG_CLEAN) == 0)
	msg_info("%s: status=deferred (%s failed)", ap->id,
		 ap->command == BOUNCE_CMD_FLUSH ? "bounce" :
		 ap->command == BOUNCE_CMD_WARN ? "delay warning" :
		 ap->command == BOUNCE_CMD_VERP ? "verp" :
		 ap->command == BOUNCE_CMD_TRACE ? "trace" :
		 "whatever");
    ap->callback(status, ap->context);
    myfree(ap->id);
    vstring_free(ap->request);
    myfree((void *) ap);
}

/* abounce_receive - receive server reply */

static void abounce_receive(int event, void *context)
{
    ABOUNCE_STATE *ap = (ABOUNCE_STATE *) context;
    int     status;

    if (event != EVENT_TIME)
	event_cancel_timer(abounce_receive, context);

    if (event == EVENT_READ
	&& attr_scan(ap->fp, ATTR_FLAG_STRICT,
		     RECV_ATTR_INT(MAIL_ATTR_STATUS, &status),
		     ATTR_TYPE_END) == 1) {
	abounce_done(ap, status);
    } else {
	abounce_done(ap, -1);
    }
}

/* abounce_send - send the request and suspend until the server replies */

static void abounce_send(int event, void *context)
{
    ABOUNCE_STATE *ap = (ABOUNCE_STATE *) context;

    /*
     * Receive the server's protocol name announcement. At this point the
     * server is ready to receive a request without blocking the sender. Send
     * the request and suspend until the server replies (or dies).
     */
    if (event != EVENT_TIME)
	event_cancel_timer(abounce_send, context);

    non_blocking(vstream_fileno(ap->fp), BLOCKING);
    if (event == EVENT_READ
	&& attr_scan(ap->fp, ATTR_FLAG_STRICT,
		   RECV_ATTR_STREQ(MAIL_ATTR_PROTO, MAIL_ATTR_PROTO_BOUNCE),
		     ATTR_TYPE_END) == 0
	&& vstream_fwrite(ap->fp, STR(ap->request),
			  LEN(ap->request)) == LEN(ap->request)
	&& vstream_fflush(ap->fp) == 0) {
	ABOUNCE_EVENT_ENABLE(vstream_fileno(ap->fp), abounce_receive,
			     (void *) ap, ABOUNCE_TIMEOUT);
    } else {
	abounce_done(ap, -1);
    }
}

/* abounce_connect - connect and suspend until the server replies */

static void abounce_connect(const char *class, const char *service,
			            int command, int flags,
			            const char *queue, const char *id,
			            const char *encoding, int smtputf8,
			            const char *sender,
			            const char *dsn_envid, int dsn_ret,
			            const char *verp, ABOUNCE_FN callback,
			            void *context)
{
    ABOUNCE_STATE *ap;

    /*
     * Save pseudo thread state. Connect to the server. Prior to Postfix 3.6
     * the asynchronous bounce flush/warn client called mail_connect_wait()
     * which sleeps and retries several times before terminating with a fatal
     * error. This block-and-sleep behavior was not consistent with a) the
     * rest of the code in this module, and with b) the synchronous bounce
     * client which gives up immediately. It should be safe to give up
     * immediately because that leaves the bounce/defer/trace logs in the
     * queue. In particular, this should not increase the simultaneous number
     * of asynchronous bounce/defer/trace flush/warn requests that are in
     * flight.
     */
    ap = (ABOUNCE_STATE *) mymalloc(sizeof(*ap));
    ap->command = command;
    ap->flags = flags;
    ap->id = mystrdup(id);
    ap->request = vstring_alloc(ABOUNCE_BUFSIZE);
    ap->callback = callback;
    ap->context = context;
    ap->fp = mail_connect(class, service, NON_BLOCKING);

    /*
     * Format the request now, so that we don't have to save a lot of
     * arguments now and format the request later.
     */
    if (ap->fp != 0) {
	/* Note: all code paths must terminate or enable I/O events. */
	VSTREAM *mp = vstream_memopen(ap->request, O_WRONLY);

	if (attr_print(mp, ATTR_FLAG_MORE,
		       SEND_ATTR_INT(MAIL_ATTR_NREQ, command),
		       SEND_ATTR_INT(MAIL_ATTR_FLAGS, flags),
		       SEND_ATTR_STR(MAIL_ATTR_QUEUE, queue),
		       SEND_ATTR_STR(MAIL_ATTR_QUEUEID, id),
		       SEND_ATTR_STR(MAIL_ATTR_ENCODING, encoding),
		       SEND_ATTR_INT(MAIL_ATTR_SMTPUTF8, smtputf8),
		       SEND_ATTR_STR(MAIL_ATTR_SENDER, sender),
		       SEND_ATTR_STR(MAIL_ATTR_DSN_ENVID, dsn_envid),
		       SEND_ATTR_INT(MAIL_ATTR_DSN_RET, dsn_ret),
		       ATTR_TYPE_END) != 0
	    || (verp != 0
		&& attr_print(mp, ATTR_FLAG_MORE,
			      SEND_ATTR_STR(MAIL_ATTR_VERPDL, verp),
			      ATTR_TYPE_END) != 0)
	    || attr_print(mp, ATTR_FLAG_NONE,
			  ATTR_TYPE_END) != 0
	    || vstream_fclose(mp) != 0)
	    msg_panic("abounce_connect: write request to memory stream: %m");

	/*
	 * Suspend until the server replies (or dies).
	 */
	ABOUNCE_EVENT_ENABLE(vstream_fileno(ap->fp), abounce_send,
			     (void *) ap, ABOUNCE_TIMEOUT);
    } else {
	abounce_done(ap, -1);
    }
}

/* abounce_flush_verp - asynchronous bounce flush */

void    abounce_flush_verp(int flags, const char *queue, const char *id,
			           const char *encoding, int smtputf8,
			           const char *sender, const char *dsn_envid,
			           int dsn_ret, const char *verp,
			           ABOUNCE_FN callback,
			           void *context)
{
    abounce_connect(MAIL_CLASS_PRIVATE, var_bounce_service,
		    BOUNCE_CMD_VERP, flags, queue, id, encoding, smtputf8,
		    sender, dsn_envid, dsn_ret, verp, callback, context);
}

/* adefer_flush_verp - asynchronous defer flush */

void    adefer_flush_verp(int flags, const char *queue, const char *id,
			          const char *encoding, int smtputf8,
			          const char *sender, const char *dsn_envid,
			          int dsn_ret, const char *verp,
			          ABOUNCE_FN callback, void *context)
{
    flags |= BOUNCE_FLAG_DELRCPT;
    abounce_connect(MAIL_CLASS_PRIVATE, var_defer_service,
		    BOUNCE_CMD_VERP, flags, queue, id, encoding, smtputf8,
		    sender, dsn_envid, dsn_ret, verp, callback, context);
}

/* abounce_flush - asynchronous bounce flush */

void    abounce_flush(int flags, const char *queue, const char *id,
		              const char *encoding, int smtputf8,
		              const char *sender, const char *dsn_envid,
		              int dsn_ret, ABOUNCE_FN callback,
		              void *context)
{
    abounce_connect(MAIL_CLASS_PRIVATE, var_bounce_service, BOUNCE_CMD_FLUSH,
		    flags, queue, id, encoding, smtputf8, sender, dsn_envid,
		    dsn_ret, ABOUNCE_NO_VERP, callback, context);
}

/* adefer_flush - asynchronous defer flush */

void    adefer_flush(int flags, const char *queue, const char *id,
		             const char *encoding, int smtputf8,
		             const char *sender, const char *dsn_envid,
		             int dsn_ret, ABOUNCE_FN callback, void *context)
{
    flags |= BOUNCE_FLAG_DELRCPT;
    abounce_connect(MAIL_CLASS_PRIVATE, var_defer_service, BOUNCE_CMD_FLUSH,
		    flags, queue, id, encoding, smtputf8, sender, dsn_envid,
		    dsn_ret, ABOUNCE_NO_VERP, callback, context);
}

/* adefer_warn - send copy of defer log to sender as warning bounce */

void    adefer_warn(int flags, const char *queue, const char *id,
		            const char *encoding, int smtputf8,
		            const char *sender, const char *dsn_envid,
		            int dsn_ret, ABOUNCE_FN callback, void *context)
{
    abounce_connect(MAIL_CLASS_PRIVATE, var_defer_service, BOUNCE_CMD_WARN,
		    flags, queue, id, encoding, smtputf8, sender, dsn_envid,
		    dsn_ret, ABOUNCE_NO_VERP, callback, context);
}

/* atrace_flush - asynchronous trace flush */

void    atrace_flush(int flags, const char *queue, const char *id,
		             const char *encoding, int smtputf8,
		             const char *sender, const char *dsn_envid,
		             int dsn_ret, ABOUNCE_FN callback, void *context)
{
    abounce_connect(MAIL_CLASS_PRIVATE, var_trace_service, BOUNCE_CMD_TRACE,
		    flags, queue, id, encoding, smtputf8, sender, dsn_envid,
		    dsn_ret, ABOUNCE_NO_VERP, callback, context);
}
