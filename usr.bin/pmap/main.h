/*      $NetBSD: main.h,v 1.3 2003/03/28 23:10:33 atatat Exp $ */

/*
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
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

extern int debug, verbose, recurse, page_size;
extern int print_all, print_map, print_maps, print_solaris, print_ddb;
extern u_long kernel_map_addr;
extern void *uvm_vnodeops, *uvm_deviceops, *aobj_pager, *ubc_pager;
extern rlim_t maxssiz;

LIST_HEAD(nchashhead, namecache);
extern struct nchashhead *nchashtbl;

struct cache_entry {
	LIST_ENTRY(cache_entry) ce_next;
	struct vnode *ce_vp, *ce_pvp;
	u_long ce_cid, ce_pcid;
	int ce_nlen;
	char ce_name[256];
};

LIST_HEAD(cache_head, cache_entry);
extern struct cache_head lcache;

#define PRINT_VMSPACE		0x00000001
#define PRINT_VM_MAP		0x00000002
#define PRINT_VM_MAP_HEADER	0x00000004
#define PRINT_VM_MAP_ENTRY	0x00000008
#define PRINT_VM_AMAP		0x00000010
#define DUMP_VM_AMAP_DATA	0x00000020
#define PRINT_VM_ANON		0x00000040
#define DUMP_NAMEI_CACHE	0x00001000

void (*process_map)(kvm_t *, pid_t, struct kinfo_proc2 *);
void load_name_cache(kvm_t *);
const char *mapname(void *);
