/*	$NetBSD: defs.h,v 1.97 2003/07/18 09:44:20 dsl Exp $	*/

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

#ifndef _DEFS_H_
#define _DEFS_H_

/* defs.h -- definitions for use in the sysinst program. */

/* System includes needed for this. */
#include <sys/types.h>
#include <sys/disklabel.h>
extern const char * const fstypenames[];

#include "msg_defs.h"

#define	min(a,b)	((a) < (b) ? (a) : (b))
#define	max(a,b)	((a) > (b) ? (a) : (b))

/* Define for external varible use */ 
#ifdef MAIN
#define EXTERN 
#define INIT(x) = x
#else
#define EXTERN extern
#define INIT(x)
#endif

/* constants */
#define MEG (1024 * 1024)
#define STRSIZE 255
#define SSTRSIZE 30

/* For run.c: collect() */
#define T_FILE 0
#define T_OUTPUT 1

/* run_prog flags */
#define RUN_DISPLAY	0x0001		/* run in subwindow */
#define RUN_FATAL	0x0002		/* errors are fatal */
#define RUN_CHROOT	0x0004		/* chroot to target disk */
#define RUN_FULLSCREEN	0x0008		/* fullscreen (use with RUN_DISPLAY) */
#define RUN_SYSTEM	0x0010		/* just use system(3) */

/* Installation sets */
#define SET_KERNEL	0x000000ffu	/* allow 8 kernels */
#define SET_KERNEL_1	0x00000001u	/* Usually GENERIC */
#define SET_KERNEL_2	0x00000002u	/* MD kernel... */
#define SET_KERNEL_3	0x00000004u	/* MD kernel... */
#define SET_KERNEL_4	0x00000008u	/* MD kernel... */
#define SET_KERNEL_5	0x00000010u	/* MD kernel... */
#define SET_KERNEL_6	0x00000020u	/* MD kernel... */
#define SET_KERNEL_7	0x00000040u	/* MD kernel... */
#define SET_KERNEL_8	0x00000080u	/* MD kernel... */

#define SET_SYSTEM	0x000fff00u	/* all system sets */
#define SET_BASE	0x00000100u	/* base */
#define SET_ETC		0x00000200u	/* /etc */
#define SET_COMPILER	0x00000400u	/* compiler tools */
#define SET_GAMES	0x00000800u	/* text games */
#define SET_MAN_PAGES	0x00001000u	/* online manual pages */
#define SET_MISC	0x00002000u	/* miscellaneuous */
#define SET_TEXT_TOOLS	0x00004000u	/* text processing tools */

#define SET_X11		0x0ff00000u	/* All X11 sets */
#define SET_X11_BASE	0x00100000u	/* X11 base and clients */
#define SET_X11_FONTS	0x00200000u	/* X11 fonts */
#define SET_X11_SERVERS	0x00400000u	/* X11 servers */
#define SET_X_CONTRIB	0x00800000u	/* X contrib clients */
#define SET_X11_PROG	0x01000000u	/* X11 programming */
#define SET_X11_MISC	0x02000000u	/* X11 miscelllaneous */

#define SET_MD		0xf0000000u	/* All machine dependant sets */
#define	SET_MD_1	0x10000000u	/* Machine dependant set */
#define	SET_MD_2	0x20000000u	/* Machine dependant set */
#define	SET_MD_3	0x40000000u	/* Machine dependant set */
#define	SET_MD_4	0x80000000u	/* Machine dependant set */

/* Macros */
#define nelem(x) (sizeof (x) / sizeof *(x))

/* Round up to the next full cylinder size */
#define	ROUNDDOWN(n,d) (((n)/(d)) * (d))
#define	DIVUP(n,d) (((n) + (d) - 1) / (d))
#define	ROUNDUP(n,d) (DIVUP((n), (d)) * (d))
#define NUMSEC(size, sizemult, cylsize) \
	((size) == -1 ? -1 : (sizemult) == 1 ? (size) : \
	 ROUNDUP((size) * (sizemult), (cylsize)))

/* What FS type? */
#define PI_ISBSDFS(p) ((p)->pi_fstype == FS_BSDLFS || \
		       (p)->pi_fstype == FS_BSDFFS)

/* Types */
typedef struct distinfo {
	char		*name;
	int		set;
	const char	*desc;
} distinfo;

