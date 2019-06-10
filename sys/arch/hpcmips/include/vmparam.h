/*	$NetBSD: vmparam.h,v 1.5.64.1 2019/06/10 22:06:17 christos Exp $	*/

#include <mips/vmparam.h>
#ifdef _KERNEL
#include <sys/kcore.h>

#define	VM_PHYSSEG_MAX		5

extern phys_ram_seg_t mem_clusters[];
extern int mem_cluster_cnt;
#endif
