/* $NetBSD: lst.c,v 1.10 2020/08/21 04:42:02 rillig Exp $ */

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
static char rcsid[] = "$NetBSD: lst.c,v 1.10 2020/08/21 04:42:02 rillig Exp $";
#else
#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: lst.c,v 1.10 2020/08/21 04:42:02 rillig Exp $");
#endif /* not lint */
#endif

typedef struct ListNode {
    struct ListNode *prevPtr;	/* previous element in list */
    struct ListNode *nextPtr;	/* next in list */
    uint8_t useCount;		/* Count of functions using the node.
				 * node may not be deleted until count
				 * goes to 0 */
    Boolean deleted;		/* List node should be removed when done */
    void *datum;		/* datum associated with this element */
} *ListNode;

typedef enum {
    Head, Middle, Tail, Unknown
} Where;

typedef struct List {
    ListNode firstPtr;		/* first node in list */
    ListNode lastPtr;		/* last node in list */
/*
 * fields for sequential access
 */
    Where atEnd;		/* Where in the list the last access was */
    Boolean isOpen;		/* true if list has been Lst_Open'ed */
    ListNode curPtr;		/* current node, if open. NULL if
				 * *just* opened */
    ListNode prevPtr;		/* Previous node, if open. Used by
				 * Lst_Remove */
} *List;

/*
 * PAlloc (var, ptype) --
 *	Allocate a pointer-typedef structure 'ptype' into the variable 'var'
 */
#define PAlloc(var, ptype) \
    var = (ptype) bmake_malloc(sizeof *(var))

/*
 * LstValid --
 *	Return TRUE if the list is valid
 */
static Boolean
LstValid(Lst l)
{
    return l != NULL;
}

/*
 * LstNodeValid --
 *	Return TRUE if the list node is valid
 */
static Boolean
LstNodeValid(LstNode ln)
{
    return ln != NULL;
}

/*
 * LstIsEmpty (l) --
 *	TRUE if the list l is empty.
 */
static Boolean
LstIsEmpty(Lst l)
{
    return l->firstPtr == NULL;
}

/* Create and initialize a new, empty list. */
Lst
Lst_Init(void)
{
    List nList;

    PAlloc (nList, List);

    nList->firstPtr = NULL;
    nList->lastPtr = NULL;
    nList->isOpen = FALSE;
    nList->atEnd = Unknown;

    return nList;
}

/*-
 *-----------------------------------------------------------------------
 * Lst_Duplicate --
 *	Duplicate an entire list. If a function to copy a void *is
 *	given, the individual client elements will be duplicated as well.
 *
 * Input:
 *	l		the list to duplicate
 *	copyProc	A function to duplicate each void *
 *
 * Results:
 *	The new Lst structure or NULL if failure.
 *
 * Side Effects:
 *	A new list is created.
 *-----------------------------------------------------------------------
 */
Lst
Lst_Duplicate(Lst l, DuplicateProc *copyProc)
{
    Lst nl;
    ListNode ln;
    List list = l;

    if (!LstValid(l)) {
	return NULL;
    }

    nl = Lst_Init();
    if (nl == NULL) {
	return NULL;
    }

    ln = list->firstPtr;
    while (ln != NULL) {
	if (copyProc != NULL) {
	    if (Lst_AtEnd(nl, copyProc(ln->datum)) == FAILURE) {
		return NULL;
	    }
	} else if (Lst_AtEnd(nl, ln->datum) == FAILURE) {
	    return NULL;
	}

	ln = ln->nextPtr;
    }

    return nl;
}

/*-
 *-----------------------------------------------------------------------
 * Lst_Destroy --
 *	Destroy a list and free all its resources. If the freeProc is
 *	given, it is called with the datum from each node in turn before
 *	the node is freed.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The given list is freed in its entirety.
 *
 *-----------------------------------------------------------------------
 */
void
Lst_Destroy(Lst list, FreeProc *freeProc)
{
    ListNode ln;
    ListNode tln = NULL;

    if (list == NULL)
	return;

    /* To ease scanning */
    if (list->lastPtr != NULL)
	list->lastPtr->nextPtr = NULL;
    else {
	free(list);
	return;
    }

    if (freeProc) {
	for (ln = list->firstPtr; ln != NULL; ln = tln) {
	    tln = ln->nextPtr;
	    freeProc(ln->datum);
	    free(ln);
	}
    } else {
	for (ln = list->firstPtr; ln != NULL; ln = tln) {
	    tln = ln->nextPtr;
	    free(ln);
	}
    }

    free(list);
}

