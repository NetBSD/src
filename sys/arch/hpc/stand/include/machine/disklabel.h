/*	$NetBSD: disklabel.h,v 1.5 2005/12/11 12:17:29 christos Exp $	*/

/* Windows CE architecture */

#define	LABELSECTOR	1		/* sector containing label */
#define	LABELOFFSET	0		/* offset of label in sector */
#define	MAXPARTITIONS	8		/* number of partitions */
#define	RAW_PART	3		/* raw partition: XX?d (XXX) */

/* Pull in MBR partition definitions. */

#if !defined(__GNUC__)
#include <sys/cdefs.h>		/* force <sys/cdefs.h> to be read */
#undef __packed			/* so that we can undo the damage */
#define	__packed
#pragma pack(push, _sys_bootblock_h, 1)
#endif

#include <sys/bootblock.h>

#if !defined(__GNUC__)
#pragma pack(pop, _sys_bootblock_h)
#endif
