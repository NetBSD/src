/*	$NetBSD: vmparam.h,v 1.4.122.1 2010/03/11 15:02:25 yamt Exp $	*/

#include <mips/vmparam.h>
#include <sys/kcore.h>

#define	VM_PHYSSEG_MAX		5

extern phys_ram_seg_t mem_clusters[];
extern int mem_cluster_cnt;
