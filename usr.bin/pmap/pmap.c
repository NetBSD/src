/*	$NetBSD: pmap.c,v 1.37.4.2 2008/01/09 02:00:52 matt Exp $ */

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

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: pmap.c,v 1.37.4.2 2008/01/09 02:00:52 matt Exp $");
#endif

#include <string.h>

#include "pmap.h"
#include "main.h"

static void dump_vm_anon(kvm_t *, struct vm_anon **, int);
static char *findname(kvm_t *, struct kbit *, struct kbit *, struct kbit *,
	struct kbit *, struct kbit *);
static int search_cache(kvm_t *, struct kbit *, char **, char *, size_t);

/* when recursing, output is indented */
#define indent(n) ((n) * (recurse > 1 ? recurse - 1 : 0))
#define rwx (VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE)

int heapfound;

void
process_map(kvm_t *kd, struct kinfo_proc2 *proc,
			      struct kbit *vmspace, const char *thing)
{
	struct kbit kbit, *vm_map = &kbit;

	if (proc) {
		heapfound = 0;
		A(vmspace) = (u_long)proc->p_vmspace;
		S(vmspace) = sizeof(struct vmspace);
		thing = "proc->p_vmspace.vm_map";
	} else if (S(vmspace) == (size_t)-1) {
		heapfound = 0;
		/* A(vmspace) set by caller */
		S(vmspace) = sizeof(struct vmspace);
		/* object identified by caller */
	} else {
		heapfound = 1; /* but really, do kernels have a heap? */
		A(vmspace) = 0;
		S(vmspace) = 0;
		thing = "kernel_map";
	}

	S(vm_map) = sizeof(struct vm_map);

	if (S(vmspace) != 0) {
		KDEREF(kd, vmspace);
		A(vm_map) = A(vmspace) + offsetof(struct vmspace, vm_map);
		memcpy(D(vm_map, vm_map), &D(vmspace, vmspace)->vm_map,
		       S(vm_map));
	} else {
		memset(vmspace, 0, sizeof(*vmspace));
		A(vm_map) = kernel_map_addr;
		KDEREF(kd, vm_map);
	}

	dump_vm_map(kd, proc, vmspace, vm_map, thing);
}

void
dump_vm_map(kvm_t *kd, struct kinfo_proc2 *proc,
	struct kbit *vmspace, struct kbit *vm_map, const char *mname)
{
	struct kbit kbit[2], *header, *vm_map_entry;
	struct vm_map_entry *last, *next;
	size_t total;
	u_long addr, end;

	if (S(vm_map) == (size_t)-1) {
		heapfound = 1;
		S(vm_map) = sizeof(struct vm_map);
		KDEREF(kd, vm_map);
	}

	header = &kbit[0];
	vm_map_entry = &kbit[1];
	A(header) = 0;
	A(vm_map_entry) = 0;

	A(header) = A(vm_map) + offsetof(struct vm_map, header);
	S(header) = sizeof(struct vm_map_entry);
	memcpy(D(header, vm_map_entry), &D(vm_map, vm_map)->header, S(header));

	if (S(vmspace) != 0 && (debug & PRINT_VMSPACE)) {
		printf("proc->p_vmspace %p = {", P(vmspace));
		printf(" vm_refcnt = %d,", D(vmspace, vmspace)->vm_refcnt);
		printf(" vm_shm = %p,\n", D(vmspace, vmspace)->vm_shm);
		printf("    vm_rssize = %d,", D(vmspace, vmspace)->vm_rssize);
		printf(" vm_swrss = %d,", D(vmspace, vmspace)->vm_swrss);
		printf(" vm_tsize = %d,", D(vmspace, vmspace)->vm_tsize);
		printf(" vm_dsize = %d,\n", D(vmspace, vmspace)->vm_dsize);
		printf("    vm_ssize = %d,", D(vmspace, vmspace)->vm_ssize);
		printf(" vm_taddr = %p,", D(vmspace, vmspace)->vm_taddr);
		printf(" vm_daddr = %p,\n", D(vmspace, vmspace)->vm_daddr);
		printf("    vm_maxsaddr = %p,",
		       D(vmspace, vmspace)->vm_maxsaddr);
		printf(" vm_minsaddr = %p }\n",
		       D(vmspace, vmspace)->vm_minsaddr);
	}

