/*	$NetBSD: disklabel.h,v 1.1 2000/06/14 15:39:57 soren Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.netbsd.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _MACHINE_DISKLABEL_H_
#define _MACHINE_DISKLABEL_H_

/*
 * SGI partition conventions:
 *
 * Partition 0 - root
 * Partition 1 - swap
 * Partition 6 - usr
 * Partition 7 - volume body
 * Partition 8 - volume header
 * Partition 10 - whole disk
 */

/*
 * XXX It might be convenient to have MAXPARTITIONS 2 like on other ports.
 * How to sanely map partition numbers in that case?
 */
#define MAXPARTITIONS	16
#define RAW_PART	10

#define LABELSECTOR	0
#define LABELOFFSET	0

struct cpu_disklabel {
	int	cd_dummy;
};

struct sgilabel {
#define SGILABEL_MAGIC	0xbe5a941
	u_int32_t	magic;
	int16_t		root;
	int16_t		swap;
	char		bootfile[16];
	char		_devparms[48];
	struct {
		char 		name[8];
		int32_t		block;
		int32_t		bytes;
	} voldir[15];
	struct {
		int32_t 	blocks;
		int32_t 	first;
		int32_t 	type;
	} partitions[MAXPARTITIONS];
	int32_t		checksum;
	int32_t		_pad;
} __attribute__((__packed__));

#define SGI_PTYPE_VOLHDR	0
#define SGI_PTYPE_RAW		3
#define SGI_PTYPE_VOLUME	6
#define SGI_PTYPE_EFS		7
#define SGI_PTYPE_LVOL		8
#define SGI_PTYPE_RLVOL     	9
#define SGI_PTYPE_XFS		10
#define SGI_PTYPE_XFSLOG	11
#define SGI_PTYPE_XLV		12
#define SGI_PTYPE_XVM		13

#endif /* _MACHINE_DISKLABEL_H_ */