/*
 * Functions to modify a list
 */

/*-
 *-----------------------------------------------------------------------
 * Lst_InsertBefore --
 *	Insert a new node with the given piece of data before the given
 *	node in the given list.
 *
 * Input:
 *	l		list to manipulate
 *	ln		node before which to insert d
 *	d		datum to be inserted
 *
 * Results:
 *	SUCCESS or FAILURE.
 *
 * Side Effects:
 *	the firstPtr field will be changed if ln is the first node in the
 *	list.
 *
 *-----------------------------------------------------------------------
 */
ReturnStatus
Lst_InsertBefore(Lst l, LstNode ln, void *d)
{
    ListNode nLNode;		/* new lnode for d */
    ListNode lNode = ln;
    List list = l;


    /*
     * check validity of arguments
     */
    if (LstValid(l) && (LstIsEmpty(l) && ln == NULL))
	goto ok;

    if (!LstValid(l) || LstIsEmpty(l) || !LstNodeValid(ln)) {
	return FAILURE;
    }

    ok:
    PAlloc (nLNode, ListNode);

    nLNode->datum = d;
    nLNode->useCount = 0;
    nLNode->deleted = FALSE;

    if (ln == NULL) {
	nLNode->prevPtr = nLNode->nextPtr = NULL;
	list->firstPtr = list->lastPtr = nLNode;
    } else {
	nLNode->prevPtr = lNode->prevPtr;
	nLNode->nextPtr = lNode;

	if (nLNode->prevPtr != NULL) {
	    nLNode->prevPtr->nextPtr = nLNode;
	}
	lNode->prevPtr = nLNode;

	if (lNode == list->firstPtr) {
	    list->firstPtr = nLNode;
	}
    }

    return SUCCESS;
}

/*-
 *-----------------------------------------------------------------------
 * Lst_InsertAfter --
 *	Create a new node and add it to the given list after the given node.
 *
 * Input:
 *	l		affected list
 *	ln		node after which to append the datum
 *	d		said datum
 *
 * Results:
 *	SUCCESS if all went well.
 *
 * Side Effects:
 *	A new ListNode is created and linked in to the List. The lastPtr
 *	field of the List will be altered if ln is the last node in the
 *	list. lastPtr and firstPtr will alter if the list was empty and
 *	ln was NULL.
 *
 *-----------------------------------------------------------------------
 */
ReturnStatus
Lst_InsertAfter(Lst l, LstNode ln, void *d)
{
    List list;
    ListNode lNode;
    ListNode nLNode;

    if (LstValid(l) && (ln == NULL && LstIsEmpty(l))) {
	goto ok;
    }

    if (!LstValid(l) || LstIsEmpty(l) || !LstNodeValid(ln)) {
	return FAILURE;
    }
    ok:

    list = l;
    lNode = ln;

    PAlloc (nLNode, ListNode);
    nLNode->datum = d;
    nLNode->useCount = 0;
    nLNode->deleted = FALSE;

    if (lNode == NULL) {
	nLNode->nextPtr = nLNode->prevPtr = NULL;
	list->firstPtr = list->lastPtr = nLNode;
    } else {
	nLNode->prevPtr = lNode;
	nLNode->nextPtr = lNode->nextPtr;

	lNode->nextPtr = nLNode;
	if (nLNode->nextPtr != NULL) {
	    nLNode->nextPtr->prevPtr = nLNode;
	}

	if (lNode == list->lastPtr) {
	    list->lastPtr = nLNode;
	}
    }

    return SUCCESS;
}

/*-
 *-----------------------------------------------------------------------
 * Lst_AtFront --
 *	Place a piece of data at the front of a list
 *
 * Results:
 *	SUCCESS or FAILURE
 *
 * Side Effects:
 *	A new ListNode is created and stuck at the front of the list.
 *	hence, firstPtr (and possible lastPtr) in the list are altered.
 *
 *-----------------------------------------------------------------------
 */
ReturnStatus
Lst_AtFront(Lst l, void *d)
{
    LstNode front;

    front = Lst_First(l);
    return Lst_InsertBefore(l, front, d);
}

