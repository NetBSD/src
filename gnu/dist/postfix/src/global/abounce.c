/*++
/* NAME
/*	abounce 3
/* SUMMARY
/*	asynchronous bounce/defer service client
/* SYNOPSIS
/*	#include <abounce.h>
/*
/*	void	abounce_flush(flags, queue, id, encoding, sender,
/*				callback, context)
/*	int	flags;
/*	const char *queue;
/*	const char *id;
/*	const char *encoding;
/*	const char *sender;
/*	void	(*callback)(int status, char *context);
/*	char	*context;
/*
/*	void	abounce_flush_verp(flags, queue, id, encoding,
/*				sender, verp, callback, context)
/*	int	flags;
/*	const char *queue;
/*	const char *id;
/*	const char *encoding;
/*	const char *sender;
/*	const char *verp;
/*	void	(*callback)(int status, char *context);
/*	char	*context;
/*
/*	void	adefer_flush(flags, queue, id, encoding, sender,
/*				callback, context)
/*	int	flags;
/*	const char *queue;
/*	const char *id;
/*	const char *encoding;
/*	const char *sender;
/*	void	(*callback)(int status, char *context);
/*	char	*context;
/*
/*	void	adefer_flush_verp(flags, queue, id, encoding,
/*				sender, verp, callback, context)
/*	int	flags;
/*	const char *queue;
/*	const char *id;
/*	const char *encoding;
/*	const char *sender;
/*	const char *verp;
/*	void	(*callback)(int status, char *context);
/*	char	*context;
/*
/*	void	adefer_warn(flags, queue, id, encoding, sender,
/*				callback, context)
/*	int	flags;
/*	const char *queue;
/*	const char *id;
/*	const char *encoding;
/*	const char *sender;
/*	void	(*callback)(int status, char *context);
/*	char	*context;
/* DESCRIPTION
/*	This module implements an asynchronous interface to the
/*	bounce/defer service for submitting sender notifications
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
/*	Arguments:
/* .IP flags
/*	The bitwise OR of zero or more of the following (specify
/*	BOUNCE_FLAG_NONE to request no special processing):
/* .RS
/* .IP BOUNCE_FLAG_CLEAN
/*	Delete the bounce log in case of an error (as in: pretend
/*	that we never even tried to bounce this message).
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
/* .IP sender
/*	The sender envelope address.
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
    ABOUNCE_FN callback;		/* application callback */
    char   *context;			/* application context */
    VSTREAM *fp;			/* server I/O handle */
} ABOUNCE;

/* abounce_done - deliver status to application and clean up pseudo thread */

static void abounce_done(ABOUNCE *ap, int status)
{
    (void) vstream_fclose(ap->fp);
    if (status != 0 && (ap->flags & BOUNCE_FLAG_CLEAN) == 0)
	msg_info("%s: status=deferred (%s failed)", ap->id,
		 ap->command == BOUNCE_CMD_FLUSH ? "bounce" :
		 ap->command == BOUNCE_CMD_WARN ? "delay warning" :
		 "whatever");
    ap->callback(status, ap->context);
    myfree(ap->id);
    myfree((char *) ap);
}

/* abounce_event - resume pseudo thread after server reply event */

static void abounce_event(int unused_event, char *context)
{
    ABOUNCE *ap = (ABOUNCE *) context;
    int     status;

    event_disable_readwrite(vstream_fileno(ap->fp));
    abounce_done(ap, attr_scan(ap->fp, ATTR_FLAG_STRICT,
			       ATTR_TYPE_NUM, MAIL_ATTR_STATUS, &status,
			       ATTR_TYPE_END) == 1 ? status : -1);
}

/* abounce_request_verp - suspend pseudo thread until server reply event */

static void abounce_request_verp(const char *class, const char *service,
				         int command, int flags,
				         const char *queue, const char *id,
				         const char *encoding,
				         const char *sender,
				         const char *verp,
				         ABOUNCE_FN callback,
				         char *context)
{
    ABOUNCE *ap;

    /*
     * Save pseudo thread state. Connect to the server. Send the request and
     * suspend the pseudo thread until the server replies (or dies).
     */
    ap = (ABOUNCE *) mymalloc(sizeof(*ap));
    ap->command = command;
    ap->flags = flags;
    ap->id = mystrdup(id);
    ap->callback = callback;
    ap->context = context;
    ap->fp = mail_connect_wait(class, service);

    if (attr_print(ap->fp, ATTR_FLAG_NONE,
		   ATTR_TYPE_NUM, MAIL_ATTR_NREQ, command,
		   ATTR_TYPE_NUM, MAIL_ATTR_FLAGS, flags,
		   ATTR_TYPE_STR, MAIL_ATTR_QUEUE, queue,
		   ATTR_TYPE_STR, MAIL_ATTR_QUEUEID, id,
		   ATTR_TYPE_STR, MAIL_ATTR_ENCODING, encoding,
		   ATTR_TYPE_STR, MAIL_ATTR_SENDER, sender,
		   ATTR_TYPE_STR, MAIL_ATTR_VERPDL, verp,
		   ATTR_TYPE_END) == 0
	&& vstream_fflush(ap->fp) == 0) {
	event_enable_read(vstream_fileno(ap->fp), abounce_event, (char *) ap);
    } else {
	abounce_done(ap, -1);
    }
}

