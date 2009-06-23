/*	$NetBSD: scache_multi.c,v 1.1.1.1 2009/06/23 10:08:48 tron Exp $	*/

/*++
/* NAME
/*	scache_multi 3
/* SUMMARY
/*	multi-session cache
/* SYNOPSIS
/*	#include <scache.h>
/* DESCRIPTION
/*	SCACHE *scache_multi_create()
/* DESCRIPTION
/*	This module implements an in-memory, multi-session cache.
/*
/*	scache_multi_create() instantiates a session cache that
/*	stores multiple sessions.
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
#include <stddef.h>			/* offsetof() */
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <ring.h>
#include <htable.h>
#include <vstring.h>
#include <mymalloc.h>
#include <events.h>

/*#define msg_verbose 1*/

/* Global library. */

#include <scache.h>

/* Application-specific. */

 /*
  * SCACHE_MULTI is a derived type from the SCACHE super-class.
  * 
  * Each destination has an entry in the destination hash table, and each
  * destination->endpoint binding is kept in a circular list under its
  * destination hash table entry.
  * 
  * Likewise, each endpoint has an entry in the endpoint hash table, and each
  * endpoint->session binding is kept in a circular list under its endpoint
  * hash table entry.
  * 
  * We do not attempt to limit the number of destination or endpoint entries,
  * nor do we attempt to limit the number of sessions. Doing so would require
  * a write-through cache. Currently, the CTABLE cache module supports only
  * read-through caching.
  * 
  * This is no problem because the number of cached destinations is limited by
  * design. Sites that specify a wild-card domain pattern, and thus cache
  * every session in recent history, may be in for a surprise.
  */
typedef struct {
    SCACHE  scache[1];			/* super-class */
    HTABLE *dest_cache;			/* destination->endpoint bindings */
    HTABLE *endp_cache;			/* endpoint->session bindings */
    int     sess_count;			/* number of cached sessions */
} SCACHE_MULTI;

 /*
  * Storage for a destination or endpoint list head. Each list head knows its
  * own hash table entry name, so that we can remove the list when it becomes
  * empty. List items are stored in a circular list under the list head.
  */
typedef struct {
    RING    ring[1];			/* circular list linkage */
    char   *parent_key;			/* parent linkage: hash table */
    SCACHE_MULTI *cache;		/* parent linkage: cache */
} SCACHE_MULTI_HEAD;

#define RING_TO_MULTI_HEAD(p) RING_TO_APPL((p), SCACHE_MULTI_HEAD, ring)

 /*
  * Storage for a destination->endpoint binding. This is an element in a
  * circular list, whose list head specifies the destination.
  */
typedef struct {
    RING    ring[1];			/* circular list linkage */
    SCACHE_MULTI_HEAD *head;		/* parent linkage: list head */
    char   *endp_label;			/* endpoint name */
    char   *dest_prop;			/* binding properties */
} SCACHE_MULTI_DEST;

#define RING_TO_MULTI_DEST(p) RING_TO_APPL((p), SCACHE_MULTI_DEST, ring)

static void scache_multi_expire_dest(int, char *);

 /*
  * Storage for an endpoint->session binding. This is an element in a
  * circular list, whose list head specifies the endpoint.
  */
typedef struct {
    RING    ring[1];			/* circular list linkage */
    SCACHE_MULTI_HEAD *head;		/* parent linkage: list head */
    int     fd;				/* cached session */
    char   *endp_prop;			/* binding properties */
} SCACHE_MULTI_ENDP;

#define RING_TO_MULTI_ENDP(p) RING_TO_APPL((p), SCACHE_MULTI_ENDP, ring)

static void scache_multi_expire_endp(int, char *);

 /*
  * When deleting a circular list element, are we deleting the entire
  * circular list, or are we removing a single list element. We need this
  * distinction to avoid a re-entrancy problem between htable_delete() and
  * htable_free().
  */
#define BOTTOM_UP	1		/* one item */
#define TOP_DOWN	2		/* whole list */

/* scache_multi_drop_endp - destroy endpoint->session binding */

