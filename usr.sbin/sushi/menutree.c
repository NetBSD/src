/*      $NetBSD: menutree.c,v 1.1 2001/01/05 01:28:37 garbled Exp $       */

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Copyright (c) 2000 Dante Profeta <dante@netbsd.org>
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

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sushi.h"
#include "menutree.h"

struct cqMenu cqMenuHead, *cqMenuHeadp;

void
tree_init(void)
{
	CIRCLEQ_INIT(&cqMenuHead);
	cqMenuHeadp = &cqMenuHead;
}

void
tree_appenditem(cqm, filename, itemname, quickname, path)
	struct cqMenu *cqm;
	char *filename;
	char *itemname;
	char *quickname;
	char *path;
{
	MTREE_ENTRY *mte;

	if ((mte = malloc(sizeof(MTREE_ENTRY))) == NULL ||
		((mte->filename = strdup(filename)) == NULL) ||
		((mte->quickname = strdup(quickname)) == NULL) ||
		((mte->path = strdup(path)) == NULL) ||
		((mte->itemname = strdup(itemname)) == NULL))
			bailout("malloc: %s", strerror(errno));

	CIRCLEQ_INIT(&mte->cqSubMenuHead);
	CIRCLEQ_INSERT_TAIL(cqm, mte, cqMenuEntries);
}

int
tree_entries(cqm)
	struct cqMenu *cqm;
{
	MTREE_ENTRY *mte;
	int entries = 0;

	for (mte = CIRCLEQ_FIRST(cqm); mte != (void *)cqm;
	     mte = CIRCLEQ_NEXT(mte, cqMenuEntries))
		++entries;

	return (entries);
}

MTREE_ENTRY *
tree_getentry(cqm, entry)
	struct cqMenu *cqm;
	int entry;
{
	MTREE_ENTRY *mte;
	int entries = 0;

	for (mte = CIRCLEQ_FIRST(cqm); (mte != (void *)cqm);
	     mte = CIRCLEQ_NEXT(mte, cqMenuEntries)) {
		if(entry == entries)
			return(mte);
		++entries;
	}

	return(NULL);
}

#ifdef DEBUG
void
tree_printtree(cqm)
	struct cqMenu *cqm;
{
	MTREE_ENTRY *mtp;

	for (mtp = CIRCLEQ_FIRST(cqm); mtp != (void *)cqm;
	     mtp = CIRCLEQ_NEXT(mtp, cqMenuEntries)) {
		printf("%s/%s\t%s\t- %s\n", mtp->path, mtp->filename,
		    mtp->quickname, mtp->itemname);
		tree_printtree(&mtp->cqSubMenuHead);
	}
}
#endif

MTREE_ENTRY *
tree_gettreebyname(struct cqMenu *cqm, char *quickname)
{
	MTREE_ENTRY *foundmtp = NULL;
	MTREE_ENTRY *mtp;

	for (mtp = CIRCLEQ_FIRST((struct cqMenu *)cqm); mtp != (void *)cqm;
	    mtp = CIRCLEQ_NEXT(mtp, cqMenuEntries)) {
		if (strcmp(mtp->quickname, quickname) == 0)
			return(mtp);
		if (!foundmtp)
			foundmtp = tree_gettreebyname(&mtp->cqSubMenuHead,
			    quickname);
	}

	return(foundmtp);
}
