/*	$NetBSD: disklabel.h,v 1.1 2001/02/09 18:35:25 uch Exp $	*/

/* Windows CE architecture */

#define	LABELSECTOR	1		/* sector containing label */
#define	LABELOFFSET	0		/* offset of label in sector */
#define	MAXPARTITIONS	8		/* number of partitions */
#define	RAW_PART	3		/* raw partition: XX?d (XXX) */

/* Pull in MBR partition definitions. */
#include <sys/disklabel_mbr.h>