static void scache_multi_drop_endp(SCACHE_MULTI_ENDP *endp, int direction)
{
    const char *myname = "scache_multi_drop_endp";
    SCACHE_MULTI_HEAD *head;

    if (msg_verbose)
	msg_info("%s: endp_prop=%s fd=%d", myname,
		 endp->endp_prop, endp->fd);

    /*
     * Stop the timer.
     */
    event_cancel_timer(scache_multi_expire_endp, (char *) endp);

    /*
     * In bottom-up mode, remove the list head from the endpoint hash when
     * the list becomes empty. Otherwise, remove the endpoint->session
     * binding from the list.
     */
    ring_detach(endp->ring);
    head = endp->head;
    head->cache->sess_count--;
    if (direction == BOTTOM_UP && ring_pred(head->ring) == head->ring)
	htable_delete(head->cache->endp_cache, head->parent_key, myfree);

    /*
     * Destroy the endpoint->session binding.
     */
    if (endp->fd >= 0 && close(endp->fd) != 0)
	msg_warn("%s: close(%d): %m", myname, endp->fd);
    myfree(endp->endp_prop);

    myfree((char *) endp);
}

/* scache_multi_expire_endp - event timer call-back */

static void scache_multi_expire_endp(int unused_event, char *context)
{
    SCACHE_MULTI_ENDP *endp = (SCACHE_MULTI_ENDP *) context;

    scache_multi_drop_endp(endp, BOTTOM_UP);
}

/* scache_multi_free_endp - hash table destructor call-back */

static void scache_multi_free_endp(char *ptr)
{
    SCACHE_MULTI_HEAD *head = (SCACHE_MULTI_HEAD *) ptr;
    SCACHE_MULTI_ENDP *endp;
    RING   *ring;

    /*
     * Delete each endpoint->session binding in the list, then delete the
     * list head. Note: this changes the list, so we must iterate carefully.
     */
    while ((ring = ring_succ(head->ring)) != head->ring) {
	endp = RING_TO_MULTI_ENDP(ring);
	scache_multi_drop_endp(endp, TOP_DOWN);
    }
    myfree((char *) head);
}

/* scache_multi_save_endp - save endpoint->session binding */

static void scache_multi_save_endp(SCACHE *scache, int ttl,
				           const char *endp_label,
				           const char *endp_prop, int fd)
{
    const char *myname = "scache_multi_save_endp";
    SCACHE_MULTI *sp = (SCACHE_MULTI *) scache;
    SCACHE_MULTI_HEAD *head;
    SCACHE_MULTI_ENDP *endp;

    if (ttl < 0)
	msg_panic("%s: bad ttl: %d", myname, ttl);

    /*
     * Look up or instantiate the list head with the endpoint name.
     */
    if ((head = (SCACHE_MULTI_HEAD *)
	 htable_find(sp->endp_cache, endp_label)) == 0) {
	head = (SCACHE_MULTI_HEAD *) mymalloc(sizeof(*head));
	ring_init(head->ring);
	head->parent_key =
	    htable_enter(sp->endp_cache, endp_label, (char *) head)->key;
	head->cache = sp;
    }

    /*
     * Add the endpoint->session binding to the list. There can never be a
     * duplicate, because each session must have a different file descriptor.
     */
    endp = (SCACHE_MULTI_ENDP *) mymalloc(sizeof(*endp));
    endp->head = head;
    endp->fd = fd;
    endp->endp_prop = mystrdup(endp_prop);
    ring_prepend(head->ring, endp->ring);
    sp->sess_count++;

    /*
     * Make sure this binding will go away eventually.
     */
    event_request_timer(scache_multi_expire_endp, (char *) endp, ttl);

    if (msg_verbose)
	msg_info("%s: endp_label=%s -> endp_prop=%s fd=%d",
		 myname, endp_label, endp_prop, fd);
}

/* scache_multi_find_endp - look up session for named endpoint */