	if (debug & PRINT_VM_MAP) {
		printf("%*s%s %p = {", indent(2), "", mname, P(vm_map));
		printf(" pmap = %p,\n", D(vm_map, vm_map)->pmap);
		printf("%*s    lock = <struct lock>,", indent(2), "");
		printf(" header = <struct vm_map_entry>,");
		printf(" nentries = %d,\n", D(vm_map, vm_map)->nentries);
		printf("%*s    size = %lx,", indent(2), "",
		       D(vm_map, vm_map)->size);
		printf(" ref_count = %d,", D(vm_map, vm_map)->ref_count);
		printf("%*s    hint = %p,", indent(2), "",
		       D(vm_map, vm_map)->hint);
		printf("%*s    first_free = %p,", indent(2), "",
		       D(vm_map, vm_map)->first_free);
		printf(" flags = %x <%s%s%s%s%s >,\n", D(vm_map, vm_map)->flags,
		       D(vm_map, vm_map)->flags & VM_MAP_PAGEABLE ? " PAGEABLE" : "",
		       D(vm_map, vm_map)->flags & VM_MAP_INTRSAFE ? " INTRSAFE" : "",
		       D(vm_map, vm_map)->flags & VM_MAP_WIREFUTURE ? " WIREFUTURE" : "",
#ifdef VM_MAP_DYING
		       D(vm_map, vm_map)->flags & VM_MAP_DYING ? " DYING" :
#endif
		       "",
#ifdef VM_MAP_TOPDOWN
		       D(vm_map, vm_map)->flags & VM_MAP_TOPDOWN ? " TOPDOWN" :
#endif
		       "");
		printf("%*s    timestamp = %u }\n", indent(2), "",
		     D(vm_map, vm_map)->timestamp);
	}
	if (print_ddb) {
		const char *name = mapname(P(vm_map));

		printf("%*s%s %p: [0x%lx->0x%lx]\n", indent(2), "",
		       recurse < 2 ? "MAP" : "SUBMAP", P(vm_map),
		       vm_map_min(D(vm_map, vm_map)),
		       vm_map_max(D(vm_map, vm_map)));
		printf("\t%*s#ent=%d, sz=%ld, ref=%d, version=%d, flags=0x%x\n",
		       indent(2), "", D(vm_map, vm_map)->nentries,
		       D(vm_map, vm_map)->size, D(vm_map, vm_map)->ref_count,
		       D(vm_map, vm_map)->timestamp, D(vm_map, vm_map)->flags);
		printf("\t%*spmap=%p(resident=<unknown>)\n", indent(2), "",
		       D(vm_map, vm_map)->pmap);
		if (verbose && name != NULL)
			printf("\t%*s([ %s ])\n", indent(2), "", name);
	}

	dump_vm_map_entry(kd, proc, vmspace, header, 1);

	/*
	 * we're not recursing into a submap, so print headers
	 */
	if (recurse < 2) {
		/* headers */
#ifdef DISABLED_HEADERS
		if (print_map)
			printf("%-*s %-*s rwx RWX CPY NCP I W A\n",
			       (int)sizeof(long) * 2 + 2, "Start",
			       (int)sizeof(long) * 2 + 2, "End");
		if (print_maps)
			printf("%-*s %-*s rwxp %-*s Dev   Inode      File\n",
			       (int)sizeof(long) * 2 + 0, "Start",
			       (int)sizeof(long) * 2 + 0, "End",
			       (int)sizeof(long) * 2 + 0, "Offset");
		if (print_solaris)
			printf("%-*s %*s Protection        File\n",
			       (int)sizeof(long) * 2 + 0, "Start",
			       (int)sizeof(int) * 2 - 1,  "Size ");
#endif
		if (print_all)
			printf("%-*s %-*s %*s %-*s rwxpc  RWX  I/W/A Dev  %*s"
			       " - File\n",
			       (int)sizeof(long) * 2, "Start",
			       (int)sizeof(long) * 2, "End",
			       (int)sizeof(int)  * 2, "Size ",
			       (int)sizeof(long) * 2, "Offset",
			       (int)sizeof(int)  * 2, "Inode");
	}

	/* these are the "sub entries" */
	total = 0;
	next = D(header, vm_map_entry)->next;
	last = P(header);
	end = 0;