typedef struct _partinfo {
	struct partition pi_partition;
#define pi_size		pi_partition.p_size
#define pi_offset	pi_partition.p_offset
#define pi_fsize	pi_partition.p_fsize
#define pi_fstype	pi_partition.p_fstype
#define pi_frag		pi_partition.p_frag
#define pi_cpg		pi_partition.p_cpg
	char	pi_mount[20];
	uint	pi_flags;
#define PIF_NEWFS	0x001		/* need to 'newfs' partition */
#define PIF_MOUNT	0x002		/* need to mount partition */
#define PIF_RESET	0x100		/* internal - restore previous values */
} partinfo;	/* Single partition from a disklabel */


/* variables */

EXTERN int debug;		/* set by -D option */

EXTERN char rel[SSTRSIZE] INIT(REL);
EXTERN char machine[SSTRSIZE] INIT(MACH);

EXTERN int yesno;
EXTERN int ignorerror;
EXTERN int ttysig_ignore;
EXTERN pid_t ttysig_forward;
EXTERN int layoutkind;
EXTERN int sizemult INIT(1);
EXTERN const char *multname; 
EXTERN const char *shellpath;

/* loging variables */

EXTERN int logging;
EXTERN int scripting;
EXTERN FILE *logfp;
EXTERN FILE *script;

/* Hardware variables */
EXTERN unsigned long ramsize INIT(0);
EXTERN unsigned int  rammb   INIT(0);

/* Actual name of the disk. */
EXTERN char diskdev[SSTRSIZE] INIT("");
EXTERN int rootpart;				/* partition we install into */
EXTERN char *disktype INIT("unknown");		/* ST506, SCSI, ... */

/* Area of disk we can allocate, start and size in disk sectors. */
EXTERN int ptstart, ptsize;	

/* Actual values for current disk - set by find_disks() or md_get_info() */
EXTERN int sectorsize;
EXTERN int dlcyl, dlhead, dlsec, dlsize, dlcylsize;
EXTERN int current_cylsize;
EXTERN int root_limit;

/* Information for the NetBSD disklabel */
enum DLTR {A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z};
#define partition_name(x)	('a' + (x))
EXTERN partinfo oldlabel[MAXPARTITIONS];	/* What we found on the disk */
EXTERN partinfo bsdlabel[MAXPARTITIONS];	/* What we want it to look like */
EXTERN int tmp_mfs_size INIT(0);

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

#if !defined(SYSINST_FTP_HOST)
#define	SYSINST_FTP_HOST	"ftp.netbsd.org"
#endif

#if !defined(SYSINST_FTP_DIR)
#define	SYSINST_FTP_DIR		"pub/NetBSD/NetBSD-" REL "/" MACH
#endif

#if !defined(SYSINST_CDROM_DIR)
#define	SYSINST_CDROM_DIR	"/" MACH
#endif

EXTERN char ext_dir[STRSIZE] INIT("");
EXTERN char ftp_host[STRSIZE] INIT(SYSINST_FTP_HOST);
EXTERN char ftp_dir[STRSIZE]  INIT(SYSINST_FTP_DIR);
EXTERN char ftp_prefix[STRSIZE] INIT("/binary/sets");
EXTERN char ftp_user[STRSIZE] INIT("ftp");
EXTERN char ftp_pass[STRSIZE] INIT("");
EXTERN char ftp_proxy[STRSIZE] INIT("");

EXTERN char nfs_host[STRSIZE] INIT("");
EXTERN char nfs_dir[STRSIZE] INIT("");

EXTERN char cdrom_dev[SSTRSIZE] INIT("cd0");
EXTERN char cdrom_dir[STRSIZE] INIT(SYSINST_CDROM_DIR);

EXTERN char localfs_dev[SSTRSIZE] INIT("sd0");
EXTERN char localfs_fs[SSTRSIZE] INIT("ffs");
EXTERN char localfs_dir[STRSIZE] INIT("");

EXTERN char targetroot_mnt[STRSIZE] INIT ("/targetroot");
EXTERN char distfs_mnt[STRSIZE] INIT ("/mnt2");

EXTERN int  mnt2_mounted INIT(0);

EXTERN char dist_postfix[STRSIZE] INIT(".tgz");

/* selescted sets */
extern distinfo dist_list[];
extern unsigned int sets_valid;
extern unsigned int sets_selected;
extern unsigned int sets_installed;

/* Variables for upgrade. */
#if 0
#define MAXFS 16
EXTERN char fs_dev[MAXFS][STRSIZE];
EXTERN char fs_mount[MAXFS][STRSIZE];
#endif

/* needed prototypes */
void set_menu_numopts(int, int);

