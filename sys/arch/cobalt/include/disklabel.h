/*	$NetBSD: disklabel.h,v 1.1 2000/03/19 23:07:46 soren Exp $	*/

#define MAXPARTITIONS	16
#define RAW_PART	3

/* Pull in MBR partition definitions. */
#include <sys/disklabel_mbr.h>
 
#if 0
struct cpu_disklabel {
        struct mbr_partition dosparts[NMBRPART];
};
#endif

/* XXX */

#ifndef __ASSEMBLER__ 
#include <sys/dkbad.h>                
struct cpu_disklabel {
        struct mbr_partition mbrparts[NMBRPART];
        struct dkbad bad;
};
#endif
