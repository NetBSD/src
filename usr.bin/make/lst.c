/* $NetBSD: lst.c,v 1.15 2020/08/21 06:38:29 rillig Exp $ */

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

#include <assert.h>

#include "lst.h"
#include "make_malloc.h"

#ifndef MAKE_NATIVE
static char rcsid[] = "$NetBSD: lst.c,v 1.15 2020/08/21 06:38:29 rillig Exp $";
#else
#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: lst.c,v 1.15 2020/08/21 06:38:29 rillig Exp $");
#endif /* not lint */
#endif

struct ListNode {
    struct ListNode *prev;	/* previous element in list */
    struct ListNode *next;	/* next in list */
    uint8_t useCount;		/* Count of functions using the node.
				 * node may not be deleted until count
				 * goes to 0 */
    Boolean deleted;		/* List node should be removed when done */
    void *datum;		/* datum associated with this element */
};

typedef enum {
    Head, Middle, Tail, Unknown
} Where;

struct List {
    LstNode first;		/* first node in list */
    LstNode last;		/* last node in list */
/*
 * fields for sequential access
 */
    Where lastAccess;		/* Where in the list the last access was */
    Boolean isOpen;		/* true if list has been Lst_Open'ed */
    LstNode curr;		/* current node, if open. NULL if
				 * *just* opened */
    LstNode prev;		/* Previous node, if open. Used by
				 * Lst_Remove */
};

/* Return TRUE if the list is valid. */
static Boolean
LstValid(Lst l)
{
    return l != NULL;
}

/* Return TRUE if the list node is valid. */
static Boolean
LstNodeValid(LstNode ln)
{
    return ln != NULL;
}

static LstNode
LstNodeNew(void *datum)
{
    LstNode ln = bmake_malloc(sizeof *ln);
    /* prev will be initialized by the calling code. */
    /* next will be initialized by the calling code. */
    ln->useCount = 0;
    ln->deleted = FALSE;
    ln->datum = datum;
    return ln;
}

/* Return TRUE if the list is empty. */
static Boolean
LstIsEmpty(Lst l)
{
    return l->first == NULL;
}

/* Create and initialize a new, empty list. */
Lst
Lst_Init(void)
{
    Lst nList = bmake_malloc(sizeof *nList);

    nList->first = NULL;
    nList->last = NULL;
    nList->isOpen = FALSE;
    nList->lastAccess = Unknown;

    return nList;
}

/* Duplicate an entire list, usually by copying the datum pointers.
 * If copyProc is given, that function is used to create the new datum from the
 * old datum, usually by creating a copy of it.
 * Return the new list, or NULL on failure. */
Lst
Lst_Duplicate(Lst l, DuplicateProc *copyProc)
{
    Lst nl;
    LstNode ln;
    Lst list = l;

    if (!LstValid(l)) {
	return NULL;
    }

    nl = Lst_Init();
    if (nl == NULL) {
	return NULL;
    }

    ln = list->first;
    while (ln != NULL) {
	if (copyProc != NULL) {
	    if (Lst_AtEnd(nl, copyProc(ln->datum)) == FAILURE) {
		return NULL;
	    }
	} else if (Lst_AtEnd(nl, ln->datum) == FAILURE) {
	    return NULL;
	}

	ln = ln->next;
    }

    return nl;
}

/* Destroy a list and free all its resources. If the freeProc is given, it is
 * called with the datum from each node in turn before the node is freed. */
void
Lst_Destroy(Lst list, FreeProc *freeProc)
{
    LstNode ln;
    LstNode tln = NULL;

    if (list == NULL)
	return;

    /* To ease scanning */
    if (list->last != NULL)
	list->last->next = NULL;
    else {
	free(list);
	return;
    }

    if (freeProc) {
	for (ln = list->first; ln != NULL; ln = tln) {
	    tln = ln->next;
	    freeProc(ln->datum);
	    free(ln);
	}
    } else {
	for (ln = list->first; ln != NULL; ln = tln) {
	    tln = ln->next;
	    free(ln);
	}
    }

    free(list);
}

/*
 * Functions to modify a list
 */

/* Insert a new node with the given piece of data before the given node in the
 * given list. */