	while (next != 0 && next != last) {
		addr = (u_long)next;
		A(vm_map_entry) = addr;
		S(vm_map_entry) = sizeof(struct vm_map_entry);
		KDEREF(kd, vm_map_entry);
		next = D(vm_map_entry, vm_map_entry)->next;

		if (end == 0)
			end = D(vm_map_entry, vm_map_entry)->start;
		else if (verbose > 1 &&
		    end != D(vm_map_entry, vm_map_entry)->start)
			printf("%*s[%lu pages / %luK]\n", indent(2), "",
			       (D(vm_map_entry, vm_map_entry)->start - end) /
			       page_size,
			       (D(vm_map_entry, vm_map_entry)->start - end) /
			       1024);
		total += dump_vm_map_entry(kd, proc, vmspace, vm_map_entry, 0);

		end = D(vm_map_entry, vm_map_entry)->end;
	}

	/*
	 * we're not recursing into a submap, so print totals
	 */
	if (recurse < 2) {
		if (print_solaris)
			printf("%-*s %8luK\n",
			       (int)sizeof(void *) * 2 - 2, " total",
			       (unsigned long)total);
		if (print_all)
			printf("%-*s %9luk\n",
			       (int)sizeof(void *) * 4 - 1, " total",
			       (unsigned long)total);
	}
}

size_t
dump_vm_map_entry(kvm_t *kd, struct kinfo_proc2 *proc, struct kbit *vmspace,
	struct kbit *vm_map_entry, int ishead)
{
	struct kbit kbit[3];
	struct kbit *uvm_obj, *vp, *vfs;
	struct vm_map_entry *vme;
	size_t sz;
	char *name;
	dev_t dev;
	ino_t inode;

	if (S(vm_map_entry) == (size_t)-1) {
		heapfound = 1;
		S(vm_map_entry) = sizeof(struct vm_map_entry);
		KDEREF(kd, vm_map_entry);
	}

	uvm_obj = &kbit[0];
	vp = &kbit[1];
	vfs = &kbit[2];

	A(uvm_obj) = 0;
	A(vp) = 0;
	A(vfs) = 0;

	vme = D(vm_map_entry, vm_map_entry);

	if ((ishead && (debug & PRINT_VM_MAP_HEADER)) ||
	    (!ishead && (debug & PRINT_VM_MAP_ENTRY))) {
		printf("%*s%s %p = {", indent(2), "",
		       ishead ? "vm_map.header" : "vm_map_entry",
		       P(vm_map_entry));
		printf(" prev = %p,", vme->prev);
		printf(" next = %p,\n", vme->next);
		printf("%*s    start = %lx,", indent(2), "", vme->start);
		printf(" end = %lx,", vme->end);
		printf(" object.uvm_obj/sub_map = %p,\n", vme->object.uvm_obj);
		printf("%*s    offset = %" PRIx64 ",", indent(2), "",
		       vme->offset);
		printf(" etype = %x <%s%s%s%s >,", vme->etype,
		       UVM_ET_ISOBJ(vme) ? " OBJ" : "",
		       UVM_ET_ISSUBMAP(vme) ? " SUBMAP" : "",
		       UVM_ET_ISCOPYONWRITE(vme) ? " COW" : "",
		       UVM_ET_ISNEEDSCOPY(vme) ? " NEEDSCOPY" : "");
		printf(" protection = %x,\n", vme->protection);
		printf("%*s    max_protection = %x,", indent(2), "",
		       vme->max_protection);
		printf(" inheritance = %d,", vme->inheritance);
		printf(" wired_count = %d,\n", vme->wired_count);
		printf("%*s    aref = { ar_pageoff = %x, ar_amap = %p },",
		       indent(2), "", vme->aref.ar_pageoff, vme->aref.ar_amap);
		printf(" advice = %d,\n", vme->advice);
		printf("%*s    flags = %x <%s%s%s%s%s > }\n", indent(2), "",
		       vme->flags,
		       vme->flags & UVM_MAP_KERNEL ? " KERNEL" : "",
		       vme->flags & UVM_MAP_KMAPENT ? " KMAPENT" : "",
		       vme->flags & UVM_MAP_FIRST ? " FIRST" : "",
		       vme->flags & UVM_MAP_QUANTUM ? " QUANTUM" : "",
		       vme->flags & UVM_MAP_NOMERGE ? " NOMERGE" : "");
	}

