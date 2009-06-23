/*	$NetBSD: scache_clnt.c,v 1.1.1.1 2009/06/23 10:08:48 tron Exp $	*/

/*++
/* NAME
/*	scache_clnt 3
/* SUMMARY
/*	session cache manager client
/* SYNOPSIS
/*	#include <scache.h>
/* DESCRIPTION
/*	SCACHE *scache_clnt_create(server, timeout, idle_limit, ttl_limit)
/*	const char *server;
/*	int	timeout;
/*	int	idle_limit;
/*	int	ttl_limit;
/* DESCRIPTION
/*	This module implements the client-side protocol of the
/*	session cache service.
/*
/*	scache_clnt_create() creates a session cache service client.
/*
/*	Arguments:
/* .IP server
/*	The session cache service name.
/* .IP timeout
/*	Time limit for connect, send or receive operations.
/* .IP idle_limit
/*	Idle time after which the client disconnects.
/* .IP ttl_limit
/*	Upper bound on the time that a connection is allowed to persist.
/* DIAGNOSTICS
/*	Fatal error: memory allocation problem;
/*	warning: communication error;
/*	panic: internal consistency failure.
/* SEE ALSO
/*	scache(3), generic session cache API
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
#include <errno.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <auto_clnt.h>
#include <stringops.h>

/*#define msg_verbose 1*/

/* Global library. */

#include <mail_proto.h>
#include <mail_params.h>
#include <scache.h>

/* Application-specific. */

 /*
  * SCACHE_CLNT is a derived type from the SCACHE super-class.
  */
typedef struct {
    SCACHE  scache[1];			/* super-class */
    AUTO_CLNT *auto_clnt;		/* client endpoint */
#ifdef CANT_WRITE_BEFORE_SENDING_FD
    VSTRING *dummy;			/* dummy buffer */
#endif
} SCACHE_CLNT;

#define STR(x) vstring_str(x)

#define SCACHE_MAX_TRIES	2

/* scache_clnt_save_endp - save endpoint */

static void scache_clnt_save_endp(SCACHE *scache, int endp_ttl,
				          const char *endp_label,
				          const char *endp_prop, int fd)
{
    SCACHE_CLNT *sp = (SCACHE_CLNT *) scache;
    const char *myname = "scache_clnt_save_endp";
    VSTREAM *stream;
    int     status;
    int     tries;
    int     count = 0;

    if (msg_verbose)
	msg_info("%s: endp=%s prop=%s fd=%d",
		 myname, endp_label, endp_prop, fd);

    /*
     * Sanity check.
     */
    if (endp_ttl <= 0)
	msg_panic("%s: bad endp_ttl: %d", myname, endp_ttl);

    /*
     * Try a few times before disabling the cache. We use synchronous calls;
     * the session cache service is CPU bound and making the client
     * asynchronous would just complicate the code.
     */
    for (tries = 0; sp->auto_clnt != 0; tries++) {
	if ((stream = auto_clnt_access(sp->auto_clnt)) != 0) {
	    errno = 0;
	    count += 1;
	    if (attr_print(stream, ATTR_FLAG_NONE,
			 ATTR_TYPE_STR, MAIL_ATTR_REQ, SCACHE_REQ_SAVE_ENDP,
			   ATTR_TYPE_INT, MAIL_ATTR_TTL, endp_ttl,
			   ATTR_TYPE_STR, MAIL_ATTR_LABEL, endp_label,
			   ATTR_TYPE_STR, MAIL_ATTR_PROP, endp_prop,
			   ATTR_TYPE_END) != 0
		|| vstream_fflush(stream)
#ifdef CANT_WRITE_BEFORE_SENDING_FD
		|| attr_scan(stream, ATTR_FLAG_STRICT,
			     ATTR_TYPE_STR, MAIL_ATTR_DUMMY, sp->dummy,
			     ATTR_TYPE_END) != 1
#endif
		|| LOCAL_SEND_FD(vstream_fileno(stream), fd) < 0
		|| attr_scan(stream, ATTR_FLAG_STRICT,
			     ATTR_TYPE_INT, MAIL_ATTR_STATUS, &status,
			     ATTR_TYPE_END) != 1) {
		if (msg_verbose || count > 1 || (errno && errno != EPIPE && errno != ENOENT))
		    msg_warn("problem talking to service %s: %m",
			     VSTREAM_PATH(stream));
		/* Give up or recover. */
	    } else {
		if (msg_verbose && status != 0)
		    msg_warn("%s: descriptor save failed with status %d",
			     myname, status);
		break;
	    }
	}
	/* Give up or recover. */
	if (tries >= SCACHE_MAX_TRIES - 1) {
	    msg_warn("disabling connection caching");
	    auto_clnt_free(sp->auto_clnt);
	    sp->auto_clnt = 0;
	    break;
	}
	sleep(1);				/* XXX make configurable */
	auto_clnt_recover(sp->auto_clnt);
    }
    /* Always close the descriptor before returning. */
    if (close(fd) < 0)
	msg_warn("%s: close(%d): %m", myname, fd);
}

