/*	$NetBSD: vmparam.h,v 1.3.4.1 2002/02/11 20:08:09 jdolecek Exp $	*/

#include <mips/vmparam.h>
#include <sys/kcore.h>

#define	VM_PHYSSEG_MAX		5

#define	VM_NFREELIST		1
#define	VM_FREELIST_DEFAULT	0

extern phys_ram_seg_t mem_clusters[];
extern int mem_cluster_cnt;
