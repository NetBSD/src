/*      $NetBSD: menutree.h,v 1.1 2001/01/05 01:28:37 garbled Exp $       */

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *  
 * Copyright (c) 2000 Tim Rightnour <garbled@netbsd.org>  
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _MENUTREE_H_
#define _MENUTREE_H_

CIRCLEQ_HEAD(cqMenu, menuentry);

typedef struct menuentry {
	struct cqMenu cqSubMenuHead;	/* CIRCLEQ_HEAD */
	CIRCLEQ_ENTRY(menuentry) cqMenuEntries;

	char	*path;
	char	*filename;
	char	*itemname;
	char	*quickname;
} MTREE_ENTRY;

extern struct cqMenu *cqMenuHeadp;

#define TREE_ISEMPTY(cqm)	(CIRCLEQ_FIRST(cqm) == (void *)cqm)

void tree_init __P((void));
void tree_appenditem __P((struct cqMenu *, char *, char *, char *, char *));
int tree_entries __P((struct cqMenu *));
MTREE_ENTRY *tree_getentry __P((struct cqMenu *, int));
MTREE_ENTRY *tree_gettreebyname __P((struct cqMenu *, char *));
void tree_printtree __P((struct cqMenu *));

#endif	/* _MENUTREE_H_ */
