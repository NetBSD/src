/*	$NetBSD: disklabel.h,v 1.3 2000/03/24 20:16:27 soren Exp $	*/

#ifndef _MACHINE_DISKLABEL_H_
#define _MACHINE_DISKLABEL_H_

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

#endif /* _MACHINE_DISKLABEL_H_ */
