/*	$NetBSD: vmparam.h,v 1.3.8.1 2002/02/28 04:09:58 nathanw Exp $	*/

#include <mips/vmparam.h>
#include <sys/kcore.h>

#define	VM_PHYSSEG_MAX		5

#define	VM_NFREELIST		1
#define	VM_FREELIST_DEFAULT	0

extern phys_ram_seg_t mem_clusters[];
extern int mem_cluster_cnt;
