/*	$NetBSD: disklabel.h,v 1.2 2003/10/08 04:25:44 lukem Exp $	*/

/* Windows CE architecture */

#define	LABELSECTOR	1		/* sector containing label */
#define	LABELOFFSET	0		/* offset of label in sector */
#define	MAXPARTITIONS	8		/* number of partitions */
#define	RAW_PART	3		/* raw partition: XX?d (XXX) */

/* Pull in MBR partition definitions. */
#include <sys/bootblock.h>
