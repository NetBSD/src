/*	$NetBSD: disklabel.h,v 1.1.1.1.32.1 2002/08/01 02:42:50 nathanw Exp $	*/
/*
 * Copyright (c) 1994 Rolf Grossmann
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
 *      This product includes software developed by Rolf Grossmann.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#define	LABELSECTOR	0	/* sector containing label */
#define	LABELOFFSET	0	/* offset of label in sector */
#define LABELSIZE	8192	/* size of label */
#define	MAXPARTITIONS	8	/* number of partitions */
#define	RAW_PART	2	/* raw partition: xx?c */

#define	CPUMAXPARTITIONS	8	/* number of partitions in cpu_disklabel */
#define CD_V1	0x4e655854	/* version #1: "NeXT" */
#define CD_V2	0x646c5632	/* version #2: "dlV2" */
#define CD_V3	0x646c5633	/* version #3: "dlV3" */
#define IS_DISKLABEL(l) ((l)->cd_version == CD_V1 || (l)->cd_version == CD_V2 \
                         || (l)->cd_version == CD_V3)

/* XXX should we really size all this stuff this way ? */
#define CPULBLLEN	24
#define MAXDNMLEN	24
#define MAXTYPLEN       24
#define MAXBFLEN	24
#define MAXHNLEN	32
#define MAXMPTLEN       16
#define MAXFSTLEN       8

#define DEFAULTFRONTPORCH (160 * 2)
#define DEFAULTBOOT0_1 (32 * 2)
#define DEFAULTBOOT0_2 (96 * 2)

struct  cpu_partition {
	int cp_offset;		/* starting sector */
	int cp_size;		/* number of sectors in partition */
	short cp_bsize;		/* block size in bytes */
	short cp_fsize;		/* filesystem basic fragment size */
	char cp_opt;		/* optimization type: 's'pace/'t'ime */
	char cp_pad1;
	short cp_cpg;		/* filesystem cylinders per group */
	short cp_density;	/* bytes per inode density */
	char cp_minfree;	/* minfree (%) */
	char cp_newfs;		/* run newfs during init */
	char cp_mountpt[MAXMPTLEN];/* default/standard mount point */
	char cp_automnt;	/* auto-mount when inserted */
	char cp_type[MAXFSTLEN]; /* file system type name */
	char cp_pad2;
} __attribute__ ((packed));

/* The disklabel the way it is on the disk */
struct nextstep_disklabel {
    int cd_version;		/* label version */
    int cd_label_blkno;         /* block # of this label */
    int cd_size;                /* size of media area (sectors) */
    char cd_label[CPULBLLEN];	/* disk name (label) */
    u_int cd_flags;		/* flags */
#define CD_UNINIT	0x80000000 /* label is uninitialized */
    u_int cd_tag;		/* volume tag */
    char cd_name[MAXDNMLEN];	/* drive (hardware) name */
    char cd_type[MAXTYPLEN];	/* drive type */
    int cd_secsize;		/* # of bytes per sector */
    int cd_ntracks;		/* # of tracks per cylinder */
    int cd_nsectors;		/* # of data sectors per track */
    int cd_ncylinders;          /* # of data cylinders per unit */
    int cd_rpm;                 /* rotational speed */
    short cd_front;		/* # of sectors in "front porch" */
    short cd_back;		/* # of sectors in "back porch" */
    short cd_ngroups;		/* # of alt groups */
    short cd_ag_size;		/* alt group size (sectors) */
    short cd_ag_alts;		/* alternate sectors / alt group */
    short cd_ag_off;		/* sector offset to first alternate */
    int cd_boot_blkno[2];	/* boot program locations */
    char cd_kernel[MAXBFLEN];	/* default kernel name */
    char cd_hostname[MAXHNLEN];	/* host name (usu. where disk was labeled) */
    char cd_rootpartition;	/* root partition letter e.g. 'a' */
    char cd_rwpartition;	/* r/w partition letter e.g. 'b' */
    struct cpu_partition cd_partitions[CPUMAXPARTITIONS];

    union {
	u_short CD_v3_checksum;	/* label version 3 checksum */
#define NBAD	1670		/* sized to make label ~= 8KB */
	int CD_bad[NBAD];   /* block number that is bad */
    } cd_un;
#define cd_v3_checksum  cd_un.CD_v3_checksum
#define cd_bad          cd_un.CD_bad
    u_short cd_checksum;	/* label version 1 or 2 checksum */
} __attribute__ ((packed));

struct cpu_disklabel {
	int od_version;
};

#endif /* _MACHINE_DISKLABEL_H_ */
