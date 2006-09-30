/*	$NetBSD: pdsim.h,v 1.1 2006/09/30 08:47:39 yamt Exp $	*/

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

#include <sys/null.h>
#include <sys/hash.h>
#include <sys/queue.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <inttypes.h>

#define	PAGE_SHIFT	12
#define	PAGE_SIZE	(1 << PAGE_SHIFT)
#define	PAGE_MASK	(PAGE_SIZE - 1)

#define	PDPOL_EVCNT_DEFINE(name)	int pdpol_evcnt_##name = 0;
#define	PDPOL_EVCNT_INCR(ev)		pdpol_evcnt_##ev++

#define	TRUE	1
#define	FALSE	0
typedef int boolean_t;
typedef off_t voff_t;

struct vm_page;
struct vm_anon;

struct uvm_object;
struct vm_anon {
	struct vm_page *an_page; /* XXX */
};

struct vm_page {
	TAILQ_ENTRY(vm_page) pageq;
	struct uvm_object *uobject;
	off_t offset;
	int pqflags;
#define PQ_SWAPBACKED	(PQ_ANON|PQ_AOBJ)
#define PQ_FREE		0x0001
#define PQ_ANON		0x0002
#define PQ_AOBJ		0x0004
#define PQ_PRIVATE1	0x0100
#define PQ_PRIVATE2	0x0200
#define PQ_PRIVATE3	0x0400
#define PQ_PRIVATE4	0x0800
#define PQ_PRIVATE5	0x1000
#define PQ_PRIVATE6	0x2000
#define PQ_PRIVATE7	0x4000
#define PQ_PRIVATE8	0x8000

	int _mdflags;
#define	MDPG_REFERENCED		1
#define	MDPG_SPECULATIVE	2

	/* dummy members */
	struct vm_anon *uanon;
	int wire_count;
	int flags;
#define	PG_BUSY		1
};

TAILQ_HEAD(pglist, vm_page);

boolean_t pmap_clear_reference(struct vm_page *);
boolean_t pmap_is_referenced(struct vm_page *);

struct uvmexp {
	int npages;
	int pdscans;
	int freetarg;
	int free;

	/* XXX */
	int filepages;
	int execpages;
	int anonpages;
	int pddeact;
	int pdreact;
};
extern struct uvmexp uvmexp;

#define	UVM_LOCK_ASSERT_PAGEQ()	/* nothing */
#define	KASSERT(x)	assert(x)

static void panic(const char *, ...) __unused;
static void
panic(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");
	abort();
}

#define	MAXID	102400

struct uvm_object {
	struct vm_page *pages[MAXID];
};

struct uvm_pctparam {
	int pct_pct;
};
#define	UVM_PCTPARAM_APPLY(pct, x) \
	(((x) * (pct)->pct_pct) / 100)
#define	uvm_pctparam_init(pct, x, f)	(pct)->pct_pct = x

#define	UVM_OBJ_IS_VTEXT(o)	FALSE
#define	UVM_OBJ_IS_AOBJ(o)	FALSE
#define	UVM_OBJ_IS_VNODE(o)	TRUE

#define	uvm_swapisfull()	TRUE
#define	uvmpd_trydropswap(p)	(panic("dropswap"), 0)

#define	MAX(a,b)	(((a) > (b)) ? (a) : (b))
#define	MIN(a,b)	(((a) < (b)) ? (a) : (b))

#if ((__STDC_VERSION__ - 0) >= 199901L)
#define	WARN(...)	fprintf(stderr, __VA_ARGS__)
#else /* ((__STDC_VERSION__ - 0) >= 199901L) */
#define	WARN(a...)	fprintf(stderr, a)
#endif /* ((__STDC_VERSION__ - 0) >= 199901L) */

#if defined(DEBUG)
#if ((__STDC_VERSION__ - 0) >= 199901L)
#define	DPRINTF(...)	printf(__VA_ARGS__)
#else /* ((__STDC_VERSION__ - 0) >= 199901L) */
#define	DPRINTF(a...)	printf(a)	/* GCC */
#endif /* ((__STDC_VERSION__ - 0) >= 199901L) */
#define	dump(s)		pdsim_dump(s)
void dump(const char *);
#else
#if ((__STDC_VERSION__ - 0) >= 199901L)
#define	DPRINTF(...)	/* nothing */
#else /* ((__STDC_VERSION__ - 0) >= 199901L) */
#define	DPRINTF(a...)	/* nothing */	/* GCC */
#endif /* ((__STDC_VERSION__ - 0) >= 199901L) */
#define	dump(s)		/* nothing */
#endif

#include "uvm/uvm_pdpolicy.h"
