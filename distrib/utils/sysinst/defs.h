/*	$NetBSD: defs.h,v 1.5 1997/10/15 04:35:25 phil Exp $	*/

/*
 * Copyright 1997 Piermont Information Systems Inc.
 * All rights reserved.
 *
 * Written by Philip A. Nelson for Piermont Information Systems Inc.
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
 *      This product includes software develooped for the NetBSD Project by
 *      Piermont Information Systems Inc.
 * 4. The name of Piermont Information Systems Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PIERMONT INFORMATION SYSTEMS INC. ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PIERMONT INFORMATION SYSTEMS INC. BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* defs.h -- definitions for use in the sysinst program. */

/* System includes needed for this. */
#include <sys/types.h>
#include <sys/disklabel.h>

/* Define for external varible use */ 
#ifdef MAIN
#define EXTERN 
#define INIT(x) = x
#else
#define EXTERN extern
#define INIT(x)
#endif

/* constants */
#define MEG 1048576
#define STRSIZE 255
#define SSTRSIZE 30

/* For run.c: collect() */
#define T_FILE 0
#define T_OUTPUT 1

/* Macros */

/* Round up to the next full cylinder size */
#define NUMSEC(size,sizemult,cylsize) \
	((sizemult == 1) ? (size) : \
	 (((size)*(sizemult)+(cylsize)-1)/(cylsize))*(cylsize))

/* variables */

EXTERN char rel[10] INIT(REL);
EXTERN char rels[10] INIT(REL);

EXTERN int yesno;
EXTERN int layoutkind;
EXTERN int sizemult INIT(1);
EXTERN char *multname; 
EXTERN char *doingwhat;

/* Hardware variables */
EXTERN long ramsize INIT(0);
EXTERN int  rammb   INIT(0);

/* Disk descriptions */
#define MAX_DISKS 15
struct disk_desc {
	char name[SSTRSIZE];
	int  geom[5];
};
EXTERN struct disk_desc disks[MAX_DISKS];

EXTERN struct disk_desc *disk;
EXTERN int sectorsize;

/* Actual name of the disk. */
EXTERN char diskdev[SSTRSIZE];
EXTERN char disknames[STRSIZE];
EXTERN int  numdisks;
EXTERN char *disktype;		/* ST506, SCSI, ... */

/* Used in editing partitions ... BSD disklabel and others */
EXTERN int editpart;

/* Final known sizes for the NetBSD partition, NetBSD disk sizes. */
EXTERN int ptstart, ptsize;
EXTERN int fsdsize, fsptsize;
EXTERN int fsdmb;
EXTERN int minfsdmb;
EXTERN int partstart;
EXTERN int partsize;

/* set by md_get_info() */
EXTERN int dlcyl, dlhead, dlsec, dlsize, dlcylsize;
EXTERN int swapadj INIT(0);

/* Information for the NetBSD disklabel */
EXTERN char *fstype[]
#ifdef MAIN
= {"unused", "swap", "4.2BSD", "msdos"}
#endif
;
enum DTYPE {T_UNUSED, T_SWAP, T_42BSD, T_MSDOS};
enum DINFO {D_SIZE, D_OFFSET, D_FSTYPE, D_BSIZE, D_FSIZE};
enum DLTR {A,B,C,D,E,F,G,H};
EXTERN char partname[] INIT("abcdefgh");
EXTERN int bsdlabel[8][5];
EXTERN char fsmount[8][20] INIT({""});
EXTERN char bsddiskname[80];
EXTERN char *doessf INIT("");

/* scsi_fake communication */
EXTERN int fake_sel;

/* other vars for menu communication */
EXTERN int  nodist;
EXTERN int  got_dist;
EXTERN char dist_dir[STRSIZE] INIT("/usr/INSTALL");
EXTERN int  clean_dist_dir INIT(0);
EXTERN char ftp_host[STRSIZE] INIT("ftp.netbsd.org");
EXTERN char ftp_dir[STRSIZE]  INIT("/pub/NetBSD/NetBSD-" REL "/" MACH);
EXTERN char ftp_user[STRSIZE] INIT("ftp");
EXTERN char ftp_pass[STRSIZE] INIT("");

EXTERN char nfs_host[STRSIZE] INIT("");
EXTERN char nfs_dir[STRSIZE] INIT("");

/* Vars for runing commands ... */
EXTERN char command[STRSIZE];

/* Access to network information */
EXTERN char net_devices [STRSIZE] INIT("");
EXTERN char net_dev[STRSIZE] INIT("");
EXTERN char net_domain[STRSIZE] INIT("");
EXTERN char net_host[STRSIZE] INIT("");
EXTERN char net_ip[STRSIZE] INIT("");
EXTERN char net_mask[STRSIZE] INIT("");
EXTERN char net_namesvr[STRSIZE] INIT("");
EXTERN char net_defroute[STRSIZE] INIT("");

/* Variables for upgrade. */
#define MAXFS 16
EXTERN char fs_dev[MAXFS][STRSIZE];
EXTERN char fs_mount[MAXFS][STRSIZE];
EXTERN int  fs_num;

/* needed prototypes */

/* Machine dependent functions .... */
void	md_get_info __P((void));
void	md_pre_disklabel __P((void));
void	md_post_disklabel __P((void));
void	md_post_newfs __P((void));
void	md_copy_filesystem __P((void));
void	md_make_bsd_partitions __P((void));
int	md_update __P((void));

/* from disks.c */
int	find_disks __P((void));
void	disp_cur_part __P((int,int));
void	disp_cur_fspart __P((int, int));
void	scsi_fake __P((void));
void	make_bsd_partitions __P((void));
void	write_disklabel __P((void));
void	make_filesystems __P((void));
void	make_fstab __P((void));
int	fsck_disks __P((void));

/* from install.c */
void	do_install __P((void));

/* from factor.c */
void	factor __P((long, long *, int, int *));

/* from geom.c */
int get_geom __P((char *, struct disklabel *));

/* from net.c */
int  get_via_ftp __P((void));
int  get_via_nfs __P((void));

/* From run.c */
int	collect __P((int kind, char **buffer, char *name, ...));
int	run_prog __P((char *, ...));

/* from upgrade.c */
void	do_upgrade __P((void));

/* from util.c */
void	get_ramsize __P((void));
void	ask_sizemult __P((void));
int	ask_ynquestion __P((char *quest, char def, ...));
void	extract_dist __P((void));
void	run_makedev __P((void));
