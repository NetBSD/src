/*	$NetBSD: pmap.h,v 1.5 2004/04/23 02:58:29 simonb Exp $ */

/*
 * Copyright (c) 2002, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Brown.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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

/*
 * only pull in the headers once.  if we attempt to pull them in
 * again, we get namespace collisions on some structures, and we don't
 * want to redefine some of them since it will affect the layout of
 * the struct kbit.
 */
#ifndef PMAP_HEADERS

#include <err.h>
#include <kvm.h>
#include <stddef.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/uio.h>
#include <sys/namei.h>
#include <sys/sysctl.h>

#include <uvm/uvm.h>
#include <uvm/uvm_device.h>

#include <ufs/ufs/inode.h>
#undef doff_t
#undef IN_ACCESS
#include <isofs/cd9660/iso.h>
#include <isofs/cd9660/cd9660_node.h>

#define PMAP_HEADERS
#endif /* PMAP_HEADERS */

#ifndef CONCAT
#define	CONCAT(x,y)     __CONCAT(x,y)
#endif
#ifndef PMAPFUNC
#define PMAPFUNC(x,y)	CONCAT(x,CONCAT(_,y))
#endif

/*
 * if the definition of the kbit structure (and the accessor macros)
 * is already established, don't redefine it.
 */
#ifndef struct_kbit

/*
 * stolen (and munged) from #include <uvm/uvm_object.h>
 */
#define UVM_OBJ_IS_VNODE(uobj)    ((uobj)->pgops == uvm_vnodeops)
#define UVM_OBJ_IS_AOBJ(uobj)     ((uobj)->pgops == aobj_pager)
#define UVM_OBJ_IS_DEVICE(uobj)   ((uobj)->pgops == uvm_deviceops)
#define UVM_OBJ_IS_UBCPAGER(uobj) ((uobj)->pgops == ubc_pager)

/*
 * stolen from #include <uvm/uvm_amap.h>
 */
#define PPREF_NONE ((int *) -1)

/* the size of the object in the kernel */
#define S(x)	((x)->k_size)
/* the address of the object in kernel, two forms */
#define A(x)	((x)->k_addr.k_addr_ul)
#define P(x)	((x)->k_addr.k_addr_p)
/* the data from the kernel */
#define D(x,d)	(&((x)->k_data.d))

/* suck the data from the kernel */
#define _KDEREFOK(kd, addr, dst, sz) \
	(kvm_read((kd), (addr), (dst), (sz)) == (sz))
#define _KDEREF(kd, addr, dst, sz) do { \
	if (!_KDEREFOK((kd), (addr), (dst), (sz))) \
		errx(1, "trying to read %lu bytes from %lx: %s", \
		    (unsigned long)(sz), (addr), kvm_geterr(kd)); \
} while (0/*CONSTCOND*/)

/* suck the data using the structure */
#define KDEREFOK(kd, item) _KDEREFOK((kd), A(item), D(item, data), S(item))
#define KDEREF(kd, item) _KDEREF((kd), A(item), D(item, data), S(item))

struct kbit {
	/*
	 * size of data chunk
	 */
	size_t k_size;

	/*
	 * something for printf() and something for kvm_read()
	 */
	union {
		void *k_addr_p;
		u_long k_addr_ul;
	} k_addr;

	/*
	 * where we actually put the "stuff"
	 */
	union {
		char data[1];
		struct vmspace vmspace;
		struct vm_map vm_map;
		struct vm_map_entry vm_map_entry;
		struct vnode vnode;
		struct uvm_object uvm_object;
		struct mount mount;
		struct namecache namecache;
		struct inode inode;
		struct iso_node iso_node;
		struct uvm_device uvm_device;
		struct vm_amap amap;
		struct vm_anon anon;
	} k_data;
};
#define struct_kbit
#endif /* struct_kbit */

/*
 * VERSION, if not defined, defaults to "regular" or "lockdebug" (if
 * LOCKDEBUG is defined)
 */
#ifndef VERSION
#ifndef LOCKDEBUG
#define VERSION regular
#else /* LOCKDEBUG */
#define VERSION lockdebug
#endif /* LOCKDEBUG */
#define undef_VERSION
#endif /* VERSION */

/*
 * this is the *actual* module interface
 */

void PMAPFUNC(process_map,VERSION)(kvm_t *, struct kinfo_proc2 *,
				   struct kbit *, const char *);
void PMAPFUNC(dump_vm_map,VERSION)(kvm_t *, struct kinfo_proc2 *,
				   struct kbit *, struct kbit *, const char *);
size_t PMAPFUNC(dump_vm_map_entry,VERSION)(kvm_t *, struct kinfo_proc2 *,
					   struct kbit *, struct kbit *, int);
void PMAPFUNC(dump_amap,VERSION)(kvm_t *, struct kbit *);

/*
 * if we defined VERSION here, we should undef it now that we're done.
 */
#ifdef undef_VERSION
#undef VERSION
#undef undef_VERSION
#endif /* undef_VERSION */
