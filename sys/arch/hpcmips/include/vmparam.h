/*	$NetBSD: vmparam.h,v 1.4.140.1 2009/12/11 20:22:15 matt Exp $	*/

#include <mips/vmparam.h>
#include <sys/kcore.h>

#define	VM_PHYSSEG_MAX		5

extern phys_ram_seg_t mem_clusters[];
extern int mem_cluster_cnt;
