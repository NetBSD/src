/*	$NetBSD: disklabel.h,v 1.2 2000/03/22 20:38:05 soren Exp $	*/

#define LABELSECTOR	1		/* sector containing label */
#define LABELOFFSET	0		/* offset of label in sector */
#define MAXPARTITIONS	16
#define RAW_PART	3

/* Pull in MBR partition definitions. */
#include <sys/disklabel_mbr.h>
 
#ifndef __ASSEMBLER__ 
#include <sys/dkbad.h>                
struct cpu_disklabel {
        struct mbr_partition dosparts[NMBRPART];
        struct dkbad bad;
};
#endif
