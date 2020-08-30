/*	$NetBSD: lst.h,v 1.59 2020/08/30 11:15:05 rillig Exp $	*/

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

/*-
 * lst.h --
 *	Header for using the list library
 */
#ifndef MAKE_LST_H
#define MAKE_LST_H

#include	<sys/param.h>
#include	<stdlib.h>

/*
 * basic typedef. This is what the Lst_ functions handle
 */

typedef	struct List	*Lst;
typedef	struct ListNode	*LstNode;

typedef void *LstCopyProc(void *);
typedef void LstFreeProc(void *);
typedef Boolean LstFindProc(const void *, const void *);
typedef int LstActionProc(void *, void *);

/*
 * Creation/destruction functions
 */
/* Create a new list */
Lst		Lst_Init(void);
/* Duplicate an existing list */
Lst		Lst_Copy(Lst, LstCopyProc);
/* Destroy an old one */
void		Lst_Free(Lst);
void		Lst_Destroy(Lst, LstFreeProc);
/* True if list is empty */
Boolean		Lst_IsEmpty(Lst);

/*
 * Functions to modify a list
 */
/* Insert an element before another */
void		Lst_InsertBefore(Lst, LstNode, void *);
/* Place an element at the front of a lst. */
void		Lst_Prepend(Lst, void *);
/* Place an element at the end of a lst. */
void		Lst_Append(Lst, void *);
/* Remove an element */
void		Lst_Remove(Lst, LstNode);
/* Replace a node with a new value */
void		LstNode_Set(LstNode, void *);
void		LstNode_SetNull(LstNode);

void		Lst_PrependAll(Lst, Lst);
void		Lst_AppendAll(Lst, Lst);
void		Lst_MoveAll(Lst, Lst);

/*
 * Node-specific functions
 */
/* Return first element in list */
LstNode		Lst_First(Lst);
/* Return last element in list */
LstNode		Lst_Last(Lst);
/* Return successor to given element */
LstNode		LstNode_Next(LstNode);
/* Return predecessor to given element */
LstNode		LstNode_Prev(LstNode);
/* Get datum from LstNode */
void		*LstNode_Datum(LstNode);

/*
 * Functions for entire lists
 */
/* Find an element in a list */
LstNode		Lst_Find(Lst, LstFindProc, const void *);
/* Find an element starting from somewhere */
LstNode		Lst_FindFrom(Lst, LstNode, LstFindProc, const void *);
/* Return the first node that contains the given datum, or NULL. */
LstNode		Lst_FindDatum(Lst, const void *);
/* Apply a function to all elements of a lst */
int		Lst_ForEach(Lst, LstActionProc, void *);
/* Apply a function to all elements of a lst starting from a certain point. */
int		Lst_ForEachFrom(Lst, LstNode, LstActionProc, void *);
/*
 * these functions are for dealing with a list as a table, of sorts.
 * An idea of the "current element" is kept and used by all the functions
 * between Lst_Open() and Lst_Close().
 */
/* Open the list */
void		Lst_Open(Lst);
/* Next element please, or NULL */
LstNode		Lst_Next(Lst);
/* Finish table access */
void		Lst_Close(Lst);

/*
 * for using the list as a queue
 */
/* Place an element at tail of queue */
void		Lst_Enqueue(Lst, void *);
/* Remove an element from head of queue */
void		*Lst_Dequeue(Lst);

#endif /* MAKE_LST_H */