/* abounce_flush_verp - asynchronous bounce flush */

void    abounce_flush_verp(int flags, const char *queue, const char *id,
			           const char *encoding, const char *sender,
			           const char *verp, ABOUNCE_FN callback,
			           char *context)
{
    abounce_request_verp(MAIL_CLASS_PRIVATE, var_bounce_service,
			 BOUNCE_CMD_VERP, flags, queue, id, encoding,
			 sender, verp, callback, context);
}

/* adefer_flush_verp - asynchronous defer flush */

void    adefer_flush_verp(int flags, const char *queue, const char *id,
			          const char *encoding, const char *sender,
			          const char *verp, ABOUNCE_FN callback,
			          char *context)
{
    flags |= BOUNCE_FLAG_DELRCPT;
    abounce_request_verp(MAIL_CLASS_PRIVATE, var_defer_service,
			 BOUNCE_CMD_VERP, flags, queue, id, encoding,
			 sender, verp, callback, context);
}

/* abounce_request - suspend pseudo thread until server reply event */

static void abounce_request(const char *class, const char *service,
			            int command, int flags,
			            const char *queue, const char *id,
			            const char *encoding, const char *sender,
			            ABOUNCE_FN callback, char *context)
{
    ABOUNCE *ap;

    /*
     * Save pseudo thread state. Connect to the server. Send the request and
     * suspend the pseudo thread until the server replies (or dies).
     */
    ap = (ABOUNCE *) mymalloc(sizeof(*ap));
    ap->command = command;
    ap->flags = flags;
    ap->id = mystrdup(id);
    ap->callback = callback;
    ap->context = context;
    ap->fp = mail_connect_wait(class, service);

    if (attr_print(ap->fp, ATTR_FLAG_NONE,
		   ATTR_TYPE_NUM, MAIL_ATTR_NREQ, command,
		   ATTR_TYPE_NUM, MAIL_ATTR_FLAGS, flags,
		   ATTR_TYPE_STR, MAIL_ATTR_QUEUE, queue,
		   ATTR_TYPE_STR, MAIL_ATTR_QUEUEID, id,
		   ATTR_TYPE_STR, MAIL_ATTR_ENCODING, encoding,
		   ATTR_TYPE_STR, MAIL_ATTR_SENDER, sender,
		   ATTR_TYPE_END) == 0
	&& vstream_fflush(ap->fp) == 0) {
	event_enable_read(vstream_fileno(ap->fp), abounce_event, (char *) ap);
    } else {
	abounce_done(ap, -1);
    }
}

/* abounce_flush - asynchronous bounce flush */

void    abounce_flush(int flags, const char *queue, const char *id,
		              const char *encoding, const char *sender,
		              ABOUNCE_FN callback, char *context)
{
    abounce_request(MAIL_CLASS_PRIVATE, var_bounce_service, BOUNCE_CMD_FLUSH,
		    flags, queue, id, encoding, sender, callback, context);
}

/* adefer_flush - asynchronous defer flush */

void    adefer_flush(int flags, const char *queue, const char *id,
		             const char *encoding, const char *sender,
		             ABOUNCE_FN callback, char *context)
{
    flags |= BOUNCE_FLAG_DELRCPT;
    abounce_request(MAIL_CLASS_PRIVATE, var_defer_service, BOUNCE_CMD_FLUSH,
		    flags, queue, id, encoding, sender, callback, context);
}

/* adefer_warn - send copy of defer log to sender as warning bounce */

void    adefer_warn(int flags, const char *queue, const char *id,
		            const char *encoding, const char *sender,
		            ABOUNCE_FN callback, char *context)
{
    abounce_request(MAIL_CLASS_PRIVATE, var_defer_service, BOUNCE_CMD_WARN,
		    flags, queue, id, encoding, sender, callback, context);
}
