/*	$NetBSD: vmparam.h,v 1.6 2019/03/28 08:33:07 christos Exp $	*/

#include <mips/vmparam.h>
#ifdef _KERNEL
#include <sys/kcore.h>

#define	VM_PHYSSEG_MAX		5

extern phys_ram_seg_t mem_clusters[];
extern int mem_cluster_cnt;
#endif
