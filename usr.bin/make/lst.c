/* $NetBSD: lst.c,v 1.19 2020/08/22 00:13:16 rillig Exp $ */

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

#include "make.h"

#ifndef MAKE_NATIVE
static char rcsid[] = "$NetBSD: lst.c,v 1.19 2020/08/22 00:13:16 rillig Exp $";
#else
#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: lst.c,v 1.19 2020/08/22 00:13:16 rillig Exp $");
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
LstValid(Lst list)
{
    return list != NULL;
}

/* Return TRUE if the list node is valid. */
static Boolean
LstNodeValid(LstNode node)
{
    return node != NULL;
}

static LstNode
LstNodeNew(void *datum)
{
    LstNode node = bmake_malloc(sizeof *node);
    /* prev will be initialized by the calling code. */
    /* next will be initialized by the calling code. */
    node->useCount = 0;
    node->deleted = FALSE;
    node->datum = datum;
    return node;
}

/* Return TRUE if the list is empty. */
static Boolean
LstIsEmpty(Lst list)
{
    return list->first == NULL;
}

/* Create and initialize a new, empty list. */
Lst
Lst_Init(void)
{
    Lst list = bmake_malloc(sizeof *list);

    list->first = NULL;
    list->last = NULL;
    list->isOpen = FALSE;
    list->lastAccess = Unknown;

    return list;
}

/* Duplicate an entire list, usually by copying the datum pointers.
 * If copyProc is given, that function is used to create the new datum from the
 * old datum, usually by creating a copy of it.
 * Return the new list, or NULL on failure. */
Lst
Lst_Duplicate(Lst list, DuplicateProc *copyProc)
{
    Lst newList;
    LstNode node;

    if (!LstValid(list)) {
	return NULL;
    }

    newList = Lst_Init();

    node = list->first;
    while (node != NULL) {
	if (copyProc != NULL) {
	    if (Lst_AtEnd(newList, copyProc(node->datum)) == FAILURE) {
		return NULL;
	    }
	} else if (Lst_AtEnd(newList, node->datum) == FAILURE) {
	    return NULL;
	}

	node = node->next;
    }

    return newList;
}

/* Destroy a list and free all its resources. If the freeProc is given, it is
 * called with the datum from each node in turn before the node is freed. */