static int scache_multi_find_endp(SCACHE *scache, const char *endp_label,
				          VSTRING *endp_prop)
{
    const char *myname = "scache_multi_find_endp";
    SCACHE_MULTI *sp = (SCACHE_MULTI *) scache;
    SCACHE_MULTI_HEAD *head;
    SCACHE_MULTI_ENDP *endp;
    RING   *ring;
    int     fd;

    /*
     * Look up the list head with the endpoint name.
     */
    if ((head = (SCACHE_MULTI_HEAD *)
	 htable_find(sp->endp_cache, endp_label)) == 0) {
	if (msg_verbose)
	    msg_info("%s: no endpoint cache: endp_label=%s",
		     myname, endp_label);
	return (-1);
    }

    /*
     * Use the first available session. Remove the session from the cache
     * because we're giving it to someone else.
     */
    if ((ring = ring_succ(head->ring)) != head->ring) {
	endp = RING_TO_MULTI_ENDP(ring);
	fd = endp->fd;
	endp->fd = -1;
	vstring_strcpy(endp_prop, endp->endp_prop);
	if (msg_verbose)
	    msg_info("%s: found: endp_label=%s -> endp_prop=%s fd=%d",
		     myname, endp_label, endp->endp_prop, fd);
	scache_multi_drop_endp(endp, BOTTOM_UP);
	return (fd);
    }
    if (msg_verbose)
	msg_info("%s: not found: endp_label=%s", myname, endp_label);
    return (-1);
}

/* scache_multi_drop_dest - delete destination->endpoint binding */

static void scache_multi_drop_dest(SCACHE_MULTI_DEST *dest, int direction)
{
    const char *myname = "scache_multi_drop_dest";
    SCACHE_MULTI_HEAD *head;

    if (msg_verbose)
	msg_info("%s: dest_prop=%s endp_label=%s",
		 myname, dest->dest_prop, dest->endp_label);

    /*
     * Stop the timer.
     */
    event_cancel_timer(scache_multi_expire_dest, (char *) dest);

    /*
     * In bottom-up mode, remove the list head from the destination hash when
     * the list becomes empty. Otherwise, remove the destination->endpoint
     * binding from the list.
     */
    ring_detach(dest->ring);
    head = dest->head;
    if (direction == BOTTOM_UP && ring_pred(head->ring) == head->ring)
	htable_delete(head->cache->dest_cache, head->parent_key, myfree);

    /*
     * Destroy the destination->endpoint binding.
     */
    myfree(dest->dest_prop);
    myfree(dest->endp_label);

    myfree((char *) dest);
}

/* scache_multi_expire_dest - event timer call-back */

static void scache_multi_expire_dest(int unused_event, char *context)
{
    SCACHE_MULTI_DEST *dest = (SCACHE_MULTI_DEST *) context;

    scache_multi_drop_dest(dest, BOTTOM_UP);
}

/* scache_multi_free_dest - hash table destructor call-back */

static void scache_multi_free_dest(char *ptr)
{
    SCACHE_MULTI_HEAD *head = (SCACHE_MULTI_HEAD *) ptr;
    SCACHE_MULTI_DEST *dest;
    RING   *ring;

    /*
     * Delete each destination->endpoint binding in the list, then delete the
     * list head. Note: this changes the list, so we must iterate carefully.
     */
    while ((ring = ring_succ(head->ring)) != head->ring) {
	dest = RING_TO_MULTI_DEST(ring);
	scache_multi_drop_dest(dest, TOP_DOWN);
    }
    myfree((char *) head);
}

/* scache_multi_save_dest - save destination->endpoint binding */