/* scache_clnt_find_endp - look up cached session */

static int scache_clnt_find_endp(SCACHE *scache, const char *endp_label,
				         VSTRING *endp_prop)
{
    SCACHE_CLNT *sp = (SCACHE_CLNT *) scache;
    const char *myname = "scache_clnt_find_endp";
    VSTREAM *stream;
    int     status;
    int     tries;
    int     fd;

    /*
     * Try a few times before disabling the cache. We use synchronous calls;
     * the session cache service is CPU bound and making the client
     * asynchronous would just complicate the code.
     */
    for (tries = 0; sp->auto_clnt != 0; tries++) {
	if ((stream = auto_clnt_access(sp->auto_clnt)) != 0) {
	    errno = 0;
	    if (attr_print(stream, ATTR_FLAG_NONE,
			 ATTR_TYPE_STR, MAIL_ATTR_REQ, SCACHE_REQ_FIND_ENDP,
			   ATTR_TYPE_STR, MAIL_ATTR_LABEL, endp_label,
			   ATTR_TYPE_END) != 0
		|| vstream_fflush(stream)
		|| attr_scan(stream, ATTR_FLAG_STRICT,
			     ATTR_TYPE_INT, MAIL_ATTR_STATUS, &status,
			     ATTR_TYPE_STR, MAIL_ATTR_PROP, endp_prop,
			     ATTR_TYPE_END) != 2) {
		if (msg_verbose || (errno != EPIPE && errno != ENOENT))
		    msg_warn("problem talking to service %s: %m",
			     VSTREAM_PATH(stream));
		/* Give up or recover. */
	    } else if (status != 0) {
		if (msg_verbose)
		    msg_info("%s: not found: %s", myname, endp_label);
		return (-1);
	    } else if (
#ifdef CANT_WRITE_BEFORE_SENDING_FD
		       attr_print(stream, ATTR_FLAG_NONE,
				  ATTR_TYPE_STR, MAIL_ATTR_DUMMY, "",
				  ATTR_TYPE_END) != 0
		       || vstream_fflush(stream) != 0
		       || read_wait(vstream_fileno(stream),
				    stream->timeout) < 0 ||	/* XXX */
#endif
		       (fd = LOCAL_RECV_FD(vstream_fileno(stream))) < 0) {
		if (msg_verbose || (errno != EPIPE && errno != ENOENT))
		    msg_warn("problem talking to service %s: %m",
			     VSTREAM_PATH(stream));
		/* Give up or recover. */
	    } else {
#ifdef MUST_READ_AFTER_SENDING_FD
		(void) attr_print(stream, ATTR_FLAG_NONE,
				  ATTR_TYPE_STR, MAIL_ATTR_DUMMY, "",
				  ATTR_TYPE_END);
		(void) vstream_fflush(stream);
#endif
		if (msg_verbose)
		    msg_info("%s: endp=%s prop=%s fd=%d",
			     myname, endp_label, STR(endp_prop), fd);
		return (fd);
	    }
	}
	/* Give up or recover. */
	if (tries >= SCACHE_MAX_TRIES - 1) {
	    msg_warn("disabling connection caching");
	    auto_clnt_free(sp->auto_clnt);
	    sp->auto_clnt = 0;
	    return (-1);
	}
	sleep(1);				/* XXX make configurable */
	auto_clnt_recover(sp->auto_clnt);
    }
    return (-1);
}