void
Lst_Destroy(Lst list, FreeProc *freeProc)
{
    LstNode node;
    LstNode next = NULL;

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
	for (node = list->first; node != NULL; node = next) {
	    next = node->next;
	    freeProc(node->datum);
	    free(node);
	}
    } else {
	for (node = list->first; node != NULL; node = next) {
	    next = node->next;
	    free(node);
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
Lst_InsertBefore(Lst list, LstNode node, void *datum)
{
    LstNode newNode;

    /*
     * check validity of arguments
     */
    if (LstValid(list) && (LstIsEmpty(list) && node == NULL))
	goto ok;

    if (!LstValid(list) || LstIsEmpty(list) || !LstNodeValid(node)) {
	return FAILURE;
    }

    ok:
    newNode = LstNodeNew(datum);

    if (node == NULL) {
	newNode->prev = newNode->next = NULL;
	list->first = list->last = newNode;
    } else {
	newNode->prev = node->prev;
	newNode->next = node;

	if (newNode->prev != NULL) {
	    newNode->prev->next = newNode;
	}
	node->prev = newNode;

	if (node == list->first) {
	    list->first = newNode;
	}
    }

    return SUCCESS;
}

/* Insert a new node with the given piece of data after the given node in the
 * given list. */
ReturnStatus
Lst_InsertAfter(Lst list, LstNode node, void *datum)
{
    LstNode newNode;

    if (LstValid(list) && (node == NULL && LstIsEmpty(list))) {
	goto ok;
    }

    if (!LstValid(list) || LstIsEmpty(list) || !LstNodeValid(node)) {
	return FAILURE;
    }
    ok:

    newNode = LstNodeNew(datum);

    if (node == NULL) {
	newNode->next = newNode->prev = NULL;
	list->first = list->last = newNode;
    } else {
	newNode->prev = node;
	newNode->next = node->next;

	node->next = newNode;
	if (newNode->next != NULL) {
	    newNode->next->prev = newNode;
	}

	if (node == list->last) {
	    list->last = newNode;
	}
    }

    return SUCCESS;
}

/* Add a piece of data at the front of the given list. */
ReturnStatus
Lst_AtFront(Lst list, void *datum)
{
    LstNode front = Lst_First(list);
    return Lst_InsertBefore(list, front, datum);
}

/* Add a piece of data at the end of the given list. */
ReturnStatus
Lst_AtEnd(Lst list, void *datum)
{
    LstNode end = Lst_Last(list);
    return Lst_InsertAfter(list, end, datum);
}

/* Remove the given node from the given list.
 * The datum stored in the node must be freed by the caller, if necessary. */
void
Lst_RemoveS(Lst list, LstNode node)
{
    assert(LstValid(list));
    assert(LstNodeValid(node));

    /*
     * unlink it from the list
     */
    if (node->next != NULL) {
	node->next->prev = node->prev;
    }
    if (node->prev != NULL) {
	node->prev->next = node->next;
    }

    /*
     * if either the first or last of the list point to this node,
     * adjust them accordingly
     */
    if (list->first == node) {
	list->first = node->next;
    }
    if (list->last == node) {
	list->last = node->prev;
    }

    /*
     * Sequential access stuff. If the node we're removing is the current
     * node in the list, reset the current node to the previous one. If the
     * previous one was non-existent (prev == NULL), we set the
     * end to be Unknown, since it is.
     */
    if (list->isOpen && list->curr == node) {
	list->curr = list->prev;
	if (list->curr == NULL) {
	    list->lastAccess = Unknown;
	}
    }

    /*
     * note that the datum is unmolested. The caller must free it as
     * necessary and as expected.
     */
    if (node->useCount == 0) {
	free(node);
    } else {
	node->deleted = TRUE;
    }
}

/* Replace the datum in the given node with the new datum. */
void
Lst_ReplaceS(LstNode node, void *datum)
{
    node->datum = datum;
}


/*
 * Node-specific functions
 */

/* Return the first node from the given list, or NULL if the list is empty or
 * invalid. */
LstNode
Lst_First(Lst list)
{
    if (!LstValid(list) || LstIsEmpty(list)) {
	return NULL;
    } else {
	return list->first;
    }
}

/* Return the last node from the given list, or NULL if the list is empty or
 * invalid. */
LstNode
Lst_Last(Lst list)
{
    if (!LstValid(list) || LstIsEmpty(list)) {
	return NULL;
    } else {
	return list->last;
    }
}

/* Return the successor to the given node on its list, or NULL. */
LstNode
Lst_Succ(LstNode node)
{
    if (node == NULL) {
	return NULL;
    } else {
	return node->next;
    }
}

/* Return the predecessor to the given node on its list, or NULL. */
LstNode
Lst_Prev(LstNode node)
{
    if (node == NULL) {
	return NULL;
    } else {
	return node->prev;
    }
}

/* Return the datum stored in the given node, or NULL if the node is invalid. */
void *
Lst_Datum(LstNode node)
{
    if (node != NULL) {
	return node->datum;
    } else {
	return NULL;
    }
}


/*
 * Functions for entire lists
 */

/* Return TRUE if the given list is empty or invalid. */
Boolean
Lst_IsEmpty(Lst list)
{
    return !LstValid(list) || LstIsEmpty(list);
}

/* Return the first node from the given list for which the given comparison
 * function returns 0, or NULL if none of the nodes matches. */
LstNode
Lst_Find(Lst list, const void *cmpData, int (*cmp)(const void *, const void *))
{
    return Lst_FindFrom(list, Lst_First(list), cmpData, cmp);
}

/* Return the first node from the given list, starting at the given node, for
 * which the given comparison function returns 0, or NULL if none of the nodes
 * matches. */
LstNode
Lst_FindFrom(Lst list, LstNode node, const void *cmpData,
	     int (*cmp)(const void *, const void *))
{
    LstNode tln;

    if (!LstValid(list) || LstIsEmpty(list) || !LstNodeValid(node)) {
	return NULL;
    }

    tln = node;

    do {
	if ((*cmp)(tln->datum, cmpData) == 0)
	    return tln;
	tln = tln->next;
    } while (tln != node && tln != NULL);

    return NULL;
}

/* Return the first node that contains the given datum, or NULL. */
LstNode
Lst_Member(Lst list, void *datum)
{
    LstNode node;

    if (list == NULL) {
	return NULL;
    }
    node = list->first;
    if (node == NULL) {
	return NULL;
    }

    do {
	if (node->datum == datum) {
	    return node;
	}
	node = node->next;
    } while (node != NULL && node != list->first);

    return NULL;
}

/* Apply the given function to each element of the given list. The function
 * should return 0 if traversal should continue and non-zero if it should
 * abort. */
int
Lst_ForEach(Lst list, int (*proc)(void *, void *), void *procData)
{
    return Lst_ForEachFrom(list, Lst_First(list), proc, procData);
}

/* Apply the given function to each element of the given list, starting from
 * the given node. The function should return 0 if traversal should continue,
 * and non-zero if it should abort. */
int
Lst_ForEachFrom(Lst list, LstNode node,
		int (*proc)(void *, void *), void *procData)
{
    LstNode tln = node;
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

	tln->useCount++;
	result = (*proc)(tln->datum, procData);
	tln->useCount--;

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
 *	list1		The list to which list2 is to be appended
 *	list2		The list to append to list1
 *	flags		LST_CONCNEW if the list nodes should be duplicated
 *			LST_CONCLINK if the list nodes should just be relinked
 */
ReturnStatus
Lst_Concat(Lst list1, Lst list2, int flags)
{
    LstNode node;	/* original node */
    LstNode newNode;
    LstNode last;	/* the last element in the list.
			 * Keeps bookkeeping until the end */

    if (!LstValid(list1) || !LstValid(list2)) {
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
	free(list2);
    } else if (list2->first != NULL) {
	/*
	 * We set the 'next' of the last element of list 2 to be nil to make
	 * the loop less difficult. The loop simply goes through the entire
	 * second list creating new LstNodes and filling in the 'next', and
	 * 'prev' to fit into list1 and its datum field from the
	 * datum field of the corresponding element in list2. The 'last' node
	 * follows the last of the new nodes along until the entire list2 has
	 * been appended. Only then does the bookkeeping catch up with the
	 * changes. During the first iteration of the loop, if 'last' is nil,
	 * the first list must have been empty so the newly-created node is
	 * made the first node of the list.
	 */
	list2->last->next = NULL;
	for (last = list1->last, node = list2->first;
	     node != NULL;
	     node = node->next)
	{
	    newNode = LstNodeNew(node->datum);
	    if (last != NULL) {
		last->next = newNode;
	    } else {
		list1->first = newNode;
	    }
	    newNode->prev = last;
	    last = newNode;
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
Lst_Open(Lst list)
{
    if (!LstValid(list)) {
	return FAILURE;
    }
    Lst_OpenS(list);
    return SUCCESS;
}

/* Open a list for sequential access. A list can still be searched, etc.,
 * without confusing these functions. */
void
Lst_OpenS(Lst list)
{
    assert(LstValid(list));
#if 0
    /* XXX: This assertion fails for NetBSD's "build.sh -j1 tools", somewhere
     * between "dependall ===> compat" and "dependall ===> binstall".
     * Building without the "-j1" succeeds though. */
    if (list->isOpen)
	Parse_Error(PARSE_WARNING, "Internal inconsistency: list opened twice");
    assert(!list->isOpen);
#endif

    list->isOpen = TRUE;
    list->lastAccess = LstIsEmpty(list) ? Head : Unknown;
    list->curr = NULL;
}

/* Return the next node for the given list, or NULL if the end has been
 * reached. */
LstNode
Lst_NextS(Lst list)
{
    LstNode node;

    assert(LstValid(list));
    assert(list->isOpen);

    list->prev = list->curr;

    if (list->curr == NULL) {
	if (list->lastAccess == Unknown) {
	    /*
	     * If we're just starting out, lastAccess will be Unknown.
	     * Then we want to start this thing off in the right
	     * direction -- at the start with lastAccess being Middle.
	     */
	    list->curr = node = list->first;
	    list->lastAccess = Middle;
	} else {
	    node = NULL;
	    list->lastAccess = Tail;
	}
    } else {
	node = list->curr->next;
	list->curr = node;

	if (node == list->first || node == NULL) {
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

    return node;
}

/* Close a list which was opened for sequential access. */
void
Lst_CloseS(Lst list)
{
    assert(LstValid(list));
    assert(list->isOpen);

    list->isOpen = FALSE;
    list->lastAccess = Unknown;
}


/*
 * for using the list as a queue
 */

/* Add the datum to the tail of the given list. */
ReturnStatus
Lst_EnQueue(Lst list, void *datum)
{
    if (!LstValid(list)) {
	return FAILURE;
    }

    return Lst_InsertAfter(list, Lst_Last(list), datum);
}

/* Remove and return the datum at the head of the given list, or NULL if the
 * list is empty. */
void *
Lst_DeQueue(Lst list)
{
    LstNode head;
    void *datum;

    head = Lst_First(list);
    if (head == NULL) {
	return NULL;
    }

    datum = head->datum;
    Lst_RemoveS(list, head);
    return datum;
}
