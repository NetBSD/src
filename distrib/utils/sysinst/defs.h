/*	$NetBSD: defs.h,v 1.44 1999/06/22 02:43:09 cgd Exp $	*/

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
 *      This product includes software developed for the NetBSD Project by
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
#define FSTYPENAMES
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
	((size == -1) ? -1 : (sizemult == 1) ? (size) : \
	 (((size)*(sizemult)+(cylsize)-1)/(cylsize))*(cylsize))

/* Types */
typedef struct distinfo {
	char *name;
	int   getit;
	char *fdlast;
	char *desc;
} distinfo;

typedef struct _partinfo {
	int pi_size;
	int pi_offset;
	int pi_fstype;
	int pi_bsize;
	int pi_fsize;
} partinfo;	/* Single partition from a disklabel */

/* variables */

EXTERN char rel[SSTRSIZE] INIT(REL);
EXTERN char machine[SSTRSIZE] INIT(MACH);

EXTERN int yesno;
EXTERN int ignorerror;
EXTERN int ttysig_ignore;
EXTERN pid_t ttysig_forward;
EXTERN int layoutkind;
EXTERN int sizemult INIT(1);
EXTERN char *multname; 
EXTERN char *doingwhat;

/* loging variables */

EXTERN int logging;
EXTERN int scripting;
EXTERN FILE *log;
EXTERN FILE *script;

/* Hardware variables */
EXTERN long ramsize INIT(0);
EXTERN int  rammb   INIT(0);

/* Disk descriptions */
#define MAX_DISKS 15
struct disk_desc {
	char dd_name[SSTRSIZE];
	struct disk_geom {
		int  dg_cyl;
		int  dg_head;
		int  dg_sec;
		int  dg_secsize;
		int  dg_totsec;
	} dg;
};
#define dd_cyl dg.dg_cyl
#define dd_head dg.dg_head
#define dd_sec dg.dg_sec
#define dd_secsize dg.dg_secsize
#define dd_totsec dg.dg_totsec

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

/* Partition start and size in disk sectors. */
EXTERN int ptstart, ptsize;	

/* File system disk size.  File system partition size. May not include
   full disk size. */
EXTERN int fsdsize, fsptsize;	


EXTERN int fsdmb;
EXTERN int minfsdmb;
EXTERN int partstart;
EXTERN int partsize;

/* set by md_get_info() */
EXTERN int dlcyl, dlhead, dlsec, dlsize, dlcylsize;
/* Information for the NetBSD disklabel */
enum DLTR {A,B,C,D,E,F,G,H};
EXTERN char partname[] INIT("abcdefgh");
EXTERN partinfo bsdlabel[16];
EXTERN char fsmount[16][20] INIT({""});
#define DISKNAME_SIZE 80
EXTERN char bsddiskname[DISKNAME_SIZE];
EXTERN char *doessf INIT("");

/* other vars for menu communication */
EXTERN int  nodist;
EXTERN int  got_dist;
/* Relative file name for storing a distribution. */
EXTERN char dist_dir[STRSIZE] INIT("/usr/INSTALL");  
EXTERN int  clean_dist_dir INIT(0);
/* Absolute path name where the distribution should be extracted from. */

EXTERN char ext_dir[STRSIZE] INIT("");
EXTERN char ftp_host[STRSIZE] INIT("ftp.netbsd.org");
EXTERN char ftp_dir[STRSIZE]  INIT("pub/NetBSD/NetBSD-");
EXTERN char ftp_prefix[STRSIZE] INIT("/binary/sets");
EXTERN char ftp_user[STRSIZE] INIT("ftp");
EXTERN char ftp_pass[STRSIZE] INIT("");

EXTERN char nfs_host[STRSIZE] INIT("");
EXTERN char nfs_dir[STRSIZE] INIT("");

EXTERN char cdrom_dev[SSTRSIZE] INIT("cd0");
EXTERN char cdrom_dir[STRSIZE] INIT("/");

EXTERN char localfs_dev[SSTRSIZE] INIT("sd0");
EXTERN char localfs_fs[SSTRSIZE] INIT("ffs");
EXTERN char localfs_dir[STRSIZE] INIT("");

EXTERN char targetroot_mnt[STRSIZE] INIT ("/mnt");
EXTERN char distfs_mnt[STRSIZE] INIT ("/mnt2");

