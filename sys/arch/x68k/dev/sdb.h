/*	$NetBSD: sdb.h,v 1.1.1.1 1996/05/05 12:17:03 oki Exp $	*/

/*
 * Copyright (c) 1993 Masaru Oki
 * All rights reserved.
 */

struct x68k_diskblock {
	unsigned char  db_idname[8];	 /* identifier name 'X68SCSI1' */
	unsigned short db_blocklen;	 /* logical block length (bytes) */
	unsigned long  db_logicalblocks; /* logical block size */
	unsigned char  db_extflag;	 /* 1, if 10bytes scsi transfer enable */
	unsigned char  db_removable;	 /* 1, if removal */
};
