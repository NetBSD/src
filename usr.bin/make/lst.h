/*	$NetBSD: lst.h,v 1.63 2020/09/24 06:45:59 rillig Exp $	*/

/*
 * Copyright (c) 1988, 1989, 1990 The Regents of the University of California.
 * All rights reserved.
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
 *
 *	from: @(#)lst.h	8.1 (Berkeley) 6/6/93
 */

/*
 * Copyright (c) 1988, 1989 by Adam de Boor
 * Copyright (c) 1989 by Berkeley Softworks
 * All rights reserved.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *
 *	from: @(#)lst.h	8.1 (Berkeley) 6/6/93
 */

/* Doubly-linked lists of arbitrary pointers. */

#ifndef MAKE_LST_H
#define MAKE_LST_H

#include <sys/param.h>
#include <stdlib.h>

/* A doubly-linked list of pointers. */
typedef	struct List	List;
/* A single node in the doubly-linked list. */
typedef	struct ListNode	ListNode;

/* Copy a node, usually by allocating a copy of the given object.
 * For reference-counted objects, the original object may need to be
 * modified, therefore the parameter is not const. */
typedef void *LstCopyProc(void *);
/* Free the datum of a node, called before freeing the node itself. */
typedef void LstFreeProc(void *);
/* Return TRUE if the datum matches the args, for Lst_Find. */
typedef Boolean LstFindProc(const void *datum, const void *args);
/* An action for Lst_ForEach. */
typedef int LstActionProc(void *datum, void *args);

/* Create or destroy a list */

/* Create a new list. */
List *Lst_Init(void);
/* Duplicate an existing list. */
List *Lst_Copy(List *, LstCopyProc);
/* Free the list, leaving the node data unmodified. */
void Lst_Free(List *);
/* Free the list, freeing the node data using the given function. */
void Lst_Destroy(List *, LstFreeProc);

/* Get information about a list */

Boolean Lst_IsEmpty(List *);
/* Return the first node of the list, or NULL. */
ListNode *Lst_First(List *);
/* Return the last node of the list, or NULL. */
ListNode *Lst_Last(List *);
/* Find the first node for which the function returns TRUE, or NULL. */
ListNode *Lst_Find(List *, LstFindProc, const void *);
/* Find the first node for which the function returns TRUE, or NULL.
 * The search starts at the given node, towards the end of the list. */
ListNode *Lst_FindFrom(List *, ListNode *, LstFindProc, const void *);
/* Find the first node that contains the given datum, or NULL. */
ListNode *Lst_FindDatum(List *, const void *);

/* Modify a list */

/* Insert a datum before the given node. */
void Lst_InsertBefore(List *, ListNode *, void *);
/* Place a datum at the front of the list. */
void Lst_Prepend(List *, void *);
/* Place a datum at the end of the list. */
void Lst_Append(List *, void *);
/* Remove the node from the list. */
void Lst_Remove(List *, ListNode *);
void Lst_PrependAll(List *, List *);
void Lst_AppendAll(List *, List *);
void Lst_MoveAll(List *, List *);

/* Node-specific functions */

/* Return the successor of the node, or NULL. */
ListNode *LstNode_Next(ListNode *);
/* Return the predecessor of the node, or NULL. */
ListNode *LstNode_Prev(ListNode *);
/* Return the datum of the node. Usually not NULL. */
void *LstNode_Datum(ListNode *);
/* Replace the value of the node. */
void LstNode_Set(ListNode *, void *);
/* Set the value of the node to NULL. Having NULL in a list is unusual. */
void LstNode_SetNull(ListNode *);

/* Iterating over a list, using a callback function */

/* Apply a function to each datum of the list, until the callback function
 * returns non-zero. */
int Lst_ForEach(List *, LstActionProc, void *);

/* Iterating over a list while keeping track of the current node and possible
 * concurrent modifications */

/* Start iterating the list. */
void Lst_Open(List *);
/* Return the next node, or NULL. */
ListNode *Lst_Next(List *);
/* Finish iterating the list. */
void Lst_Close(List *);

/* Using the list as a queue */

/* Add a datum at the tail of the queue. */
void Lst_Enqueue(List *, void *);
/* Remove the head node of the queue and return its datum. */
void *Lst_Dequeue(List *);

/* A stack is a very simple collection of items that only allows access to the
 * top-most item. */
typedef struct {
    void **items;
    size_t len;
    size_t cap;
} Stack;

void Stack_Init(Stack *);
Boolean Stack_IsEmpty(Stack *);
void Stack_Push(Stack *, void *);
void *Stack_Pop(Stack *);
void Stack_Done(Stack *);

#endif /* MAKE_LST_H */
