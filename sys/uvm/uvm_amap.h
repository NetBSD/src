/*	$NetBSD: uvm_amap.h,v 1.3 1998/02/07 11:07:58 mrg Exp $	*/

/*
 * XXXCDC: "ROUGH DRAFT" QUALITY UVM PRE-RELEASE FILE!   
 *	   >>>USE AT YOUR OWN RISK, WORK IS NOT FINISHED<<<
 */
/*
 *
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Charles D. Cranor and
 *      Washington University.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 * from: Id: uvm_amap.h,v 1.1.2.7 1998/01/05 18:12:56 chuck Exp
 */

/*
 * uvm_amap.h
 */

UVMHIST_DECL(maphist);

/*
 * defines for handling of large sparce amaps.
 *
 * currently the kernel likes to allocate large chunks of VM to reserve
 * them for possible use.   for example, it allocates (reserves) a large 
 * chunk of user VM for possible stack growth.   most of the time only 
 * a page or two of this VM is actually used.   since the stack is anonymous
 * memory it makes sense for it to live in an amap, but if we allocated
 * an amap for the entire stack range we could end up wasting a large 
 * amount of malloc'd KVM.   
 * 
 * for example, on the i386 at boot time we allocate two amaps for the stack 
 * of /sbin/init: 
 *  1. a 7680 slot amap at protection 0 (reserve space for stack)
 *  2. a 512 slot amap at protection 7 (top of stack)
 *
 * most of that VM is never mapped or used.   
 *
 * to avoid allocating amap resources for the whole range we have the
 * VM system look for maps that are larger than UVM_AMAP_LARGE slots
 * (note that 1 slot = 1 vm_page).   for maps that are large, we attempt
 * to break them up into UVM_AMAP_CHUNK slot sized amaps.
 *
 * so, in the i386 example, the 7680 slot area is never referenced so
 * nothing gets allocated.   the 512 slot area is referenced, and it
 * gets divided into 16 slot chunks (hopefully with one 16 slot chunk
 * being enough to handle the whole stack...).
 */

#define UVM_AMAP_LARGE		256	/* # of slots in "large" amap */
#define UVM_AMAP_CHUNK		16	/* # of slots to chunk large amaps in */


#ifdef DIAGNOSTIC
#define AMAP_B2SLOT(S,B) { \
	if ((B) % PAGE_SIZE) \
		panic("AMAP_B2SLOT: invalid byte count"); \
	(S) = (B) / PAGE_SIZE; \
}
#else
#define AMAP_B2SLOT(S,B) (S) = (B) / PAGE_SIZE
#endif

#ifdef VM_AMAP_PPREF
#define PPREF_NONE ((int *) -1)
#endif

/*
 * handle inline options
 */

#ifdef UVM_AMAP_INLINE
#define AMAP_INLINE static __inline
#else 
#define AMAP_INLINE /* nothing */
#endif /* UVM_AMAP_INLINE */

/*
 * prototypes: the following prototypes define the interface to amaps
 */

AMAP_INLINE vm_offset_t amap_add __P((struct vm_aref *, vm_offset_t,
                                                struct vm_anon *, int));
struct vm_amap *amap_alloc __P((vm_offset_t, vm_offset_t, int));
void amap_copy __P((vm_map_t, vm_map_entry_t, int, boolean_t,
			vm_offset_t, vm_offset_t));
void amap_cow_now __P((vm_map_t, vm_map_entry_t));
void amap_extend __P((vm_map_entry_t, vm_size_t));
void amap_free __P((struct vm_amap *)); 
AMAP_INLINE struct vm_anon *amap_lookup __P((struct vm_aref *, vm_offset_t));
AMAP_INLINE void amap_lookups __P((struct vm_aref *, vm_offset_t, 
				   struct vm_anon **, int));
#ifdef VM_AMAP_PPREF
void amap_pp_adjref __P((struct vm_amap *, int, vm_size_t, int));
void amap_pp_establish __P((struct vm_amap *));
#endif
AMAP_INLINE void amap_ref __P((vm_map_entry_t, int));
void amap_share_protect __P((vm_map_entry_t, vm_prot_t));
void amap_splitref __P((struct vm_aref *, struct vm_aref *, vm_offset_t));
AMAP_INLINE void amap_unadd __P((struct vm_amap *, vm_offset_t));
AMAP_INLINE void amap_unref __P((vm_map_entry_t, int));
void amap_wipeout __P((struct vm_amap *));
#ifdef VM_AMAP_PPREF
void amap_wiperange __P((struct vm_amap *, int, int));
#endif

struct vm_anon *uvm_analloc __P((void));
void uvm_anfree __P((struct vm_anon *));
void uvm_anon_init __P((void));
void uvm_anon_add __P((int));
struct vm_page *uvm_anon_lockloanpg __P((struct vm_anon *));