ReturnStatus
Lst_InsertBefore(Lst l, LstNode ln, void *d)
{
    LstNode nLNode;		/* new lnode for d */
    LstNode lNode = ln;
    Lst list = l;


    /*
     * check validity of arguments
     */
    if (LstValid(l) && (LstIsEmpty(l) && ln == NULL))
	goto ok;

    if (!LstValid(l) || LstIsEmpty(l) || !LstNodeValid(ln)) {
	return FAILURE;
    }

    ok:
    nLNode = LstNodeNew(d);

    if (ln == NULL) {
	nLNode->prev = nLNode->next = NULL;
	list->first = list->last = nLNode;
    } else {
	nLNode->prev = lNode->prev;
	nLNode->next = lNode;

	if (nLNode->prev != NULL) {
	    nLNode->prev->next = nLNode;
	}
	lNode->prev = nLNode;

	if (lNode == list->first) {
	    list->first = nLNode;
	}
    }

    return SUCCESS;
}

/* Insert a new node with the given piece of data after the given node in the
 * given list. */
ReturnStatus
Lst_InsertAfter(Lst l, LstNode ln, void *d)
{
    Lst list;
    LstNode lNode;
    LstNode nLNode;

    if (LstValid(l) && (ln == NULL && LstIsEmpty(l))) {
	goto ok;
    }

    if (!LstValid(l) || LstIsEmpty(l) || !LstNodeValid(ln)) {
	return FAILURE;
    }
    ok:

    list = l;
    lNode = ln;

    nLNode = LstNodeNew(d);

    if (lNode == NULL) {
	nLNode->next = nLNode->prev = NULL;
	list->first = list->last = nLNode;
    } else {
	nLNode->prev = lNode;
	nLNode->next = lNode->next;

	lNode->next = nLNode;
	if (nLNode->next != NULL) {
	    nLNode->next->prev = nLNode;
	}

	if (lNode == list->last) {
	    list->last = nLNode;
	}
    }

    return SUCCESS;
}

/* Add a piece of data at the front of the given list. */
ReturnStatus
Lst_AtFront(Lst l, void *d)
{
    LstNode front;

    front = Lst_First(l);
    return Lst_InsertBefore(l, front, d);
}

/* Add a piece of data at the end of the given list. */
ReturnStatus
Lst_AtEnd(Lst l, void *d)
{
    LstNode end;

    end = Lst_Last(l);
    return Lst_InsertAfter(l, end, d);
}

/* Remove the given node from the given list.
 * The datum stored in the node must be freed by the caller, if necessary. */
void
Lst_RemoveS(Lst l, LstNode ln)
{
    Lst list = l;
    LstNode lNode = ln;

    assert(LstValid(l));
    assert(LstNodeValid(ln));

    /*
     * unlink it from the list
     */
    if (lNode->next != NULL) {
	lNode->next->prev = lNode->prev;
    }
    if (lNode->prev != NULL) {
	lNode->prev->next = lNode->next;
    }

    /*
     * if either the first or last of the list point to this node,
     * adjust them accordingly
     */
    if (list->first == lNode) {
	list->first = lNode->next;
    }
    if (list->last == lNode) {
	list->last = lNode->prev;
    }

    /*
     * Sequential access stuff. If the node we're removing is the current
     * node in the list, reset the current node to the previous one. If the
     * previous one was non-existent (prev == NULL), we set the
     * end to be Unknown, since it is.
     */
    if (list->isOpen && (list->curr == lNode)) {
	list->curr = list->prev;
	if (list->curr == NULL) {
	    list->lastAccess = Unknown;
	}
    }

    /*
     * note that the datum is unmolested. The caller must free it as
     * necessary and as expected.
     */
    if (lNode->useCount == 0) {
	free(ln);
    } else {
	lNode->deleted = TRUE;
    }
}

/* Replace the datum in the given node with the new datum. */
void
Lst_ReplaceS(LstNode ln, void *d)
{
    ln->datum = d;
}


/*
 * Node-specific functions
 */

/* Return the first node from the given list, or NULL if the list is empty or
 * invalid. */
LstNode
Lst_First(Lst l)
{
    if (!LstValid(l) || LstIsEmpty(l)) {
	return NULL;
    } else {
	return l->first;
    }
}

/* Return the last node from the given list, or NULL if the list is empty or
 * invalid. */
LstNode
Lst_Last(Lst l)
{
    if (!LstValid(l) || LstIsEmpty(l)) {
	return NULL;
    } else {
	return l->last;
    }
}

/* Return the successor to the given node on its list, or NULL. */
LstNode
Lst_Succ(LstNode ln)
{
    if (ln == NULL) {
	return NULL;
    } else {
	return ln->next;
    }
}

/* Return the predecessor to the given node on its list, or NULL. */
LstNode
Lst_Prev(LstNode ln)
{
    if (ln == NULL) {
	return NULL;
    } else {
	return ln->prev;
    }
}

