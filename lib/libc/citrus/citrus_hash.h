/*	$NetBSD: citrus_hash.h,v 1.1 2003/06/25 09:51:33 tshiozak Exp $	*/

/*-
 * Copyright (c)2003 Citrus Project,
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _CITRUS_HASH_H_
#define _CITRUS_HASH_H_

#define _CITRUS_HASH_ENTRY(type) 				\
struct {							\
	struct type *che_next, **che_pprev;			\
}
#define _CITRUS_HASH_HEAD(headname, type, hashsize)		\
struct headname {						\
	struct type *chh_table[hashsize];			\
}
#define _CITRUS_HASH_INIT(head, hashsize)			\
do {								\
	int _ch_loop;						\
	for (_ch_loop=0; _ch_loop<hashsize; _ch_loop++)		\
		(head)->chh_table[_ch_loop] = NULL;		\
} while (/*CONSTCOND*/0)
#define _CITRUS_HASH_REMOVE(elm, field)				\
do {								\
	if ((elm)->field.che_next)				\
		(elm)->field.che_next->field.che_pprev =	\
			(elm)->field.che_pprev;			\
	*(elm)->field.che_pprev = (elm)->field.che_next;	\
} while (/*CONSTCOND*/0)
#define _CITRUS_HASH_INSERT(head, elm, field, hashval)		\
do {								\
	(elm)->field.che_pprev = &(head)->chh_table[hashval];	\
	(elm)->field.che_next  =  (head)->chh_table[hashval];	\
	(head)->chh_table[hashval] = (elm);			\
} while (/*CONSTCOND*/0)
#define _CITRUS_HASH_SEARCH(head, elm, field, matchfunc, key, hashval)	\
do {									\
	for ((elm)=(head)->chh_table[hashval];				\
	     (elm);							\
	     (elm)=(elm)->field.che_next)				\
		if (matchfunc((elm), key)==0)				\
			break;						\
} while (/*CONSTCOND*/0)

__BEGIN_DECLS
int	_citrus_string_hash_func(const char *, int);
__END_DECLS

#endif