/* scache_clnt_save_dest - create destination/endpoint association */

static void scache_clnt_save_dest(SCACHE *scache, int dest_ttl,
				          const char *dest_label,
				          const char *dest_prop,
				          const char *endp_label)
{
    SCACHE_CLNT *sp = (SCACHE_CLNT *) scache;
    const char *myname = "scache_clnt_save_dest";
    VSTREAM *stream;
    int     status;
    int     tries;

    if (msg_verbose)
	msg_info("%s: dest_label=%s dest_prop=%s endp_label=%s",
		 myname, dest_label, dest_prop, endp_label);

    /*
     * Sanity check.
     */
    if (dest_ttl <= 0)
	msg_panic("%s: bad dest_ttl: %d", myname, dest_ttl);

    /*
     * Try a few times before disabling the cache. We use synchronous calls;
     * the session cache service is CPU bound and making the client
     * asynchronous would just complicate the code.
     */
    for (tries = 0; sp->auto_clnt != 0; tries++) {
	if ((stream = auto_clnt_access(sp->auto_clnt)) != 0) {
	    errno = 0;
	    if (attr_print(stream, ATTR_FLAG_NONE,
			 ATTR_TYPE_STR, MAIL_ATTR_REQ, SCACHE_REQ_SAVE_DEST,
			   ATTR_TYPE_INT, MAIL_ATTR_TTL, dest_ttl,
			   ATTR_TYPE_STR, MAIL_ATTR_LABEL, dest_label,
			   ATTR_TYPE_STR, MAIL_ATTR_PROP, dest_prop,
			   ATTR_TYPE_STR, MAIL_ATTR_LABEL, endp_label,
			   ATTR_TYPE_END) != 0
		|| vstream_fflush(stream)
		|| attr_scan(stream, ATTR_FLAG_STRICT,
			     ATTR_TYPE_INT, MAIL_ATTR_STATUS, &status,
			     ATTR_TYPE_END) != 1) {
		if (msg_verbose || (errno != EPIPE && errno != ENOENT))
		    msg_warn("problem talking to service %s: %m",
			     VSTREAM_PATH(stream));
		/* Give up or recover. */
	    } else {
		if (msg_verbose && status != 0)
		    msg_warn("%s: destination save failed with status %d",
			     myname, status);
		break;
	    }
	}
	/* Give up or recover. */
	if (tries >= SCACHE_MAX_TRIES - 1) {
	    msg_warn("disabling connection caching");
	    auto_clnt_free(sp->auto_clnt);
	    sp->auto_clnt = 0;
	    break;
	}
	sleep(1);				/* XXX make configurable */
	auto_clnt_recover(sp->auto_clnt);
    }
}

/* scache_clnt_find_dest - look up cached session */