	if ((debug & PRINT_VM_AMAP) && (vme->aref.ar_amap != NULL)) {
		struct kbit akbit, *amap;

		amap = &akbit;
		P(amap) = vme->aref.ar_amap;
		S(amap) = sizeof(struct vm_amap);
		KDEREF(kd, amap);
		dump_amap(kd, amap);
	}

	if (ishead)
		return (0);

	A(vp) = 0;
	A(uvm_obj) = 0;

	if (vme->object.uvm_obj != NULL) {
		P(uvm_obj) = vme->object.uvm_obj;
		S(uvm_obj) = sizeof(struct uvm_object);
		KDEREF(kd, uvm_obj);
		if (UVM_ET_ISOBJ(vme) &&
		    UVM_OBJ_IS_VNODE(D(uvm_obj, uvm_object))) {
			P(vp) = P(uvm_obj);
			S(vp) = sizeof(struct vnode);
			KDEREF(kd, vp);
		}
	}

	A(vfs) = 0;

	if (P(vp) != NULL && D(vp, vnode)->v_mount != NULL) {
		P(vfs) = D(vp, vnode)->v_mount;
		S(vfs) = sizeof(struct mount);
		KDEREF(kd, vfs);
		D(vp, vnode)->v_mount = D(vfs, mount);
	}

