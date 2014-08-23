/*	$NetBSD: lstRemove.c,v 1.15 2014/08/23 15:05:40 christos Exp $	*/

/*
 * Copyright (c) 1988, 1989, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Adam de Boor.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef MAKE_NATIVE
static char rcsid[] = "$NetBSD: lstRemove.c,v 1.15 2014/08/23 15:05:40 christos Exp $";
#else
#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)lstRemove.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: lstRemove.c,v 1.15 2014/08/23 15:05:40 christos Exp $");
#endif
#endif /* not lint */
#endif

/*-
 * LstRemove.c --
 *	Remove an element from a list
 */

#include	"lstInt.h"

/*-
 *-----------------------------------------------------------------------
 * Lst_Remove --
 *	Remove the given node from the given list.
 *
 * Results:
 *	SUCCESS or FAILURE.
 *
 * Side Effects:
 *	The list's firstPtr will be set to NULL if ln is the last
 *	node on the list. firsPtr and lastPtr will be altered if ln is
 *	either the first or last node, respectively, on the list.
 *	If the list is open for sequential access and the removed node
 *	is the current one, the current node is reset to the previous one.
 *
 *-----------------------------------------------------------------------
 */
ReturnStatus
Lst_Remove(Lst l, LstNode ln)
{
    List 	list = l;
    ListNode	lNode = ln;

    if (!LstValid (l) ||
	!LstNodeValid (ln, l)) {
	    return (FAILURE);
    }

    /*
     * unlink it from the list
     */
    if (lNode->nextPtr != NULL) {
	lNode->nextPtr->prevPtr = lNode->prevPtr;
    }
    if (lNode->prevPtr != NULL) {
	lNode->prevPtr->nextPtr = lNode->nextPtr;
    }

    /*
     * if either the firstPtr or lastPtr of the list point to this node,
     * adjust them accordingly
     */
    if (list->firstPtr == lNode) {
	list->firstPtr = lNode->nextPtr;
    }
    if (list->lastPtr == lNode) {
	list->lastPtr = lNode->prevPtr;
    }

    /*
     * Sequential access stuff. If the node we're removing is the current
     * node in the list, reset the current node to the previous one. If the
     * previous one is non-existent, the removed item is the first item of
     * a non-circular list and we have to adjust atEnd while keeping in
     * mind that it might have also been the only item.
     */
    if (list->isOpen && list->curPtr == lNode) {
	if (list->curPtr->prevPtr == NULL) {
	    if (list->curPtr->nextPtr == NULL)
		list->atEnd = Head;	/* only item */
	    else
		list->atEnd = Unknown;
	}

	list->curPtr = list->curPtr->prevPtr;
    }

    /*
     * The only way firstPtr can still point to ln is if ln was the last
     * node of a circular list and thus lNode->nextptr == lNode.  Therefore,
     * the list has now become empty and is marked as such.
     */
    if (list->firstPtr == lNode) {
	list->firstPtr = NULL;
	if (list->isOpen) {
	    /* stop sequential access too */
	    list->curPtr = NULL;
	    list->atEnd = Head;
	}
    }

    /*
     * note that the datum is unmolested. The caller must free it as
     * necessary and as expected.
     */
    if (lNode->useCount == 0) {
	free(ln);
    } else {
	lNode->flags |= LN_DELETED;
    }

    return (SUCCESS);
}

#if 0
/*
 * This was implemented, but turned out to be unnecessary.  Put it into
 * version control, remove later and leave a note, so the effort is not
 * wasted and it can be revived later if needed.
 */

/*-
 *-----------------------------------------------------------------------
 * Lst_RemoveSlice -- remove the given interval from the given list.
 *
 * Remove all items between s and e, inclusive.  If s == e, only one
 * item is removed.  freeProc is invoked on all removed items, if it is
 * provided.
 *
 * If the first item of the list falls into the removed range,
 * the start is moved to the first item after the removed range.
 * If the last item of the list or the current sequential access
 * position falls into the removed range, the it is moved to
 * the first item before the removed range.
 *
 * If the list becomes totally empty, the special locations are
 * also set properly.
 *
 * Pre-condition:
 * 	In a non-circular list, the s must not be after e.
 *
 * Input:
 *	l	list to remove the items from
 *	s	first item to remove
 *	e	last item to remove
 *	freeProc	when non-NULL, invoked for each removed item as
 *			(*freeProc)(Lst_Datum(item), data)
 *      data	extra parameters for freeProc
 *
 * Results:
 *      FAILURE if any parameter is invalid, SUCCESS otherwise.
 *-----------------------------------------------------------------------
 */
ReturnStatus
Lst_RemoveSlice(Lst l, LstNode s, LstNode e, FreeProc2 *freeProc, void *data)
{
    List 	list;		/* list being processerd */
    ListNode	end, start;	/* endpoints of range to remove */
    ListNode	predecessor, successor;	/* start->prevPtr and end->nextPtr */
    ListNode	i, iNext;	/* iterators into the range */

    /* flags to recognize when special nodes are removed */
    Boolean	firstRemoved, lastRemoved, curRemoved;

    if (!LstValid (l) || !LstNodeValid (s, l) || !LstNodeValid(e, l))
	return FAILURE;

    list = l;
    start = s;
    end = e;
    predecessor = start->prevPtr;
    successor = end->nextPtr;

    /* Remove the slice from the list. */
    if (predecessor != NULL)
	predecessor->nextPtr = successor;
    if (successor != NULL)
	successor->prevPtr = predecessor;

    /*
     * Destroy the removed items.  While removing items, take
     * note if any of the special nodes get removed so we can fix
     * the situation afterwards.
     */
    iNext = start;
    do {
	i = iNext;

	if (list->firstPtr == i)
	    firstRemoved = TRUE;
	if (list->lastPtr == i)
	    lastRemoved = TRUE;
	if (list->curPtr == i)
	    curRemoved = TRUE;

	iNext = i->nextPtr;
	if (i->useCount == 0) {
	    if (freeProc != NULL)
		(*freeProc)(i->datum, data);
	    free(i);
	} else
	    i->flags |= LN_DELETED;
    } while (i != end);

    /* Adjust the special pointers. */
    if (firstRemoved && lastRemoved)
	predecessor = successor = NULL;

    if (firstRemoved)
	list->firstPtr = successor;

    if (lastRemoved)
	list->lastPtr = predecessor;

    if (list->isOpen && curRemoved) {
	list->curPtr = predecessor;
	if (list->curPtr == NULL)
	    list->atEnd = (LstIsEmpty(list) ? Head : Unknown);
    }

    return SUCCESS;
}
*/
#endif
