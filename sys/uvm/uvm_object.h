/*	$NetBSD: uvm_object.h,v 1.31.2.1 2011/11/02 21:54:01 yamt Exp $	*/

/*
 * Copyright (c) 1997 Charles D. Cranor and Washington University.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * from: Id: uvm_object.h,v 1.1.2.2 1998/01/04 22:44:51 chuck Exp
 */

#ifndef _UVM_UVM_OBJECT_H_
#define _UVM_UVM_OBJECT_H_

/*
 * uvm_object.h
 */

#include <sys/queue.h>
#include <sys/radixtree.h>
#include <uvm/uvm_pglist.h>

/*
 * uvm_object: all that is left of mach objects.
 */

struct uvm_object {
	kmutex_t *		vmobjlock;	/* lock on memq */
	const struct uvm_pagerops *pgops;	/* pager ops */
	struct pglist		memq;		/* pages in this object */
	int			uo_npages;	/* # of pages in memq */
	unsigned		uo_refs;	/* reference count */
	struct radix_tree	uo_pages;	/* tree of pages */
	LIST_HEAD(,ubc_map)	uo_ubc;		/* ubc mappings */
};

/*
 * uo_pages
 */

#define	UVM_PAGE_DIRTY_TAG	0	/* might be dirty (!PG_CLEAN) */

/*
 * UVM_OBJ_KERN is a 'special' uo_refs value which indicates that the
 * object is a kernel memory object rather than a normal one (kernel
 * memory objects don't have reference counts -- they never die).
 *
 * this value is used to detected kernel object mappings at uvm_unmap()
 * time.   normally when an object is unmapped its pages eventaully become
 * deactivated and then paged out and/or freed.    this is not useful
 * for kernel objects... when a kernel object is unmapped we always want
 * to free the resources associated with the mapping.   UVM_OBJ_KERN
 * allows us to decide which type of unmapping we want to do.
 */
#define UVM_OBJ_KERN		(-2)

#define	UVM_OBJ_IS_KERN_OBJECT(uobj)					\
	((uobj)->uo_refs == UVM_OBJ_KERN)

#ifdef _KERNEL

extern const struct uvm_pagerops uvm_vnodeops;
extern const struct uvm_pagerops uvm_deviceops;
extern const struct uvm_pagerops ubc_pager;
extern const struct uvm_pagerops aobj_pager;

#define	UVM_OBJ_IS_VNODE(uobj)						\
	((uobj)->pgops == &uvm_vnodeops)

#define	UVM_OBJ_IS_DEVICE(uobj)						\
	((uobj)->pgops == &uvm_deviceops)

#define	UVM_OBJ_IS_VTEXT(uobj)						\
	(UVM_OBJ_IS_VNODE(uobj) && uvn_text_p(uobj))

#define	UVM_OBJ_IS_CLEAN(uobj)						\
	(UVM_OBJ_IS_VNODE(uobj) && uvn_clean_p(uobj))

/*
 * UVM_OBJ_NEEDS_WRITEFAULT: true if the uobj needs to detect modification.
 * (ie. wants to avoid writable user mappings.)
 *
 * XXX bad name
 */

#define	UVM_OBJ_NEEDS_WRITEFAULT(uobj)					\
	(UVM_OBJ_IS_VNODE(uobj) && uvn_needs_writefault_p(uobj))

#define	UVM_OBJ_IS_AOBJ(uobj)						\
	((uobj)->pgops == &aobj_pager)

#endif /* _KERNEL */

#endif /* _UVM_UVM_OBJECT_H_ */