	/*
	 * dig out the device number and inode number from certain
	 * file system types.
	 */
#define V_DATA_IS(vp, type, d, i) do { \
	struct kbit data; \
	P(&data) = D(vp, vnode)->v_data; \
	S(&data) = sizeof(*D(&data, type)); \
	KDEREF(kd, &data); \
	dev = D(&data, type)->d; \
	inode = D(&data, type)->i; \
} while (0/*CONSTCOND*/)

	dev = 0;
	inode = 0;

	if (A(vp) &&
	    D(vp, vnode)->v_type == VREG &&
	    D(vp, vnode)->v_data != NULL) {
		switch (D(vp, vnode)->v_tag) {
		case VT_UFS:
		case VT_LFS:
		case VT_EXT2FS:
			V_DATA_IS(vp, inode, i_dev, i_number);
			break;
		case VT_ISOFS:
			V_DATA_IS(vp, iso_node, i_dev, i_number);
			break;
		default:
			break;
		}
	}

	name = findname(kd, vmspace, vm_map_entry, vp, vfs, uvm_obj);

	if (print_map) {
		printf("%*s0x%lx 0x%lx %c%c%c %c%c%c %s %s %d %d %d",
		       indent(2), "",
		       vme->start, vme->end,
		       (vme->protection & VM_PROT_READ) ? 'r' : '-',
		       (vme->protection & VM_PROT_WRITE) ? 'w' : '-',
		       (vme->protection & VM_PROT_EXECUTE) ? 'x' : '-',
		       (vme->max_protection & VM_PROT_READ) ? 'r' : '-',
		       (vme->max_protection & VM_PROT_WRITE) ? 'w' : '-',
		       (vme->max_protection & VM_PROT_EXECUTE) ? 'x' : '-',
		       UVM_ET_ISCOPYONWRITE(vme) ? "COW" : "NCOW",
		       UVM_ET_ISNEEDSCOPY(vme) ? "NC" : "NNC",
		       vme->inheritance, vme->wired_count,
		       vme->advice);
		if (verbose) {
			if (inode)
				printf(" %u,%u %llu", major(dev), minor(dev),
				    (unsigned long long)inode);
			if (name[0])
				printf(" %s", name);
		}
		printf("\n");
	}

	if (print_maps) {
		printf("%*s%0*lx-%0*lx %c%c%c%c %0*" PRIx64 " %02x:%02x %llu     %s\n",
		       indent(2), "",
		       (int)sizeof(void *) * 2, vme->start,
		       (int)sizeof(void *) * 2, vme->end,
		       (vme->protection & VM_PROT_READ) ? 'r' : '-',
		       (vme->protection & VM_PROT_WRITE) ? 'w' : '-',
		       (vme->protection & VM_PROT_EXECUTE) ? 'x' : '-',
		       UVM_ET_ISCOPYONWRITE(vme) ? 'p' : 's',
		       (int)sizeof(void *) * 2,
		       vme->offset,
		       major(dev), minor(dev), (unsigned long long)inode,
		       (name[0] != ' ') || verbose ? name : "");
	}

	if (print_ddb) {
		printf("%*s - %p: 0x%lx->0x%lx: obj=%p/0x%" PRIx64 ", amap=%p/%d\n",
		       indent(2), "",
		       P(vm_map_entry), vme->start, vme->end,
		       vme->object.uvm_obj, vme->offset,
		       vme->aref.ar_amap, vme->aref.ar_pageoff);
		printf("\t%*ssubmap=%c, cow=%c, nc=%c, prot(max)=%d/%d, inh=%d, "
		       "wc=%d, adv=%d\n",
		       indent(2), "",
		       UVM_ET_ISSUBMAP(vme) ? 'T' : 'F',
		       UVM_ET_ISCOPYONWRITE(vme) ? 'T' : 'F',
		       UVM_ET_ISNEEDSCOPY(vme) ? 'T' : 'F',
		       vme->protection, vme->max_protection,
		       vme->inheritance, vme->wired_count, vme->advice);
		if (verbose) {
			printf("\t%*s", indent(2), "");
			if (inode)
				printf("(dev=%u,%u ino=%llu [%s] [%p])\n",
				    major(dev), minor(dev),
				    (unsigned long long)inode, name, P(vp));
			else if (name[0] == ' ')
				printf("(%s)\n", &name[2]);
			else
				printf("(%s)\n", name);
		}
	}

	sz = 0;
	if (print_solaris) {
		char prot[30];

		prot[0] = '\0';
		prot[1] = '\0';
		if (vme->protection & VM_PROT_READ)
			strlcat(prot, "/read", sizeof(prot));
		if (vme->protection & VM_PROT_WRITE)
			strlcat(prot, "/write", sizeof(prot));
		if (vme->protection & VM_PROT_EXECUTE)
			strlcat(prot, "/exec", sizeof(prot));

		sz = (size_t)((vme->end - vme->start) / 1024);
		printf("%*s%0*lX %6luK %-15s   %s\n",
		       indent(2), "",
		       (int)sizeof(void *) * 2,
		       (unsigned long)vme->start,
		       (unsigned long)sz,
		       &prot[1],
		       name);
	}

	if (print_all) {
		sz = (size_t)((vme->end - vme->start) / 1024);
		printf(A(vp) ?
		       "%*s%0*lx-%0*lx %7luk %0*" PRIx64 " %c%c%c%c%c (%c%c%c) %d/%d/%d %02u:%02u %7llu - %s [%p]\n" :
		       "%*s%0*lx-%0*lx %7luk %0*" PRIx64 " %c%c%c%c%c (%c%c%c) %d/%d/%d %02u:%02u %7llu - %s\n",
		       indent(2), "",
		       (int)sizeof(void *) * 2,
		       vme->start,
		       (int)sizeof(void *) * 2,
		       vme->end - (vme->start != vme->end ? 1 : 0),
		       (unsigned long)sz,
		       (int)sizeof(void *) * 2,
		       vme->offset,
		       (vme->protection & VM_PROT_READ) ? 'r' : '-',
		       (vme->protection & VM_PROT_WRITE) ? 'w' : '-',
		       (vme->protection & VM_PROT_EXECUTE) ? 'x' : '-',
		       UVM_ET_ISCOPYONWRITE(vme) ? 'p' : 's',
		       UVM_ET_ISNEEDSCOPY(vme) ? '+' : '-',
		       (vme->max_protection & VM_PROT_READ) ? 'r' : '-',
		       (vme->max_protection & VM_PROT_WRITE) ? 'w' : '-',
		       (vme->max_protection & VM_PROT_EXECUTE) ? 'x' : '-',
		       vme->inheritance,
		       vme->wired_count,
		       vme->advice,
		       major(dev), minor(dev), (unsigned long long)inode,
		       name, P(vp));
	}

	/* no access allowed, don't count space */
	if ((vme->protection & rwx) == 0)
		sz = 0;

	if (recurse && UVM_ET_ISSUBMAP(vme)) {
		struct kbit mkbit, *submap;

		recurse++;
		submap = &mkbit;
		P(submap) = vme->object.sub_map;
		S(submap) = sizeof(*vme->object.sub_map);
		KDEREF(kd, submap);
		dump_vm_map(kd, proc, vmspace, submap, "submap");
		recurse--;
	}

	return (sz);
}