EXTERN int  mnt2_mounted INIT(0);

EXTERN char dist_postfix[STRSIZE] INIT(".tgz");

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
EXTERN char net_media[STRSIZE] INIT("");

/* Variables for upgrade. */
#if 0
#define MAXFS 16
EXTERN char fs_dev[MAXFS][STRSIZE];
EXTERN char fs_mount[MAXFS][STRSIZE];
#endif

/* needed prototypes */

/* Machine dependent functions .... */
int	md_get_info __P((void));
int	md_pre_disklabel __P((void));
int	md_post_disklabel __P((void));
int	md_post_newfs __P((void));
int	md_copy_filesystem __P((void));
int	md_make_bsd_partitions __P((void));
int	md_update __P((void));
void	md_cleanup_install __P((void));

/* from main.c */
void toplevel __P((void));

/* from disks.c */
int	find_disks __P((void));
void	disp_cur_fspart __P((int, int));
int	write_disklabel __P((void));
int	make_filesystems __P((void));
int	make_fstab __P((void));
int	fsck_disks __P((void));

/* from label.c */

void	emptylabel __P((partinfo *lp));
int	savenewlabel __P((partinfo *lp, int nparts));
int	incorelabel __P((const char *dkname, partinfo *lp));
int	edit_and_check_label __P((partinfo *lp, int nparts,
				  int rawpart, int bsdpart));
int	getpartoff __P((int msg_no, int partstart));
int	getpartsize __P((int msg_no, int partstart, int defpartsize));

/* from install.c */
void	do_install __P((void));

/* from factor.c */
void	factor __P((long, long *, int, int *));

/* from fdisk.c */
void	get_disk_info __P((char *));
void	set_disk_info __P((char *));

/* from geom.c */
int	get_geom __P((char *, struct disklabel *));

/* from net.c */
int	get_via_ftp __P((void));
int	get_via_nfs __P((void));
int	config_network __P((void));
void	mnt_net_config __P((void));

/* From run.c */
int	collect __P((int kind, char **buffer, const char *name, ...));
int	run_prog __P((int, int, char *, char *, ...));
void	do_logging __P((void));
int	do_system __P((const char *));

/* from upgrade.c */
void	do_upgrade __P((void));
void	do_reinstall_sets __P((void));

/* from util.c */
int	askyesno __P((int reverse));
int	dir_exists_p(const char *path);
int	file_exists_p(const char *path);
int	distribution_sets_exist_p __P((const char *path));
void	get_ramsize __P((void));
void	ask_sizemult __P((void));
void	reask_sizemult __P((void));
void	run_makedev __P((void));
int	get_via_floppy __P((void));
int	get_via_cdrom __P((void));
int	get_via_localfs __P((void));
int	get_via_localdir __P((void));
void	cd_dist_dir __P((char *));
void	toggle_getit __P((int));
void	show_cur_distsets __P((void));
void	make_ramdisk_dir __P((const char *path));
void	ask_verbose_dist __P((void));
void 	get_and_unpack_sets(int success_msg, int failure_msg);
int	sanity_check __P((void));

/* from target.c */
int	must_mount_root __P((void));
const	char * target_expand __P((const char *pathname));
void	make_target_dir __P((const char *path));
void	append_to_target_file __P((const char *path, const char *string));
void	echo_to_target_file __P(( const char *path, const char *string));
void	sprintf_to_target_file __P(( const char *path, const char *fmt, ...));
void	trunc_target_file __P((const char *path));
const	char* target_prefix __P((void));
int	target_chdir __P(( const char *path));
void	target_chdir_or_die __P((const char *dir));
int	target_already_root __P((void));
FILE*	target_fopen __P((const char *filename, const char *type));
int	target_collect_file __P((int kind, char **buffer, char *name));
int	is_active_rootpart __P((const char *partname));
int	cp_to_target __P((const char *src, const char *tgt_dst));
void	dup_file_into_target __P((const char *filename));
void	mv_within_target_or_die __P((const char *from, const char *to));
int	cp_within_target __P((const char *frompath, const char *topath));
int	target_mount __P((const char *fstype, const char *from, const char* on));
int	target_test __P((const char*, const char*));
int	target_dir_exists_p __P((const char *path));
int	target_file_exists_p __P((const char *path));
void	unwind_mounts __P((void));