/* Return the datum stored in the given node, or NULL if the node is invalid. */
void *
Lst_Datum(LstNode ln)
{
    if (ln != NULL) {
	return ln->datum;
    } else {
	return NULL;
    }
}


/*
 * Functions for entire lists
 */

/* Return TRUE if the given list is empty or invalid. */
Boolean
Lst_IsEmpty(Lst l)
{
    return !LstValid(l) || LstIsEmpty(l);
}

/* Return the first node from the given list for which the given comparison
 * function returns 0, or NULL if none of the nodes matches. */
LstNode
Lst_Find(Lst l, const void *d, int (*cProc)(const void *, const void *))
{
    return Lst_FindFrom(l, Lst_First(l), d, cProc);
}

/* Return the first node from the given list, starting at the given node, for
 * which the given comparison function returns 0, or NULL if none of the nodes
 * matches. */
LstNode
Lst_FindFrom(Lst l, LstNode ln, const void *d,
	     int (*cProc)(const void *, const void *))
{
    LstNode tln;

    if (!LstValid(l) || LstIsEmpty(l) || !LstNodeValid(ln)) {
	return NULL;
    }

    tln = ln;

    do {
	if ((*cProc)(tln->datum, d) == 0)
	    return tln;
	tln = tln->next;
    } while (tln != ln && tln != NULL);

    return NULL;
}

/* Return the first node that contains the given datum, or NULL. */
LstNode
Lst_Member(Lst l, void *d)
{
    Lst list = l;
    LstNode lNode;

    if (list == NULL) {
	return NULL;
    }
    lNode = list->first;
    if (lNode == NULL) {
	return NULL;
    }

    do {
	if (lNode->datum == d) {
	    return lNode;
	}
	lNode = lNode->next;
    } while (lNode != NULL && lNode != list->first);

    return NULL;
}

/* Apply the given function to each element of the given list. The function
 * should return 0 if traversal should continue and non-zero if it should
 * abort. */
int
Lst_ForEach(Lst l, int (*proc)(void *, void *), void *d)
{
    return Lst_ForEachFrom(l, Lst_First(l), proc, d);
}

/* Apply the given function to each element of the given list, starting from
 * the given node. The function should return 0 if traversal should continue,
 * and non-zero if it should abort. */
int
Lst_ForEachFrom(Lst l, LstNode ln, int (*proc)(void *, void *),
		void *d)
{
    LstNode tln = ln;
    Lst list = l;
    LstNode next;
    Boolean done;
    int result;

    if (!LstValid(list) || LstIsEmpty(list)) {
	return 0;
    }

    do {
	/*
	 * Take care of having the current element deleted out from under
	 * us.
	 */

	next = tln->next;

	/*
	 * We're done with the traversal if
	 *  - the next node to examine is the first in the queue or
	 *    doesn't exist and
	 *  - nothing's been added after the current node (check this
	 *    after proc() has been called).
	 */
	done = (next == NULL || next == list->first);

	(void)tln->useCount++;
	result = (*proc)(tln->datum, d);
	(void)tln->useCount--;

	/*
	 * Now check whether a node has been added.
	 * Note: this doesn't work if this node was deleted before
	 *       the new node was added.
	 */
	if (next != tln->next) {
	    next = tln->next;
	    done = 0;
	}

	if (tln->deleted) {
	    free((char *)tln);
	}
	tln = next;
    } while (!result && !LstIsEmpty(list) && !done);

    return result;
}

/* Concatenate two lists. New nodes are created to hold the data elements,
 * if specified, but the data themselves are not copied. If the data
 * should be duplicated to avoid confusion with another list, the Lst_Duplicate
 * function should be called first. If LST_CONCLINK is specified, the second
 * list is destroyed since its pointers have been corrupted and the list is no
 * longer usable.
 *
 * Input:
 *	l1		The list to which l2 is to be appended
 *	l2		The list to append to l1
 *	flags		LST_CONCNEW if the list nodes should be duplicated
 *			LST_CONCLINK if the list nodes should just be relinked
 */
