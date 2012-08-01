/*	$NetBSD: uvm_page_array.h,v 1.1.2.6 2012/08/01 22:34:14 yamt Exp $	*/

/*-
 * Copyright (c)2011 YAMAMOTO Takashi,
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

#if !defined(_UVM_UVM_PAGE_ARRAY_H_)
#define _UVM_UVM_PAGE_ARRAY_H_

/*
 * uvm_page_array: an array of pages.
 *
 * these structure and functions simply manipulate struct vm_page pointers.
 * it's caller's responsibity to acquire and keep the object lock so that
 * the result is valid.
 *
 * typical usage:
 *
 *	struct uvm_page_array a;
 *
 *	uvm_page_array_init(&a);
 *	while ((pg = uvm_page_array_fill_and_peek(&a, uobj, off, ....))
 *	    != NULL) {
 *		off = pg->offset + PAGE_SIZE;
 *		do_something(pg);
 *		uvm_page_array_advance(&a);
 *	}
 *	uvm_page_array_fini(&a);
 */

struct vm_page;

struct uvm_page_array {
	struct vm_page *ar_pages[16];	/* XXX tune */
	unsigned int ar_npages;		/* valid elements in ar_pages */
	unsigned int ar_idx;		/* index in ar_pages */
};

void uvm_page_array_init(struct uvm_page_array *);
void uvm_page_array_fini(struct uvm_page_array *);
void uvm_page_array_clear(struct uvm_page_array *);
struct vm_page *uvm_page_array_peek(struct uvm_page_array *);
void uvm_page_array_advance(struct uvm_page_array *);
int uvm_page_array_fill(struct uvm_page_array *, struct uvm_object *,
    voff_t, unsigned int, unsigned int);
struct vm_page *uvm_page_array_fill_and_peek(struct uvm_page_array *,
    struct uvm_object *, voff_t, unsigned int, unsigned int);

/*
 * flags for uvm_page_array_fill and uvm_page_array_fill_and_peek
 */
#define	UVM_PAGE_ARRAY_FILL_DIRTY	1	/* dirty pages */
#define	UVM_PAGE_ARRAY_FILL_WRITEBACK	2	/* dirty or written-back */
#define	UVM_PAGE_ARRAY_FILL_DENSE	4	/* stop on a hole */
#define	UVM_PAGE_ARRAY_FILL_BACKWARD	8	/* descend order */

#endif /* defined(_UVM_UVM_ARRAY_H_) */