/* Machine dependent functions .... */
int	md_check_partitions(void);
void	md_cleanup_install(void);
int	md_copy_filesystem(void);
int	md_get_info(void);
int	md_make_bsd_partitions(void);
int	md_post_disklabel(void);
int	md_post_newfs(void);
int	md_pre_disklabel(void);
int	md_pre_update(void);
int	md_update(void);
void	md_init(void);
void	md_set_sizemultname(void);
void	md_set_no_x(void);

/* from main.c */
void	toplevel(void);

/* from disks.c */
int	find_disks(const char *);
struct menudesc;
void	fmt_fspart(struct menudesc *, int, void *);
void	disp_cur_fspart(int, int);
int	write_disklabel(void);
int	make_filesystems(void);
int	make_fstab(void);
int	fsck_disks(void);
int	set_swap(const char *, partinfo *);
int	check_swap(const char *, int);

/* from disks_lfs.c */
int	fs_is_lfs(void *);

/* from label.c */

const char *get_last_mounted(int, int);
void	emptylabel(partinfo *);
int	savenewlabel(partinfo *, int);
int	incorelabel(const char *, partinfo *);
int	edit_and_check_label(partinfo *, int, int, int);
int	getpartoff(int);
int	getpartsize(int, int);
int	set_bsize(partinfo *, int);
int	set_fsize(partinfo *, int);

/* from install.c */
void	do_install(void);

/* from factor.c */
void	factor(long, long *, int, int *);

/* from fdisk.c */
void	get_disk_info(char *);
void	set_disk_info(char *);

/* from geom.c */
int	get_geom(const char *, struct disklabel *);
int	get_real_geom(const char *, struct disklabel *);

/* from net.c */
int	get_via_ftp(void);
int	get_via_nfs(void);
int	config_network(void);
void	mnt_net_config(void);

/* From run.c */
int	collect(int, char **, const char *, ...);
int	run_prog(int, msg, const char *, ...);
void	do_logging(void);
int	do_system(const char *);

/* from upgrade.c */
void	do_upgrade(void);
void	do_reinstall_sets(void);
void	restore_etc(void);

/* from util.c */
int	askyesno(int);
int	dir_exists_p(const char *);
int	file_exists_p(const char *);
int	file_mode_match(const char *, unsigned int);
int	distribution_sets_exist_p(const char *);
void	get_ramsize(void);
void	ask_sizemult(int);
void	reask_sizemult(int);
void	run_makedev(void);
int	get_via_floppy(void);
int	get_via_cdrom(void);
int	get_via_localfs(void);
int	get_via_localdir(void);
void	cd_dist_dir(char *);
void	show_cur_distsets(void);
void	make_ramdisk_dir(const char *);
void	ask_verbose_dist(void);
int 	get_and_unpack_sets(msg, msg);
int	sanity_check(void);
int	set_timezone(void);
int	set_crypt_type(void);
int	set_root_password(void);
int	set_root_shell(void);
void	scripting_fprintf(FILE *, const char *, ...);
void	scripting_vfprintf(FILE *, const char *, va_list);
void	add_rc_conf(const char *, ...);
int	check_partitions(void);
void	set_sizemultname_cyl(void);
void	set_sizemultname_meg(void);
int	check_lfs_progs(void);
void	customise_sets(void);

/* from target.c */
int	must_mount_root(void);
const	char *concat_paths(const char *, const char *);
char	*target_realpath(const char *, char *);
const	char *target_expand(const char *);
void	make_target_dir(const char *);
void	append_to_target_file(const char *, const char *);
void	echo_to_target_file(const char *, const char *);
void	sprintf_to_target_file(const char *, const char *, ...);
void	trunc_target_file(const char *);
const	char *target_prefix(void);
int	target_chdir(const char *);
void	target_chdir_or_die(const char *);
int	target_already_root(void);
FILE	*target_fopen(const char *, const char *);
int	target_collect_file(int, char **, char *);
int	is_active_rootpart(const char *);
int	cp_to_target(const char *, const char *);
void	dup_file_into_target(const char *);
void	mv_within_target_or_die(const char *, const char *);
int	cp_within_target(const char *, const char *);
int	target_mount(const char *, const char *, const char *);
int	target_test(unsigned int, const char *);
int	target_dir_exists_p(const char *);
int	target_file_exists_p(const char *);
int	target_symlink_exists_p(const char *);
void	unwind_mounts(void);

/* from bsddisklabel.c */
int	make_bsd_partitions(void);

/* from aout2elf.c */
int move_aout_libs(void);
#endif	/* _DEFS_H_ */