void
dump_amap(kvm_t *kd, struct kbit *amap)
{
	struct vm_anon **am_anon;
	int *am_slots;
	int *am_bckptr;
	int *am_ppref;
	size_t i, r, l, e;

	if (S(amap) == (size_t)-1) {
		heapfound = 1;
		S(amap) = sizeof(struct vm_amap);
		KDEREF(kd, amap);
	}

	printf("%*s  amap %p = { am_ref = %d, "
	       "am_flags = %x,\n"
	       "%*s      am_maxslot = %d, am_nslot = %d, am_nused = %d, "
	       "am_slots = %p,\n"
	       "%*s      am_bckptr = %p, am_anon = %p, am_ppref = %p }\n",
	       indent(2), "",
	       P(amap),
	       D(amap, amap)->am_ref,
	       D(amap, amap)->am_flags,
	       indent(2), "",
	       D(amap, amap)->am_maxslot,
	       D(amap, amap)->am_nslot,
	       D(amap, amap)->am_nused,
	       D(amap, amap)->am_slots,
	       indent(2), "",
	       D(amap, amap)->am_bckptr,
	       D(amap, amap)->am_anon,
	       D(amap, amap)->am_ppref);
	
	if (!(debug & DUMP_VM_AMAP_DATA))
		return;

	/*
	 * Assume that sizeof(struct vm_anon *) >= sizeof(size_t) and
	 * allocate that amount of space.
	 */
	l = sizeof(struct vm_anon *) * D(amap, amap)->am_maxslot;
	am_anon = malloc(l);
	_KDEREF(kd, (u_long)D(amap, amap)->am_anon, am_anon, l);

	l = sizeof(int) * D(amap, amap)->am_maxslot;
	am_bckptr = malloc(l);
	_KDEREF(kd, (u_long)D(amap, amap)->am_bckptr, am_bckptr, l);

	l = sizeof(int) * D(amap, amap)->am_maxslot;
	am_slots = malloc(l);
	_KDEREF(kd, (u_long)D(amap, amap)->am_slots, am_slots, l);

	if (D(amap, amap)->am_ppref != NULL &&
	    D(amap, amap)->am_ppref != PPREF_NONE) {
		l = sizeof(int) * D(amap, amap)->am_maxslot;
		am_ppref = malloc(l);
		_KDEREF(kd, (u_long)D(amap, amap)->am_ppref, am_ppref, l);
	} else {
		am_ppref = NULL;
	}

	printf(" page# %9s  %8s", "am_bckptr", "am_slots");
	if (am_ppref)
		printf("  %8s               ", "am_ppref");
	printf("  %10s\n", "am_anon");

	l = r = 0;
	e = verbose > 1 ? D(amap, amap)->am_maxslot : D(amap, amap)->am_nslot;
	for (i = 0; i < e; i++) {
		printf("  %4lx", (unsigned long)i);

		if (am_anon[i] || verbose > 1)
			printf("  %8x", am_bckptr[i]);
		else
			printf("  %8s", "-");

		if (i < D(amap, amap)->am_nused || verbose > 1)
			printf("  %8x", am_slots[i]);
		else
			printf("  %8s", "-");

		if (am_ppref) {
			if (l == 0 || r || verbose > 1)
				printf("  %8d", am_ppref[i]);
			else
				printf("  %8s", "-");
			r = 0;
			if (l == 0) {
				if (am_ppref[i] > 0) {
					r = am_ppref[i] - 1;
					l = 1;
				} else {
					r = -am_ppref[i] - 1;
					l = am_ppref[i + 1];
				}
				printf("  (%4ld @ %4ld)", (long)l, (long)r);
				r = (l > 1) ? 1 : 0;
			}
			else
				printf("               ");
			l--;
		}

		dump_vm_anon(kd, am_anon, (int)i);
	}

	free(am_anon);
	free(am_bckptr);
	free(am_slots);
	if (am_ppref)
		free(am_ppref);
}

static void
dump_vm_anon(kvm_t *kd, struct vm_anon **alist, int i)
{

	printf("  %10p", alist[i]);

	if (debug & PRINT_VM_ANON) {
		struct kbit kbit, *anon = &kbit;

		A(anon) = (u_long)alist[i];
		S(anon) = sizeof(struct vm_anon);
		if (A(anon) == 0) {
			printf(" = { }\n");
			return;
		}
		else
			KDEREF(kd, anon);

		printf(" = { an_ref = %d, an_page = %p, an_swslot = %d }",
		    D(anon, anon)->an_ref, D(anon, anon)->an_page,
		    D(anon, anon)->an_swslot);
	}

	printf("\n");
}

