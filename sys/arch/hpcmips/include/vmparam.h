/*	$NetBSD: vmparam.h,v 1.5 2009/12/14 00:46:04 matt Exp $	*/

#include <mips/vmparam.h>
#include <sys/kcore.h>

#define	VM_PHYSSEG_MAX		5

extern phys_ram_seg_t mem_clusters[];
extern int mem_cluster_cnt;
