/*	$NetBSD: ring.c,v 1.1.1.1.2.2 2009/09/15 06:04:02 snj Exp $	*/

/*++
/* NAME
/*	ring 3
/* SUMMARY
/*	circular list management
/* SYNOPSIS
/*	#include <ring.h>
/*
/*	void	ring_init(list)
/*	RING	*list;
/*
/*	void	ring_prepend(list, element)
/*	RING	*list;
/*	RING	*element;
/*
/*	void	ring_append(list, element)
/*	RING	*list;
/*	RING	*element;
/*
/*	RING	*ring_pred(element)
/*	RING	*element;
/*
/*	RING	*ring_succ(element)
/*	RING	*element;
/*
/*	void	ring_detach(element)
/*	RING	*element;
/*
/*	RING_FOREACH(RING *element, RING *head)
/* DESCRIPTION
/*	This module manages circular, doubly-linked, lists. It provides
/*	operations to initialize a list, to add or remove an element,
/*	and to iterate over a list. Although the documentation appears
/*	to emphasize the special role of the list head, each operation
/*	can be applied to each list member.
/*
/*	Examples of applications: any sequence of objects such as queue,
/*	unordered list, or stack. Typically, an application embeds a RING
/*	structure into its own data structure, and uses the RING primitives
/*	to maintain the linkage between application-specific data objects.
/*
/*	ring_init() initializes its argument to a list of just one element.
/*
/*	ring_append() appends the named element to the named list head.
/*
/*	ring_prepend() prepends the named element to the named list head.
/*
/*	ring_succ() returns the list element that follows its argument.
/*
/*	ring_pred() returns the list element that precedes its argument.
/*
/*	ring_detach() disconnects a list element from its neighbors
/*	and closes the hole. This routine performs no implicit ring_init()
/*	on the removed element.
/*
/*	RING_FOREACH() is a macro that expands to a for (... ; ... ; ...)
/*	statement that iterates over each list element in forward order.
/*	Upon completion, the \fIelement\fR variable is set equal to
/*	\fIhead\fR. The list head itself is not treated as a list member.
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

/* System libraries. */

/* Application-specific. */

#include "ring.h"

/* ring_init - initialize ring head */

void    ring_init(ring)
RING   *ring;
{
    ring->pred = ring->succ = ring;
}

/* ring_append - insert entry after ring head */

void    ring_append(ring, entry)
RING   *ring;
RING   *entry;
{
    entry->succ = ring->succ;
    entry->pred = ring;
    ring->succ->pred = entry;
    ring->succ = entry;
}

/* ring_prepend - insert new entry before ring head */

void    ring_prepend(ring, entry)
RING   *ring;
RING   *entry;
{
    entry->pred = ring->pred;
    entry->succ = ring;
    ring->pred->succ = entry;
    ring->pred = entry;
}

/* ring_detach - remove entry from ring */

void    ring_detach(entry)
RING   *entry;
{
    RING   *succ = entry->succ;
    RING   *pred = entry->pred;

    pred->succ = succ;
    succ->pred = pred;

    entry->succ = entry->pred = 0;
}