static int scache_clnt_find_dest(SCACHE *scache, const char *dest_label,
				         VSTRING *dest_prop,
				         VSTRING *endp_prop)
{
    SCACHE_CLNT *sp = (SCACHE_CLNT *) scache;
    const char *myname = "scache_clnt_find_dest";
    VSTREAM *stream;
    int     status;
    int     tries;
    int     fd;

    /*
     * Try a few times before disabling the cache. We use synchronous calls;
     * the session cache service is CPU bound and making the client
     * asynchronous would just complicate the code.
     */
    for (tries = 0; sp->auto_clnt != 0; tries++) {
	if ((stream = auto_clnt_access(sp->auto_clnt)) != 0) {
	    errno = 0;
	    if (attr_print(stream, ATTR_FLAG_NONE,
			 ATTR_TYPE_STR, MAIL_ATTR_REQ, SCACHE_REQ_FIND_DEST,
			   ATTR_TYPE_STR, MAIL_ATTR_LABEL, dest_label,
			   ATTR_TYPE_END) != 0
		|| vstream_fflush(stream)
		|| attr_scan(stream, ATTR_FLAG_STRICT,
			     ATTR_TYPE_INT, MAIL_ATTR_STATUS, &status,
			     ATTR_TYPE_STR, MAIL_ATTR_PROP, dest_prop,
			     ATTR_TYPE_STR, MAIL_ATTR_PROP, endp_prop,
			     ATTR_TYPE_END) != 3) {
		if (msg_verbose || (errno != EPIPE && errno != ENOENT))
		    msg_warn("problem talking to service %s: %m",
			     VSTREAM_PATH(stream));
		/* Give up or recover. */
	    } else if (status != 0) {
		if (msg_verbose)
		    msg_info("%s: not found: %s", myname, dest_label);
		return (-1);
	    } else if (
#ifdef CANT_WRITE_BEFORE_SENDING_FD
		       attr_print(stream, ATTR_FLAG_NONE,
				  ATTR_TYPE_STR, MAIL_ATTR_DUMMY, "",
				  ATTR_TYPE_END) != 0
		       || vstream_fflush(stream) != 0
		       || read_wait(vstream_fileno(stream),
				    stream->timeout) < 0 ||	/* XXX */
#endif
		       (fd = LOCAL_RECV_FD(vstream_fileno(stream))) < 0) {
		if (msg_verbose || (errno != EPIPE && errno != ENOENT))
		    msg_warn("problem talking to service %s: %m",
			     VSTREAM_PATH(stream));
		/* Give up or recover. */
	    } else {
#ifdef MUST_READ_AFTER_SENDING_FD
		(void) attr_print(stream, ATTR_FLAG_NONE,
				  ATTR_TYPE_STR, MAIL_ATTR_DUMMY, "",
				  ATTR_TYPE_END);
		(void) vstream_fflush(stream);
#endif
		if (msg_verbose)
		    msg_info("%s: dest=%s dest_prop=%s endp_prop=%s fd=%d",
		    myname, dest_label, STR(dest_prop), STR(endp_prop), fd);
		return (fd);
	    }
	}
	/* Give up or recover. */
	if (tries >= SCACHE_MAX_TRIES - 1) {
	    msg_warn("disabling connection caching");
	    auto_clnt_free(sp->auto_clnt);
	    sp->auto_clnt = 0;
	    return (-1);
	}
	sleep(1);				/* XXX make configurable */
	auto_clnt_recover(sp->auto_clnt);
    }
    return (-1);
}

/* scache_clnt_size - dummy */

static void scache_clnt_size(SCACHE *unused_scache, SCACHE_SIZE *size)
{
    /* XXX Crap in a hurry. */
    size->dest_count = 0;
    size->endp_count = 0;
    size->sess_count = 0;
}

/* scache_clnt_free - destroy cache */

static void scache_clnt_free(SCACHE *scache)
{
    SCACHE_CLNT *sp = (SCACHE_CLNT *) scache;

    if (sp->auto_clnt)
	auto_clnt_free(sp->auto_clnt);
#ifdef CANT_WRITE_BEFORE_SENDING_FD
    vstring_free(sp->dummy);
#endif
    myfree((char *) sp);
}

/* scache_clnt_create - initialize */

SCACHE *scache_clnt_create(const char *server, int timeout,
			           int idle_limit, int ttl_limit)
{
    SCACHE_CLNT *sp = (SCACHE_CLNT *) mymalloc(sizeof(*sp));
    char   *service;

    sp->scache->save_endp = scache_clnt_save_endp;
    sp->scache->find_endp = scache_clnt_find_endp;
    sp->scache->save_dest = scache_clnt_save_dest;
    sp->scache->find_dest = scache_clnt_find_dest;
    sp->scache->size = scache_clnt_size;
    sp->scache->free = scache_clnt_free;

    service = concatenate("local:private/", var_scache_service, (char *) 0);
    sp->auto_clnt = auto_clnt_create(service, timeout, idle_limit, ttl_limit);
    myfree(service);

#ifdef CANT_WRITE_BEFORE_SENDING_FD
    sp->dummy = vstring_alloc(1);
#endif

    return (sp->scache);
}
