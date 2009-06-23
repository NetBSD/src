/*	$NetBSD: scache_single.c,v 1.1.1.1 2009/06/23 10:08:48 tron Exp $	*/

/*++
/* NAME
/*	scache_single 3
/* SUMMARY
/*	single-item session cache
/* SYNOPSIS
/*	#include <scache.h>
/* DESCRIPTION
/*	SCACHE *scache_single_create()
/* DESCRIPTION
/*	This module implements an in-memory, single-session cache.
/*
/*	scache_single_create() creates a session cache instance
/*	that stores a single session.
/* DIAGNOSTICS
/*	Fatal error: memory allocation problem;
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
#include <unistd.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <mymalloc.h>
#include <events.h>

/*#define msg_verbose 1*/

/* Global library. */

#include <scache.h>

/* Application-specific. */

 /*
  * Data structure for one saved connection. It is left up to the application
  * to serialize attributes upon passivation, and to de-serialize them upon
  * re-activation.
  */
typedef struct {
    VSTRING *endp_label;		/* physical endpoint name */
    VSTRING *endp_prop;			/* endpoint properties, serialized */
    int     fd;				/* the session */
} SCACHE_SINGLE_ENDP;

 /*
  * Data structure for a logical name to physical endpoint binding. It is
  * left up to the application to serialize attributes upon passivation, and
  * to de-serialize then upon re-activation.
  */
typedef struct {
    VSTRING *dest_label;		/* logical destination name */
    VSTRING *dest_prop;			/* binding properties, serialized */
    VSTRING *endp_label;		/* physical endpoint name */
} SCACHE_SINGLE_DEST;

 /*
  * SCACHE_SINGLE is a derived type from the SCACHE super-class.
  */
typedef struct {
    SCACHE  scache[1];			/* super-class */
    SCACHE_SINGLE_ENDP endp;		/* one cached session */
    SCACHE_SINGLE_DEST dest;		/* one cached binding */
} SCACHE_SINGLE;

static void scache_single_expire_endp(int, char *);
static void scache_single_expire_dest(int, char *);

#define SCACHE_SINGLE_ENDP_BUSY(sp)	(VSTRING_LEN(sp->endp.endp_label) > 0)
#define SCACHE_SINGLE_DEST_BUSY(sp)	(VSTRING_LEN(sp->dest.dest_label) > 0)

#define STR(x) vstring_str(x)

/* scache_single_free_endp - discard endpoint */

static void scache_single_free_endp(SCACHE_SINGLE *sp)
{
    const char *myname = "scache_single_free_endp";

    if (msg_verbose)
	msg_info("%s: %s", myname, STR(sp->endp.endp_label));

    event_cancel_timer(scache_single_expire_endp, (char *) sp);
    if (sp->endp.fd >= 0 && close(sp->endp.fd) < 0)
	msg_warn("close session endpoint %s: %m", STR(sp->endp.endp_label));
    VSTRING_RESET(sp->endp.endp_label);
    VSTRING_TERMINATE(sp->endp.endp_label);
    VSTRING_RESET(sp->endp.endp_prop);
    VSTRING_TERMINATE(sp->endp.endp_prop);
    sp->endp.fd = -1;
}

/* scache_single_expire_endp - discard expired session */

static void scache_single_expire_endp(int unused_event, char *context)
{
    SCACHE_SINGLE *sp = (SCACHE_SINGLE *) context;

    scache_single_free_endp(sp);
}

/* scache_single_save_endp - save endpoint */

static void scache_single_save_endp(SCACHE *scache, int endp_ttl,
				            const char *endp_label,
				            const char *endp_prop, int fd)
{
    SCACHE_SINGLE *sp = (SCACHE_SINGLE *) scache;
    const char *myname = "scache_single_save_endp";

    if (endp_ttl <= 0)
	msg_panic("%s: bad endp_ttl: %d", myname, endp_ttl);

    if (SCACHE_SINGLE_ENDP_BUSY(sp))
	scache_single_free_endp(sp);		/* dump the cached fd */

    vstring_strcpy(sp->endp.endp_label, endp_label);
    vstring_strcpy(sp->endp.endp_prop, endp_prop);
    sp->endp.fd = fd;
    event_request_timer(scache_single_expire_endp, (char *) sp, endp_ttl);

    if (msg_verbose)
	msg_info("%s: %s fd=%d", myname, endp_label, fd);
}

/* scache_single_find_endp - look up cached session */

static int scache_single_find_endp(SCACHE *scache, const char *endp_label,
				           VSTRING *endp_prop)
{
    SCACHE_SINGLE *sp = (SCACHE_SINGLE *) scache;
    const char *myname = "scache_single_find_endp";
    int     fd;

    if (!SCACHE_SINGLE_ENDP_BUSY(sp)) {
	if (msg_verbose)
	    msg_info("%s: no endpoint cache: %s", myname, endp_label);
	return (-1);
    }
    if (strcmp(STR(sp->endp.endp_label), endp_label) == 0) {
	vstring_strcpy(endp_prop, STR(sp->endp.endp_prop));
	fd = sp->endp.fd;
	sp->endp.fd = -1;
	scache_single_free_endp(sp);
	if (msg_verbose)
	    msg_info("%s: found: %s fd=%d", myname, endp_label, fd);
	return (fd);
    }
    if (msg_verbose)
	msg_info("%s: not found: %s", myname, endp_label);
    return (-1);
}

