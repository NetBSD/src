/*	$NetBSD: defs.h,v 1.79 2003/05/29 17:54:22 dsl Exp $	*/

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

#define	min(a,b)	(a < b ? a : b)
#define	max(a,b)	(a > b ? a : b)

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

/* run_prog flags */
#define RUN_DISPLAY	0x0001		/* run in subwindow */
#define RUN_FATAL	0x0002		/* errors are fatal */
#define RUN_CHROOT	0x0004		/* chroot to target disk */
#define RUN_FULLSCREEN	0x0008		/* fullscreen (use with RUN_DISPLAY) */
#define RUN_SYSTEM	0x0010		/* just use system(3) */

/* Macros */

/* Round up to the next full cylinder size */
#define NUMSEC(size,sizemult,cylsize) \
	(((int)(size) == -1) ? -1 : (int)((sizemult == 1) ? (size) : \
	 (((size)*(sizemult)+(cylsize)-1)/(cylsize))*(cylsize)))

/* What FS type? */
#define PI_ISBSDFS(p) ((p)->pi_fstype == FS_BSDLFS || \
		       (p)->pi_fstype == FS_BSDFFS)

/* Types */
typedef struct distinfo {
	char *name;
	int   getit;
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

EXTERN char m_continue[STRSIZE] INIT("");

EXTERN char rel[SSTRSIZE] INIT(REL);
EXTERN char machine[SSTRSIZE] INIT(MACH);

EXTERN int yesno;
EXTERN int ignorerror;
EXTERN int ttysig_ignore;
EXTERN pid_t ttysig_forward;
EXTERN int layoutkind;
EXTERN int sizemult INIT(1);
EXTERN const char *multname; 
EXTERN const char *doingwhat;
EXTERN const char *shellpath;

/* loging variables */

EXTERN int logging;
EXTERN int scripting;
EXTERN FILE *logfp;
EXTERN FILE *script;

/* Hardware variables */
EXTERN unsigned long ramsize INIT(0);
EXTERN unsigned int  rammb   INIT(0);

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
EXTERN char diskdev[SSTRSIZE] INIT("");
EXTERN char disknames[STRSIZE];
EXTERN int  numdisks;
EXTERN char *disktype;		/* ST506, SCSI, ... */
EXTERN char swapdev[SSTRSIZE] INIT("");

/* Used in editing partitions ... BSD disklabel and others */
EXTERN int editpart;

/* Area of disk we can allocate, start and size in disk sectors. */
EXTERN int ptstart, ptsize;	

EXTERN int minfsdmb;

/* Actual values for current disk - set by md_get_info() */
EXTERN int dlcyl, dlhead, dlsec, dlsize, dlcylsize;
EXTERN int current_cylsize;

/* Information for the NetBSD disklabel */
enum DLTR {A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z};
#define partition_name(x)	('a' + (x))
EXTERN partinfo bsdlabel[16];
EXTERN char fsmount[16][20] INIT({""});
#define PM_INIT {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
EXTERN int preservemount[16] INIT(PM_INIT);
#undef PM_INIT
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

EXTERN char targetroot_mnt[STRSIZE] INIT ("/mnt");
EXTERN char distfs_mnt[STRSIZE] INIT ("/mnt2");

EXTERN int  mnt2_mounted INIT(0);

EXTERN char dist_postfix[STRSIZE] INIT(".tgz");

/* Vars for runing commands ... */
EXTERN char command[STRSIZE];

/* Access to network information */
EXTERN char net_devices[STRSIZE] INIT("");
EXTERN char net_dev[STRSIZE] INIT("");
EXTERN char net_domain[STRSIZE] INIT("");
EXTERN char net_host[STRSIZE] INIT("");
EXTERN char net_ip[STRSIZE] INIT("");
EXTERN char net_mask[STRSIZE] INIT("");
EXTERN char net_namesvr[STRSIZE] INIT("");
EXTERN char net_defroute[STRSIZE] INIT("");
EXTERN char net_media[STRSIZE] INIT("");
EXTERN int net_dhcpconf INIT(0);
#define DHCPCONF_IPADDR		0x01
#define DHCPCONF_NAMESVR	0x02
#define DHCPCONF_HOST		0x04
#define DHCPCONF_DOMAIN		0x08
#ifdef INET6
EXTERN char net_ip6[STRSIZE] INIT("");
EXTERN char net_namesvr6[STRSIZE] INIT("");
EXTERN int net_ip6conf INIT(0);
#define IP6CONF_AUTOHOST	0x01
#endif

/* Variables for upgrade. */
#if 0
#define MAXFS 16
EXTERN char fs_dev[MAXFS][STRSIZE];
EXTERN char fs_mount[MAXFS][STRSIZE];
#endif

/* needed prototypes */

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
int	find_disks(void);
void	disp_cur_fspart(int, int);
int	write_disklabel(void);
int	make_filesystems(void);
int	make_fstab(void);
int	fsck_disks(void);
int	set_swap(const char *, partinfo *, int);

/* from disks_lfs.c */
int	fs_is_lfs(void *);

/* from label.c */

void	emptylabel(partinfo *);
int	savenewlabel(partinfo *, int);
int	incorelabel(const char *, partinfo *);
int	edit_and_check_label(partinfo *, int, int, int);
int	getpartoff(msg, int);
int	getpartsize(msg, int, int);

/* from install.c */
void	do_install(void);

/* from factor.c */
void	factor(long, long *, int, int *);

/* from fdisk.c */
void	get_disk_info(char *);
void	set_disk_info(char *);

/* from geom.c */
int	get_geom(char *, struct disklabel *);
int	get_real_geom(char *, struct disklabel *);

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
void	toggle_getit(int);
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
void	show_cur_filesystems(void);
extern int layout_swap, layout_usr, layout_tmp, layout_var, layout_home;
int	make_bsd_partitions(void);

/* from aout2elf.c */
int move_aout_libs(void);
#endif	/* _DEFS_H_ */
