/*	$NetBSD: direntry.h,v 1.5 1994/07/16 21:33:19 cgd Exp $	*/

/*
 * Written by Paul Popelka (paulp@uts.amdahl.com)
 * 
 * You can do anything you want with this software, just don't say you wrote
 * it, and don't remove this notice.
 * 
 * This software is provided "as is".
 * 
 * The author supplies this software to be publicly redistributed on the
 * understanding that the author is not responsible for the correct
 * functioning of this software in any circumstances and is not liable for
 * any damages caused by this software.
 * 
 * October 1992
 */

/*
 * Structure of a dos directory entry.
 */
struct direntry {
	u_char deName[8];	/* filename, blank filled */
#define	SLOT_EMPTY	0x00	/* slot has never been used */
#define	SLOT_E5		0x05	/* the real value is 0xe5 */
#define	SLOT_DELETED	0xe5	/* file in this slot deleted */
	u_char deExtension[3];	/* extension, blank filled */
	u_char deAttributes;	/* file attributes */
#define	ATTR_NORMAL	0x00	/* normal file */
#define	ATTR_READONLY	0x01	/* file is readonly */
#define	ATTR_HIDDEN	0x02	/* file is hidden */
#define	ATTR_SYSTEM	0x04	/* file is a system file */
#define	ATTR_VOLUME	0x08	/* entry is a volume label */
#define	ATTR_DIRECTORY	0x10	/* entry is a directory name */
#define	ATTR_ARCHIVE	0x20	/* file is new or modified */
	u_char deReserved[10];	/* reserved */
	u_char deTime[2];	/* create/last update time */
	u_char deDate[2];	/* create/last update date */
	u_char deStartCluster[2]; /* starting cluster of file */
	u_char deFileSize[4];	/* size of file in bytes */
};

/*
 * This is the format of the contents of the deTime field in the direntry
 * structure.
 * We don't use bitfields because we don't know how compilers for
 * arbitrary machines will lay them out.
 */
#define DT_2SECONDS_MASK	0x1F	/* seconds divided by 2 */
#define DT_2SECONDS_SHIFT	0
#define DT_MINUTES_MASK		0x7E0	/* minutes */
#define DT_MINUTES_SHIFT	5
#define DT_HOURS_MASK		0xF800	/* hours */
#define DT_HOURS_SHIFT		11

/*
 * This is the format of the contents of the deDate field in the direntry
 * structure.
 */
#define DD_DAY_MASK		0x1F	/* day of month */
#define DD_DAY_SHIFT		0
#define DD_MONTH_MASK		0x1E0	/* month */
#define DD_MONTH_SHIFT		5
#define DD_YEAR_MASK		0xFE00	/* year - 1980 */
#define DD_YEAR_SHIFT		9

#if defined(KERNEL)
void unix2dostime __P((struct timespec * tsp, u_short * ddp, u_short * dtp));
void dos2unixtime __P((u_short dd, u_short dt, struct timespec * tsp));
int dos2unixfn __P((u_char dn[11], u_char * un));
void unix2dosfn __P((u_char * un, u_char dn[11], int unlen));
#endif				/* defined(KERNEL) */