ReturnStatus
Lst_Concat(Lst l1, Lst l2, int flags)
{
    LstNode ln;     /* original LstNode */
    LstNode nln;    /* new LstNode */
    LstNode last;   /* the last element in the list. Keeps
				 * bookkeeping until the end */
    Lst list1 = l1;
    Lst list2 = l2;

    if (!LstValid(l1) || !LstValid(l2)) {
	return FAILURE;
    }

    if (flags == LST_CONCLINK) {
	if (list2->first != NULL) {
	    /*
	     * So long as the second list isn't empty, we just link the
	     * first element of the second list to the last element of the
	     * first list. If the first list isn't empty, we then link the
	     * last element of the list to the first element of the second list
	     * The last element of the second list, if it exists, then becomes
	     * the last element of the first list.
	     */
	    list2->first->prev = list1->last;
	    if (list1->last != NULL) {
		list1->last->next = list2->first;
	    } else {
		list1->first = list2->first;
	    }
	    list1->last = list2->last;
	}
	free(l2);
    } else if (list2->first != NULL) {
	/*
	 * We set the 'next' of the last element of list 2 to be nil to make
	 * the loop less difficult. The loop simply goes through the entire
	 * second list creating new LstNodes and filling in the 'next', and
	 * 'prev' to fit into l1 and its datum field from the
	 * datum field of the corresponding element in l2. The 'last' node
	 * follows the last of the new nodes along until the entire l2 has
	 * been appended. Only then does the bookkeeping catch up with the
	 * changes. During the first iteration of the loop, if 'last' is nil,
	 * the first list must have been empty so the newly-created node is
	 * made the first node of the list.
	 */
	list2->last->next = NULL;
	for (last = list1->last, ln = list2->first; ln != NULL; ln = ln->next) {
	    nln = LstNodeNew(ln->datum);
	    if (last != NULL) {
		last->next = nln;
	    } else {
		list1->first = nln;
	    }
	    nln->prev = last;
	    last = nln;
	}

	/*
	 * Finish bookkeeping. The last new element becomes the last element
	 * of list one.
	 */
	list1->last = last;
	last->next = NULL;
    }

    return SUCCESS;
}


/*
 * these functions are for dealing with a list as a table, of sorts.
 * An idea of the "current element" is kept and used by all the functions
 * between Lst_Open() and Lst_Close().
 *
 * The sequential functions access the list in a slightly different way.
 * CurPtr points to their idea of the current node in the list and they
 * access the list based on it.
 */

/* Open a list for sequential access. A list can still be searched, etc.,
 * without confusing these functions. */
ReturnStatus
Lst_Open(Lst l)
{
    if (LstValid(l) == FALSE) {
	return FAILURE;
    }
    l->isOpen = TRUE;
    l->lastAccess = LstIsEmpty(l) ? Head : Unknown;
    l->curr = NULL;

    return SUCCESS;
}

/* Open a list for sequential access. A list can still be searched, etc.,
 * without confusing these functions. */
void
Lst_OpenS(Lst l)
{
    assert(LstValid(l));
    assert(!l->isOpen);

    l->isOpen = TRUE;
    l->lastAccess = LstIsEmpty(l) ? Head : Unknown;
    l->curr = NULL;
}

/* Return the next node for the given list, or NULL if the end has been
 * reached. */
LstNode
Lst_NextS(Lst l)
{
    LstNode tln;
    Lst list = l;

    assert(LstValid(l));
    assert(list->isOpen);

    list->prev = list->curr;

    if (list->curr == NULL) {
	if (list->lastAccess == Unknown) {
	    /*
	     * If we're just starting out, lastAccess will be Unknown.
	     * Then we want to start this thing off in the right
	     * direction -- at the start with lastAccess being Middle.
	     */
	    list->curr = tln = list->first;
	    list->lastAccess = Middle;
	} else {
	    tln = NULL;
	    list->lastAccess = Tail;
	}
    } else {
	tln = list->curr->next;
	list->curr = tln;

	if (tln == list->first || tln == NULL) {
	    /*
	     * If back at the front, then we've hit the end...
	     */
	    list->lastAccess = Tail;
	} else {
	    /*
	     * Reset to Middle if gone past first.
	     */
	    list->lastAccess = Middle;
	}
    }

    return tln;
}

/* Close a list which was opened for sequential access. */
void
Lst_CloseS(Lst l)
{
    Lst list = l;

    assert(LstValid(l));
    assert(list->isOpen);
    list->isOpen = FALSE;
    list->lastAccess = Unknown;
}


/*
 * for using the list as a queue
 */

/* Add the datum to the tail of the given list. */
ReturnStatus
Lst_EnQueue(Lst l, void *d)
{
    if (LstValid(l) == FALSE) {
	return FAILURE;
    }

    return Lst_InsertAfter(l, Lst_Last(l), d);
}

/* Remove and return the datum at the head of the given list, or NULL if the
 * list is empty. */
void *
Lst_DeQueue(Lst l)
{
    void *rd;
    LstNode tln;

    tln = Lst_First(l);
    if (tln == NULL) {
	return NULL;
    }

    rd = tln->datum;
    Lst_RemoveS(l, tln);
    return rd;
}