static char*
findname(kvm_t *kd, struct kbit *vmspace,
	 struct kbit *vm_map_entry, struct kbit *vp,
	 struct kbit *vfs, struct kbit *uvm_obj)
{
	static char buf[1024], *name;
	struct vm_map_entry *vme;
	size_t l;

	vme = D(vm_map_entry, vm_map_entry);

	if (UVM_ET_ISOBJ(vme)) {
		if (A(vfs)) {
			l = (unsigned)strlen(D(vfs, mount)->mnt_stat.f_mntonname);
			switch (search_cache(kd, vp, &name, buf, sizeof(buf))) { 
			    case 0: /* found something */
                                name--;
                                *name = '/';
				/*FALLTHROUGH*/
			    case 2: /* found nothing */
				name -= 5;
				memcpy(name, " -?- ", (size_t)5);
				name -= l;
				memcpy(name,
				       D(vfs, mount)->mnt_stat.f_mntonname, l);
				break;
			    case 1: /* all is well */
				name--;
				*name = '/';
				if (l != 1) {
					name -= l;
					memcpy(name,
					       D(vfs, mount)->mnt_stat.f_mntonname, l);
				}
				break;
			}
		}
		else if (UVM_OBJ_IS_DEVICE(D(uvm_obj, uvm_object))) {
			struct kbit kdev;
			dev_t dev;

			P(&kdev) = P(uvm_obj);
			S(&kdev) = sizeof(struct uvm_device);
			KDEREF(kd, &kdev);
			dev = D(&kdev, uvm_device)->u_device;
			name = devname(dev, S_IFCHR);
			if (name != NULL)
				snprintf(buf, sizeof(buf), "/dev/%s", name);
			else
				snprintf(buf, sizeof(buf), "  [ device %d,%d ]",
					 major(dev), minor(dev));
			name = buf;
		}
		else if (UVM_OBJ_IS_AOBJ(D(uvm_obj, uvm_object))) 
			name = "  [ uvm_aobj ]";
		else if (UVM_OBJ_IS_UBCPAGER(D(uvm_obj, uvm_object)))
			name = "  [ ubc_pager ]";
		else if (UVM_OBJ_IS_VNODE(D(uvm_obj, uvm_object)))
			name = "  [ ?VNODE? ]";
		else {
			snprintf(buf, sizeof(buf), "  [ ?? %p ?? ]",
				 D(uvm_obj, uvm_object)->pgops);
			name = buf;
		}
	}

	else if ((char *)D(vmspace, vmspace)->vm_maxsaddr <=
		 (char *)vme->start &&
		 ((char *)D(vmspace, vmspace)->vm_maxsaddr + (size_t)maxssiz) >=
		 (char *)vme->end)
		name = "  [ stack ]";

	else if (!heapfound &&
		 (vme->protection & rwx) == rwx &&
		 vme->start >= (u_long)D(vmspace, vmspace)->vm_daddr) {
		heapfound = 1;
		name = "  [ heap ]";
	}

	else if (UVM_ET_ISSUBMAP(vme)) {
		const char *sub = mapname(vme->object.sub_map);
		snprintf(buf, sizeof(buf), "  [ %s ]", sub ? sub : "(submap)");
		name = buf;
	}

	else
		name = "  [ anon ]";

	return (name);
}

static int
search_cache(kvm_t *kd, struct kbit *vp, char **name, char *buf, size_t blen)
{
	char *o, *e;
	struct cache_entry *ce;
	struct kbit svp;

	if (nchashtbl == NULL)
		load_name_cache(kd);

	P(&svp) = P(vp);
	S(&svp) = sizeof(struct vnode);

	e = &buf[blen - 1];
	o = e;
	do {
		LIST_FOREACH(ce, &lcache, ce_next)
			if (ce->ce_vp == P(&svp))
				break;
		if (ce && ce->ce_vp == P(&svp)) {
			if (o != e)
				*(--o) = '/';
			o -= ce->ce_nlen;
			memcpy(o, ce->ce_name, (unsigned)ce->ce_nlen);
			P(&svp) = ce->ce_pvp;
		}
		else
			break;
	} while (1/*CONSTCOND*/);
	*e = '\0';
	*name = o;

	if (e == o)
		return (2);

	KDEREF(kd, &svp);
	return (D(&svp, vnode)->v_vflag & VV_ROOT);
}