static void scache_multi_save_dest(SCACHE *scache, int ttl,
				           const char *dest_label,
				           const char *dest_prop,
				           const char *endp_label)
{
    const char *myname = "scache_multi_save_dest";
    SCACHE_MULTI *sp = (SCACHE_MULTI *) scache;
    SCACHE_MULTI_HEAD *head;
    SCACHE_MULTI_DEST *dest;
    RING   *ring;
    int     refresh = 0;

    if (ttl < 0)
	msg_panic("%s: bad ttl: %d", myname, ttl);

    /*
     * Look up or instantiate the list head with the destination name.
     */
    if ((head = (SCACHE_MULTI_HEAD *)
	 htable_find(sp->dest_cache, dest_label)) == 0) {
	head = (SCACHE_MULTI_HEAD *) mymalloc(sizeof(*head));
	ring_init(head->ring);
	head->parent_key =
	    htable_enter(sp->dest_cache, dest_label, (char *) head)->key;
	head->cache = sp;
    }

    /*
     * Look up or instantiate the destination->endpoint binding. Update the
     * expiration time if this destination->endpoint binding already exists.
     */
    RING_FOREACH(ring, head->ring) {
	dest = RING_TO_MULTI_DEST(ring);
	if (strcmp(dest->endp_label, endp_label) == 0
	    && strcmp(dest->dest_prop, dest_prop) == 0) {
	    refresh = 1;
	    break;
	}
    }
    if (refresh == 0) {
	dest = (SCACHE_MULTI_DEST *) mymalloc(sizeof(*dest));
	dest->head = head;
	dest->endp_label = mystrdup(endp_label);
	dest->dest_prop = mystrdup(dest_prop);
	ring_prepend(head->ring, dest->ring);
    }

    /*
     * Make sure this binding will go away eventually.
     */
    event_request_timer(scache_multi_expire_dest, (char *) dest, ttl);

    if (msg_verbose)
	msg_info("%s: dest_label=%s -> dest_prop=%s endp_label=%s%s",
		 myname, dest_label, dest_prop, endp_label,
		 refresh ? " (refreshed)" : "");
}

/* scache_multi_find_dest - look up session for named destination */

static int scache_multi_find_dest(SCACHE *scache, const char *dest_label,
				          VSTRING *dest_prop,
				          VSTRING *endp_prop)
{
    const char *myname = "scache_multi_find_dest";
    SCACHE_MULTI *sp = (SCACHE_MULTI *) scache;
    SCACHE_MULTI_HEAD *head;
    SCACHE_MULTI_DEST *dest;
    RING   *ring;
    int     fd;

    /*
     * Look up the list head with the destination name.
     */
    if ((head = (SCACHE_MULTI_HEAD *)
	 htable_find(sp->dest_cache, dest_label)) == 0) {
	if (msg_verbose)
	    msg_info("%s: no destination cache: dest_label=%s",
		     myname, dest_label);
	return (-1);
    }

    /*
     * Search endpoints for the first available session.
     */
    RING_FOREACH(ring, head->ring) {
	dest = RING_TO_MULTI_DEST(ring);
	fd = scache_multi_find_endp(scache, dest->endp_label, endp_prop);
	if (fd >= 0) {
	    vstring_strcpy(dest_prop, dest->dest_prop);
	    return (fd);
	}
    }
    if (msg_verbose)
	msg_info("%s: not found: dest_label=%s", myname, dest_label);
    return (-1);
}

/* scache_multi_size - size of multi-element cache object */

static void scache_multi_size(SCACHE *scache, SCACHE_SIZE *size)
{
    SCACHE_MULTI *sp = (SCACHE_MULTI *) scache;

    size->dest_count = sp->dest_cache->used;
    size->endp_count = sp->endp_cache->used;
    size->sess_count = sp->sess_count;
}

/* scache_multi_free - destroy multi-element cache object */

static void scache_multi_free(SCACHE *scache)
{
    SCACHE_MULTI *sp = (SCACHE_MULTI *) scache;

    htable_free(sp->dest_cache, scache_multi_free_dest);
    htable_free(sp->endp_cache, scache_multi_free_endp);

    myfree((char *) sp);
}

/* scache_multi_create - initialize */

SCACHE *scache_multi_create(void)
{
    SCACHE_MULTI *sp = (SCACHE_MULTI *) mymalloc(sizeof(*sp));

    sp->scache->save_endp = scache_multi_save_endp;
    sp->scache->find_endp = scache_multi_find_endp;
    sp->scache->save_dest = scache_multi_save_dest;
    sp->scache->find_dest = scache_multi_find_dest;
    sp->scache->size = scache_multi_size;
    sp->scache->free = scache_multi_free;

    sp->dest_cache = htable_create(1);
    sp->endp_cache = htable_create(1);
    sp->sess_count = 0;

    return (sp->scache);
}