/*-
 *-----------------------------------------------------------------------
 * Lst_AtEnd --
 *	Add a node to the end of the given list
 *
 * Input:
 *	l		List to which to add the datum
 *	d		Datum to add
 *
 * Results:
 *	SUCCESS if life is good.
 *
 * Side Effects:
 *	A new ListNode is created and added to the list.
 *
 *-----------------------------------------------------------------------
 */
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
    List list = l;
    ListNode lNode = ln;

    assert(LstValid(l));
    assert(LstNodeValid(ln));

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
     * previous one was non-existent (prevPtr == NULL), we set the
     * end to be Unknown, since it is.
     */
    if (list->isOpen && (list->curPtr == lNode)) {
	list->curPtr = list->prevPtr;
	if (list->curPtr == NULL) {
	    list->atEnd = Unknown;
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

/*-
 *-----------------------------------------------------------------------
 * Lst_First --
 *	Return the first node on the given list.
 *
 * Results:
 *	The first node or NULL if the list is empty.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
LstNode
Lst_First(Lst l)
{
    if (!LstValid(l) || LstIsEmpty(l)) {
	return NULL;
    } else {
	return l->firstPtr;
    }
}

/*-
 *-----------------------------------------------------------------------
 * Lst_Last --
 *	Return the last node on the list l.
 *
 * Results:
 *	The requested node or NULL if the list is empty.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
LstNode
Lst_Last(Lst l)
{
    if (!LstValid(l) || LstIsEmpty(l)) {
	return NULL;
    } else {
	return l->lastPtr;
    }
}

/* Return the successor to the given node on its list, or NULL. */
LstNode
Lst_Succ(LstNode ln)
{
    if (ln == NULL) {
	return NULL;
    } else {
	return ln->nextPtr;
    }
}

/* Return the predecessor to the given node on its list, or NULL. */
LstNode
Lst_Prev(LstNode ln)
{
    if (ln == NULL) {
	return NULL;
    } else {
	return ln->prevPtr;
    }
}

/*-
 *-----------------------------------------------------------------------
 * Lst_Datum --
 *	Return the datum stored in the given node.
 *
 * Results:
 *	The datum or NULL if the node is invalid.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
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

/*-
 *-----------------------------------------------------------------------
 * Lst_IsEmpty --
 *	Return TRUE if the given list is empty.
 *
 * Results:
 *	TRUE if the list is empty, FALSE otherwise.
 *
 * Side Effects:
 *	None.
 *
 *	A list is considered empty if its firstPtr == NULL (or if
 *	the list itself is NULL).
 *-----------------------------------------------------------------------
 */
Boolean
Lst_IsEmpty(Lst l)
{
    return !LstValid(l) || LstIsEmpty(l);
}

/*-
 *-----------------------------------------------------------------------
 * Lst_Find --
 *	Find a node on the given list using the given comparison function
 *	and the given datum.
 *
 * Results:
 *	The found node or NULL if none matches.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
LstNode
Lst_Find(Lst l, const void *d, int (*cProc)(const void *, const void *))
{
    return Lst_FindFrom(l, Lst_First(l), d, cProc);
}

/*-
 *-----------------------------------------------------------------------
 * Lst_FindFrom --
 *	Search for a node starting and ending with the given one on the
 *	given list using the passed datum and comparison function to
 *	determine when it has been found.
 *
 * Results:
 *	The found node or NULL
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
LstNode
Lst_FindFrom(Lst l, LstNode ln, const void *d,
	     int (*cProc)(const void *, const void *))
{
    ListNode tln;

    if (!LstValid(l) || LstIsEmpty(l) || !LstNodeValid(ln)) {
	return NULL;
    }

    tln = ln;

    do {
	if ((*cProc)(tln->datum, d) == 0)
	    return tln;
	tln = tln->nextPtr;
    } while (tln != ln && tln != NULL);

    return NULL;
}

/*-
 * See if a given datum is on a given list.
 */
LstNode
Lst_Member(Lst l, void *d)
{
    List list = l;
    ListNode lNode;

    if (list == NULL) {
	return NULL;
    }
    lNode = list->firstPtr;
    if (lNode == NULL) {
	return NULL;
    }

    do {
	if (lNode->datum == d) {
	    return lNode;
	}
	lNode = lNode->nextPtr;
    } while (lNode != NULL && lNode != list->firstPtr);

    return NULL;
}

/*-
 *-----------------------------------------------------------------------
 * Lst_ForEach --
 *	Apply the given function to each element of the given list. The
 *	function should return 0 if Lst_ForEach should continue and non-
 *	zero if it should abort.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Only those created by the passed-in function.
 *
 *-----------------------------------------------------------------------
 */
/*VARARGS2*/
int
Lst_ForEach(Lst l, int (*proc)(void *, void *), void *d)
{
    return Lst_ForEachFrom(l, Lst_First(l), proc, d);
}

/*-
 *-----------------------------------------------------------------------
 * Lst_ForEachFrom --
 *	Apply the given function to each element of the given list,
 *	starting from a given point.
 *
 *	The function should return 0 if traversal should continue, and
 *	non-zero if it should abort.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Only those created by the passed-in function.
 *
 *-----------------------------------------------------------------------
 */
/*VARARGS2*/
int
Lst_ForEachFrom(Lst l, LstNode ln, int (*proc)(void *, void *),
		void *d)
{
    ListNode tln = ln;
    List list = l;
    ListNode next;
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

	next = tln->nextPtr;

	/*
	 * We're done with the traversal if
	 *  - the next node to examine is the first in the queue or
	 *    doesn't exist and
	 *  - nothing's been added after the current node (check this
	 *    after proc() has been called).
	 */
	done = (next == NULL || next == list->firstPtr);

	(void)tln->useCount++;
	result = (*proc)(tln->datum, d);
	(void)tln->useCount--;

	/*
	 * Now check whether a node has been added.
	 * Note: this doesn't work if this node was deleted before
	 *       the new node was added.
	 */
	if (next != tln->nextPtr) {
	    next = tln->nextPtr;
	    done = 0;
	}

	if (tln->deleted) {
	    free((char *)tln);
	}
	tln = next;
    } while (!result && !LstIsEmpty(list) && !done);

    return result;
}

/*-
 *-----------------------------------------------------------------------
 * Lst_Concat --
 *	Concatenate two lists. New elements are created to hold the data
 *	elements, if specified, but the elements themselves are not copied.
 *	If the elements should be duplicated to avoid confusion with another
 *	list, the Lst_Duplicate function should be called first.
 *	If LST_CONCLINK is specified, the second list is destroyed since
 *	its pointers have been corrupted and the list is no longer useable.
 *
 * Input:
 *	l1		The list to which l2 is to be appended
 *	l2		The list to append to l1
 *	flags		LST_CONCNEW if LstNode's should be duplicated
 *			LST_CONCLINK if should just be relinked
 *
 * Results:
 *	SUCCESS if all went well. FAILURE otherwise.
 *
 * Side Effects:
 *	New elements are created and appended the first list.
 *-----------------------------------------------------------------------
 */
ReturnStatus
Lst_Concat(Lst l1, Lst l2, int flags)
{
    ListNode ln;     /* original LstNode */
    ListNode nln;    /* new LstNode */
    ListNode last;   /* the last element in the list. Keeps
				 * bookkeeping until the end */
    List list1 = l1;
    List list2 = l2;

    if (!LstValid(l1) || !LstValid(l2)) {
	return FAILURE;
    }

    if (flags == LST_CONCLINK) {
	if (list2->firstPtr != NULL) {
	    /*
	     * So long as the second list isn't empty, we just link the
	     * first element of the second list to the last element of the
	     * first list. If the first list isn't empty, we then link the
	     * last element of the list to the first element of the second list
	     * The last element of the second list, if it exists, then becomes
	     * the last element of the first list.
	     */
	    list2->firstPtr->prevPtr = list1->lastPtr;
	    if (list1->lastPtr != NULL) {
		list1->lastPtr->nextPtr = list2->firstPtr;
	    } else {
		list1->firstPtr = list2->firstPtr;
	    }
	    list1->lastPtr = list2->lastPtr;
	}
	free(l2);
    } else if (list2->firstPtr != NULL) {
	/*
	 * We set the nextPtr of the last element of list 2 to be nil to make
	 * the loop less difficult. The loop simply goes through the entire
	 * second list creating new LstNodes and filling in the nextPtr, and
	 * prevPtr to fit into l1 and its datum field from the
	 * datum field of the corresponding element in l2. The 'last' node
	 * follows the last of the new nodes along until the entire l2 has
	 * been appended. Only then does the bookkeeping catch up with the
	 * changes. During the first iteration of the loop, if 'last' is nil,
	 * the first list must have been empty so the newly-created node is
	 * made the first node of the list.
	 */
	list2->lastPtr->nextPtr = NULL;
	for (last = list1->lastPtr, ln = list2->firstPtr;
	     ln != NULL;
	     ln = ln->nextPtr)
	{
	    PAlloc (nln, ListNode);
	    nln->datum = ln->datum;
	    if (last != NULL) {
		last->nextPtr = nln;
	    } else {
		list1->firstPtr = nln;
	    }
	    nln->prevPtr = last;
	    nln->useCount = 0;
	    nln->deleted = FALSE;
	    last = nln;
	}

	/*
	 * Finish bookkeeping. The last new element becomes the last element
	 * of list one.
	 */
	list1->lastPtr = last;
	last->nextPtr = NULL;
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

/*-
 *-----------------------------------------------------------------------
 * Lst_Open --
 *	Open a list for sequential access. A list can still be searched,
 *	etc., without confusing these functions.
 *
 * Results:
 *	SUCCESS or FAILURE.
 *
 * Side Effects:
 *	isOpen is set TRUE and curPtr is set to NULL so the
 *	other sequential functions know it was just opened and can choose
 *	the first element accessed based on this.
 *
 *-----------------------------------------------------------------------
 */
ReturnStatus
Lst_Open(Lst l)
{
    if (LstValid(l) == FALSE) {
	return FAILURE;
    }
    l->isOpen = TRUE;
    l->atEnd = LstIsEmpty(l) ? Head : Unknown;
    l->curPtr = NULL;

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
    l->atEnd = LstIsEmpty(l) ? Head : Unknown;
    l->curPtr = NULL;
}

/* Return the next node for the given list, or NULL if the end has been
 * reached. */
LstNode
Lst_NextS(Lst l)
{
    ListNode tln;
    List list = l;

    assert(LstValid(l));
    assert(list->isOpen);

    list->prevPtr = list->curPtr;

    if (list->curPtr == NULL) {
	if (list->atEnd == Unknown) {
	    /*
	     * If we're just starting out, atEnd will be Unknown.
	     * Then we want to start this thing off in the right
	     * direction -- at the start with atEnd being Middle.
	     */
	    list->curPtr = tln = list->firstPtr;
	    list->atEnd = Middle;
	} else {
	    tln = NULL;
	    list->atEnd = Tail;
	}
    } else {
	tln = list->curPtr->nextPtr;
	list->curPtr = tln;

	if (tln == list->firstPtr || tln == NULL) {
	    /*
	     * If back at the front, then we've hit the end...
	     */
	    list->atEnd = Tail;
	} else {
	    /*
	     * Reset to Middle if gone past first.
	     */
	    list->atEnd = Middle;
	}
    }

    return tln;
}

/* Close a list which was opened for sequential access. */
void
Lst_CloseS(Lst l)
{
    List list = l;

    assert(LstValid(l));
    assert(list->isOpen);
    list->isOpen = FALSE;
    list->atEnd = Unknown;
}


/*
 * for using the list as a queue
 */

/*-
 *-----------------------------------------------------------------------
 * Lst_EnQueue --
 *	Add the datum to the tail of the given list.
 *
 * Results:
 *	SUCCESS or FAILURE as returned by Lst_InsertAfter.
 *
 * Side Effects:
 *	the lastPtr field is altered all the time and the firstPtr field
 *	will be altered if the list used to be empty.
 *
 *-----------------------------------------------------------------------
 */
ReturnStatus
Lst_EnQueue(Lst l, void *d)
{
    if (LstValid(l) == FALSE) {
	return FAILURE;
    }

    return Lst_InsertAfter(l, Lst_Last(l), d);
}

/*-
 *-----------------------------------------------------------------------
 * Lst_DeQueue --
 *	Remove and return the datum at the head of the given list.
 *
 * Results:
 *	The datum in the node at the head or NULL if the list
 *	is empty.
 *
 * Side Effects:
 *	The head node is removed from the list.
 *
 *-----------------------------------------------------------------------
 */
void *
Lst_DeQueue(Lst l)
{
    void *rd;
    ListNode tln;

    tln = Lst_First(l);
    if (tln == NULL) {
	return NULL;
    }

    rd = tln->datum;
    Lst_RemoveS(l, tln);
    return rd;
}