/* scache_single_free_dest - discard destination/endpoint association */

static void scache_single_free_dest(SCACHE_SINGLE *sp)
{
    const char *myname = "scache_single_free_dest";

    if (msg_verbose)
	msg_info("%s: %s -> %s", myname, STR(sp->dest.dest_label),
		 STR(sp->dest.endp_label));

    event_cancel_timer(scache_single_expire_dest, (char *) sp);
    VSTRING_RESET(sp->dest.dest_label);
    VSTRING_TERMINATE(sp->dest.dest_label);
    VSTRING_RESET(sp->dest.dest_prop);
    VSTRING_TERMINATE(sp->dest.dest_prop);
    VSTRING_RESET(sp->dest.endp_label);
    VSTRING_TERMINATE(sp->dest.endp_label);
}

/* scache_single_expire_dest - discard expired destination/endpoint binding */

static void scache_single_expire_dest(int unused_event, char *context)
{
    SCACHE_SINGLE *sp = (SCACHE_SINGLE *) context;

    scache_single_free_dest(sp);
}

/* scache_single_save_dest - create destination/endpoint association */

static void scache_single_save_dest(SCACHE *scache, int dest_ttl,
				            const char *dest_label,
				            const char *dest_prop,
				            const char *endp_label)
{
    SCACHE_SINGLE *sp = (SCACHE_SINGLE *) scache;
    const char *myname = "scache_single_save_dest";
    int     refresh;

    if (dest_ttl <= 0)
	msg_panic("%s: bad dest_ttl: %d", myname, dest_ttl);

    /*
     * Optimize: reset timer only, if nothing has changed.
     */
    refresh =
	(SCACHE_SINGLE_DEST_BUSY(sp)
	 && strcmp(STR(sp->dest.dest_label), dest_label) == 0
	 && strcmp(STR(sp->dest.dest_prop), dest_prop) == 0
	 && strcmp(STR(sp->dest.endp_label), endp_label) == 0);

    if (refresh == 0) {
	vstring_strcpy(sp->dest.dest_label, dest_label);
	vstring_strcpy(sp->dest.dest_prop, dest_prop);
	vstring_strcpy(sp->dest.endp_label, endp_label);
    }
    event_request_timer(scache_single_expire_dest, (char *) sp, dest_ttl);

    if (msg_verbose)
	msg_info("%s: %s -> %s%s", myname, dest_label, endp_label,
		 refresh ? " (refreshed)" : "");
}

/* scache_single_find_dest - look up cached session */

static int scache_single_find_dest(SCACHE *scache, const char *dest_label,
			             VSTRING *dest_prop, VSTRING *endp_prop)
{
    SCACHE_SINGLE *sp = (SCACHE_SINGLE *) scache;
    const char *myname = "scache_single_find_dest";
    int     fd;

    if (!SCACHE_SINGLE_DEST_BUSY(sp)) {
	if (msg_verbose)
	    msg_info("%s: no destination cache: %s", myname, dest_label);
	return (-1);
    }
    if (strcmp(STR(sp->dest.dest_label), dest_label) == 0) {
	if (msg_verbose)
	    msg_info("%s: found: %s", myname, dest_label);
	if ((fd = scache_single_find_endp(scache, STR(sp->dest.endp_label), endp_prop)) >= 0) {
	    vstring_strcpy(dest_prop, STR(sp->dest.dest_prop));
	    return (fd);
	}
    }
    if (msg_verbose)
	msg_info("%s: not found: %s", myname, dest_label);
    return (-1);
}

/* scache_single_size - size of single-element cache :-) */

static void scache_single_size(SCACHE *scache, SCACHE_SIZE *size)
{
    SCACHE_SINGLE *sp = (SCACHE_SINGLE *) scache;

    size->dest_count = (!SCACHE_SINGLE_DEST_BUSY(sp) ? 0 : 1);
    size->endp_count = (!SCACHE_SINGLE_ENDP_BUSY(sp) ? 0 : 1);
    size->sess_count = (sp->endp.fd < 0 ? 0 : 1);
}

/* scache_single_free - destroy single-element cache object */

static void scache_single_free(SCACHE *scache)
{
    SCACHE_SINGLE *sp = (SCACHE_SINGLE *) scache;

    vstring_free(sp->endp.endp_label);
    vstring_free(sp->endp.endp_prop);
    if (sp->endp.fd >= 0)
	close(sp->endp.fd);

    vstring_free(sp->dest.dest_label);
    vstring_free(sp->dest.dest_prop);
    vstring_free(sp->dest.endp_label);

    myfree((char *) sp);
}

/* scache_single_create - initialize */

SCACHE *scache_single_create(void)
{
    SCACHE_SINGLE *sp = (SCACHE_SINGLE *) mymalloc(sizeof(*sp));

    sp->scache->save_endp = scache_single_save_endp;
    sp->scache->find_endp = scache_single_find_endp;
    sp->scache->save_dest = scache_single_save_dest;
    sp->scache->find_dest = scache_single_find_dest;
    sp->scache->size = scache_single_size;
    sp->scache->free = scache_single_free;

    sp->endp.endp_label = vstring_alloc(10);
    sp->endp.endp_prop = vstring_alloc(10);
    sp->endp.fd = -1;

    sp->dest.dest_label = vstring_alloc(10);
    sp->dest.dest_prop = vstring_alloc(10);
    sp->dest.endp_label = vstring_alloc(10);

    return (sp->scache);
}
