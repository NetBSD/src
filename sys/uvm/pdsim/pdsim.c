/*	$NetBSD: pdsim.c,v 1.1 2006/09/30 08:47:39 yamt Exp $	*/

/*-
 * Copyright (c)2006 YAMAMOTO Takashi,
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

#include "pdsim.h"

#define	SHOWFAULT
#if defined(SHOWQLEN) || defined(SHOWIRR)
#undef SHOWFAULT
#endif

#undef	READAHEAD

struct vm_page *pages;

struct uvmexp uvmexp;

int npagein;
int nfault;
int raio;
int rahit;

int lastacc[MAXID];
int irr[MAXID];
int ts;

struct {
	int fault;
	int hit;
} stats[MAXID];

TAILQ_HEAD(, vm_page) freeq;

struct vm_page *
pdsim_pagealloc(struct uvm_object *obj, int idx)
{
	struct vm_page *pg;

	pg = TAILQ_FIRST(&freeq);
	if (pg == NULL) {
		return NULL;
	}
	TAILQ_REMOVE(&freeq, pg, pageq);
	pg->offset = idx << PAGE_SHIFT;
	pg->uanon = NULL;
	pg->uobject = obj;
	pg->pqflags = 0;
	obj->pages[idx] = pg;
	uvmexp.free--;
	uvmexp.filepages++;

	return pg;
}

void
pdsim_pagefree(struct vm_page *pg)
{
	struct uvm_object *obj;

	KASSERT(pg != NULL);

#if defined(SHOWFREE)
	if (pg->offset != -1) {
		int idx = pg->offset >> PAGE_SHIFT;
		printf("%d %d	# FREE IRR\n", idx, irr[idx]);
	}
#endif /* defined(SHOWFREE) */

	uvmpdpol_pagedequeue(pg);

	KASSERT(pg->uanon == NULL);
	obj = pg->uobject;
	if (obj != NULL) {
		int idx;

		idx = pg->offset >> PAGE_SHIFT;
		KASSERT(obj->pages[idx] == pg);
		obj->pages[idx] = NULL;
		uvmexp.filepages--;
	}
	TAILQ_INSERT_HEAD(&freeq, pg, pageq);
	uvmexp.free++;
}

static struct vm_page *
pdsim_pagelookup(struct uvm_object *obj, int index)
{
	struct vm_page *pg;

	pg = obj->pages[index];

	return pg;
}

static void
pdsim_pagemarkreferenced(struct vm_page *pg)
{

	pg->_mdflags |= MDPG_REFERENCED;
}

boolean_t
pmap_is_referenced(struct vm_page *pg)
{

	return pg->_mdflags & MDPG_REFERENCED;
}

boolean_t
pmap_clear_reference(struct vm_page *pg)
{
	boolean_t referenced = pmap_is_referenced(pg);

	pg->_mdflags &= ~MDPG_REFERENCED;

	return referenced;
}

static void
pdsim_init(int n)
{
	struct vm_page *pg;
	int i;

	uvmpdpol_init();
	uvmexp.npages = n;
	uvmpdpol_reinit();

	TAILQ_INIT(&freeq);
	pages = calloc(n, sizeof(*pg));
	for (i = 0; i < n; i++) {
		pg = &pages[i];
		pg->offset = -1;
		pdsim_pagefree(pg);
	}
}

static void
pdsim_reclaimone(void)
{
	struct vm_page *pg;

	uvmexp.freetarg = 1;
	while (uvmexp.free < uvmexp.freetarg) {
		uvmpdpol_tune();
		uvmpdpol_scaninit();
		pg = uvmpdpol_selectvictim();
		if (pg != NULL) {
			pdsim_pagefree(pg);
		}
		uvmpdpol_balancequeue(0);
	}
}

static void
fault(struct uvm_object *obj, int index)
{
	struct vm_page *pg;

	DPRINTF("fault: %d -> ", index);
	nfault++;
	ts++;
	if (lastacc[index]) {
		irr[index] = ts - lastacc[index];
	}
	lastacc[index] = ts;
	stats[index].fault++;
	pg = pdsim_pagelookup(obj, index);
	if (pg) {
		DPRINTF("cached\n");
		pdsim_pagemarkreferenced(pg);
		stats[index].hit++;
		if ((pg->_mdflags & MDPG_SPECULATIVE) != 0) {
			pg->_mdflags &= ~MDPG_SPECULATIVE;
			rahit++;
		}
		return;
	}
	DPRINTF("miss\n");
retry:
	pg = pdsim_pagealloc(obj, index);
	if (pg == NULL) {
		pdsim_reclaimone();
		goto retry;
	}
	npagein++;
#if defined(SHOWFAULT)
	printf("%d	# FLT\n", index);
#endif
	pdsim_pagemarkreferenced(pg);
	uvmpdpol_pageactivate(pg);
	uvmpdpol_pageactivate(pg);
	dump("fault");
#if defined(READAHEAD)
	pg = pdsim_pagelookup(obj, index + 1);
	if (pg == NULL) {
ra_retry:
		pg = pdsim_pagealloc(obj, index + 1);
		if (pg == NULL) {
			pdsim_reclaimone();
			goto ra_retry;
		}
		raio++;
		pg->_mdflags |= MDPG_SPECULATIVE;
#if defined(SHOWFAULT)
		printf("%d	# READ-AHEAD\n", index + 1);
#endif
	}
	uvmpdpol_pageenqueue(pg);
	dump("read-ahead");
#endif /* defined(READAHEAD) */
}

struct uvm_object obj;

static void
test(void)
{
	memset(&obj, 0, sizeof(obj));
	char *ln;

	for (;; free(ln)) {
		int i;
		int ch;

		ln = fparseln(stdin, NULL, NULL, NULL, 0); 
		if (ln == NULL) {
			break;
		}
		ch = *ln;
		if (ch == '\0') {
			break;
		}
		if (ch == 'd') {
			dump("test");
			continue;
		}
		i = atoi(ln);
		fault(&obj, i);
#if defined(SHOWQLEN)
		showqlen();
#endif
	}
}

static void
dumpstats(void)
{
	int i;
	for (i = 0; i < MAXID; i++) {
		if (stats[i].fault == 0) {
			continue;
		}
		DPRINTF("[%d] %d/%d %d\n", i,
		    stats[i].hit, stats[i].fault, irr[i]);
	}
}

int
main(int argc, char *argv[])
{

	setvbuf(stderr, NULL, _IOFBF, 0); /* XXX */

	pdsim_init(atoi(argv[1]));
	test();
	DPRINTF("io %d (%d + ra %d) / flt %d\n",
	    npagein + raio, npagein, raio, nfault);
	DPRINTF("rahit / raio= %d / %d\n", rahit, raio);
#if defined(DEBUG)
	dumpstats();
#endif
	exit(0);
}
