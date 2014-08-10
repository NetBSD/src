/*	$NetBSD: defs.h,v 1.3.2.2 2014/08/10 07:00:24 tls Exp $	*/

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
 * 3. The name of Piermont Information Systems Inc. may not be used to endorse
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
#include <sys/queue.h>
#include <sys/types.h>
#include <sys/disklabel.h>
#include <sys/disk.h>

const char *getfslabelname(uint8_t);

static inline void *
deconst(const void *p)
{
	return (char *)0 + ((const char *)p - (const char *)0);
}

#include "msg_defs.h"
#include "menu_defs.h"

#define min(a,b)	((a) < (b) ? (a) : (b))
#define max(a,b)	((a) > (b) ? (a) : (b))

/* constants */
#define MEG (1024 * 1024)
#define STRSIZE 255
#define SSTRSIZE 30

/* For run.c: collect() */
#define T_FILE		0
#define T_OUTPUT	1

/* Some install status/response values */
#define	SET_OK		0		/* Set extracted */
#define	SET_RETRY	1		/* Retry */
#define	SET_SKIP	2		/* Skip this set */
#define	SET_SKIP_GROUP	3		/* Skip this set and rest of group */
#define	SET_ABANDON	4		/* Abandon installation */
#define	SET_CONTINUE	5		/* Continue (copy from floppy loop) */

/* run_prog flags */
#define RUN_DISPLAY	0x0001		/* Display program output */
#define RUN_FATAL	0x0002		/* errors are fatal */
#define RUN_CHROOT	0x0004		/* chroot to target disk */
#define RUN_FULLSCREEN	0x0008		/* fullscreen (use with RUN_DISPLAY) */
#define RUN_SILENT	0x0010		/* Do not show output */
#define RUN_ERROR_OK	0x0040		/* Don't wait for error confirmation */
#define RUN_PROGRESS	0x0080		/* Output is just progess test */
#define RUN_NO_CLEAR	0x0100		/* Leave program output after error */
#define RUN_XFER_DIR	0x0200		/* cd to xfer_dir in child */

/* for bsddisklabel.c */
enum { LY_SETNEW, LY_NEWRAID, LY_NEWCGD, LY_NEWLVM, LY_USEEXIST };

/* Installation sets */
enum {
    SET_NONE,
    SET_KERNEL_FIRST,
    SET_KERNEL_1,	/* Usually GENERIC */
    SET_KERNEL_2,	/* MD kernel... */
    SET_KERNEL_3,	/* MD kernel... */
    SET_KERNEL_4,	/* MD kernel... */
    SET_KERNEL_5,	/* MD kernel... */
    SET_KERNEL_6,	/* MD kernel... */
    SET_KERNEL_7,	/* MD kernel... */
    SET_KERNEL_8,	/* MD kernel... */
    SET_KERNEL_9,	/* MD kernel... */
    SET_KERNEL_LAST,	/* allow 9 kernels */

    /* System sets */
    SET_BASE,		/* base */
    SET_ETC,		/* /etc */
    SET_COMPILER,	/* compiler tools */
    SET_GAMES,		/* text games */
    SET_MAN_PAGES,	/* online manual pages */
    SET_MISC,		/* miscellaneuous */
    SET_MODULES,	/* kernel modules */
    SET_TESTS,		/* tests */
    SET_TEXT_TOOLS,	/* text processing tools */

    /* X11 sets */
    SET_X11_FIRST,
    SET_X11_BASE,	/* X11 base and clients */
    SET_X11_FONTS,	/* X11 fonts */
    SET_X11_SERVERS,	/* X11 servers */
    SET_X11_PROG,	/* X11 programming */
    SET_X11_ETC,	/* X11 config */
    SET_X11_LAST,

    /* Machine dependent sets */
    SET_MD_1,		/* Machine dependent set */
    SET_MD_2,		/* Machine dependent set */
    SET_MD_3,		/* Machine dependent set */
    SET_MD_4,		/* Machine dependent set */
    
    /* Source sets */
    SET_SYSSRC,
    SET_SRC,
    SET_SHARESRC,
    SET_GNUSRC,
    SET_XSRC,

    /* Debug sets */
    SET_DEBUG,
    SET_X11_DEBUG,

    SET_LAST,
    SET_GROUP,		/* Start of submenu */
    SET_GROUP_END,	/* End of submenu */
    SET_PKGSRC,		/* pkgsrc, not counted as regular set */
};

/* Initialisers to select sets */
/* All kernels */
#define SET_KERNEL SET_KERNEL_1, SET_KERNEL_2, SET_KERNEL_3, SET_KERNEL_4, \
		    SET_KERNEL_5, SET_KERNEL_6, SET_KERNEL_7, SET_KERNEL_8
/* Core system sets */
#define SET_CORE SET_MODULES, SET_BASE, SET_ETC
/* All system sets */
#define SET_SYSTEM SET_CORE, SET_COMPILER, SET_GAMES, \
		    SET_MAN_PAGES, SET_MISC, SET_TESTS, SET_TEXT_TOOLS
/* All X11 sets */
#define SET_X11_NOSERVERS SET_X11_BASE, SET_X11_FONTS, SET_X11_PROG, SET_X11_ETC
#define SET_X11 SET_X11_NOSERVERS, SET_X11_SERVERS

/* All machine dependent sets */
#define SET_MD SET_MD_1, SET_MD_2, SET_MD_3, SET_MD_4

/* All source sets */
#define SET_SOURCE SET_SYSSRC, SET_SRC, SET_SHARESRC, SET_GNUSRC, SET_XSRC

/* All debug sets */
#define SET_DEBUGGING SET_DEBUG, SET_X11_DEBUG

/* Set list flags */
#define SFLAG_MINIMAL	1
#define	SFLAG_NOX	2

/* Macros */
#define nelem(x) (sizeof (x) / sizeof *(x))

/* Round up to the next full cylinder size */
#define NUMSEC(size, sizemult, cylsize) \
	((size) == ~0u ? ~0u : (sizemult) == 1 ? (size) : \
	 roundup((size) * (sizemult), (cylsize)))

/* What FS type? */
#define PI_ISBSDFS(p) ((p)->pi_fstype == FS_BSDLFS || \
		       (p)->pi_fstype == FS_BSDFFS)

/* standard cd0 device */
#define CD_NAMES "cd0a"

/* Types */
typedef struct distinfo {
	const char	*name;
	uint		set;
	const char	*desc;
	const char	*marker_file;	/* set assumed installed if exists */
} distinfo;

#define MOUNTLEN 20
typedef struct _partinfo {
	struct partition pi_partition;
#define pi_size		pi_partition.p_size
#define pi_offset	pi_partition.p_offset
#define pi_fsize	pi_partition.p_fsize
#define pi_fstype	pi_partition.p_fstype
#define pi_frag		pi_partition.p_frag
#define pi_cpg		pi_partition.p_cpg
	char	pi_mount[MOUNTLEN];
	uint	pi_isize;		/* bytes per inode (for # inodes) */
	uint	pi_flags;
#define PIF_NEWFS	0x0001		/* need to 'newfs' partition */
#define PIF_FFSv2	0x0002		/* newfs with FFSv2, not FFSv1 */
#define PIF_MOUNT	0x0004		/* need to mount partition */
#define PIF_ASYNC	0x0010		/* mount -o async */
#define PIF_NOATIME	0x0020		/* mount -o noatime */
#define PIF_NODEV	0x0040		/* mount -o nodev */
#define PIF_NODEVMTIME	0x0080		/* mount -o nodevmtime */
#define PIF_NOEXEC	0x0100		/* mount -o noexec */
#define PIF_NOSUID	0x0200		/* mount -o nosuid */
#define PIF__UNUSED	0x0400		/* unused */
#define PIF_LOG		0x0800		/* mount -o log */
#define PIF_MOUNT_OPTS	0x0ff0		/* all above mount flags */
#define PIF_RESET	0x1000		/* internal - restore previous values */
    const char *mnt_opts;
    const char *fsname;
    char mounted[MOUNTLEN];
    int lvmpv; /* should we use partition as LVM PV? */
} partinfo;	/* Single partition from a disklabel */

struct ptn_info {
	int		menu_no;
	struct ptn_size {
		int	ptn_id;
		char	mount[MOUNTLEN];
		daddr_t	dflt_size;
		daddr_t	size;
		int	limit;
		int	changed;
	}		ptn_sizes[MAXPARTITIONS + 1];	/* +1 for delete code */
	menu_ent	ptn_menus[MAXPARTITIONS + 1];	/* +1 for unit chg */
	int		free_parts;
	daddr_t		free_space;
	struct ptn_size	*pool_part;
	char		exit_msg[70];
};

/* variables */

int debug;		/* set by -D option */

char rel[SSTRSIZE];
char machine[SSTRSIZE];

int yesno;
int ignorerror;
int ttysig_ignore;
pid_t ttysig_forward;
int layoutkind;
int sizemult;
const char *multname;
int partman_go; /* run extended partition manager */

/* logging variables */

FILE *logfp;
FILE *script;

/* Information for the NetBSD disklabel */

enum DLTR { PART_A, PART_B, PART_C, PART_D, PART_E, PART_F, PART_G, PART_H,
	    PART_I, PART_J, PART_K, PART_L, PART_M, PART_N, PART_O, PART_P};
#define partition_name(x)	('a' + (x))
daddr_t tmp_ramdisk_size;
#define MAX_DISKS 15

unsigned int root_limit;    /* BIOS (etc) read limit */

enum SHRED_T { SHRED_NONE=0, SHRED_ZEROS, SHRED_RANDOM, SHRED_CRYPTO };

/* All information that is unique for each drive */
SLIST_HEAD(pm_head_t, pm_devs_t) pm_head;

typedef struct pm_devs_t {
    int unsaved; /* Flag indicating to partman that device need saving */
    int found; /* Flag to delete unplugged and unconfigured devices */
    int blocked; /* Device is busy and cannot be changed */
    void *refdev; /* If device is blocked thats is a pointers to a parent dev */
    int isspecial; /* LVM LV or DK device that doesnot accept disklabel */
    char diskdev[SSTRSIZE]; /* Actual name of the disk. */
    char diskdev_descr[STRSIZE];
    int bootable;
#define DISKNAME_SIZE 16
    char bsddiskname[DISKNAME_SIZE];
    partinfo oldlabel[MAXPARTITIONS]; /* What we found on the disk */
    partinfo bsdlabel[MAXPARTITIONS]; /* What we want it to look like */
    int gpt;
    int no_mbr; /* set for raid (etc) */
    int rootpart; /* partition we install into */
    const char *disktype; /* ST506, SCSI, ... */
    const char *doessf;
    /* Actual values for current disk - set by find_disks() or md_get_info() */
    int sectorsize, dlcyl, dlhead, dlsec, dlcylsize, current_cylsize;
    daddr_t dlsize;
    /* Area of disk we can allocate, start and size in disk sectors. */
    daddr_t ptstart, ptsize;
    /* If we have an MBR boot partition, start and size in sectors */
    int bootstart, bootsize;
    struct ptn_info pi;
    SLIST_ENTRY(pm_devs_t) l;
} pm_devs_t;
pm_devs_t *pm; /* Pointer to currend device with which we work */
pm_devs_t *pm_new; /* Pointer for next allocating device in find_disks() */

#define MAX_WEDGES 16 /* num of dk* devices */
typedef struct pm_wedge_t {
    int allocated;
    int todel;
    pm_devs_t *pm;
    int ptn;
} pm_wedge_t;
pm_wedge_t wedges[MAX_WEDGES];

/* Generic structure for partman */
typedef struct {
    int retvalue;
    int dev_num;
    int wedge_num;
    void *dev_ptr;
    int dev_ptr_delta;
    char fullname[SSTRSIZE];
    enum {PM_DISK_T=1, PM_PART_T, PM_WEDGE_T, PM_SPEC_T, PM_RAID_T, PM_CGD_T,
        PM_VND_T, PM_LVM_T, PM_LVMLV_T} type;
} part_entry_t;

/* Relative file name for storing a distribution. */
char xfer_dir[STRSIZE];
int  clean_xfer_dir;

#if !defined(SYSINST_FTP_HOST)
#define SYSINST_FTP_HOST	"ftp.NetBSD.org"
#endif

#if !defined(SYSINST_FTP_DIR)
#define SYSINST_FTP_DIR		"pub/NetBSD/NetBSD-" REL
#endif

#if !defined(SYSINST_PKG_HOST)
#define SYSINST_PKG_HOST	SYSINST_FTP_HOST
#endif

#if !defined(SYSINST_PKG_DIR)
#define SYSINST_PKG_DIR		"pub/pkgsrc/packages/NetBSD"
#endif

#if !defined(SYSINST_PKGSRC_HOST)
#define SYSINST_PKGSRC_HOST	SYSINST_PKG_HOST
#endif

/* Abs. path we extract binary sets from */
char ext_dir_bin[STRSIZE];

/* Abs. path we extract source sets from */
char ext_dir_src[STRSIZE];

/* Abs. path we extract pkgsrc from */
char ext_dir_pkgsrc[STRSIZE];

/* Place we look for binary sets in all fs types */
char set_dir_bin[STRSIZE];

/* Place we look for source sets in all fs types */
char set_dir_src[STRSIZE];

/* Place we look for pkgs in all fs types */
char pkg_dir[STRSIZE];

/* Place we look for pkgsrc in all fs types */
char pkgsrc_dir[STRSIZE];

/* User shell */
const char *ushell;

struct ftpinfo {
    char host[STRSIZE];
    char dir[STRSIZE] ;
    char user[SSTRSIZE];
    char pass[STRSIZE];
    char proxy[STRSIZE];
    const char *xfer_type;		/* "ftp" or "http" */
};

/* use the same struct for sets ftp and to build pkgpath */
struct ftpinfo ftp, pkg, pkgsrc;

int (*fetch_fn)(const char *);
char nfs_host[STRSIZE];
char nfs_dir[STRSIZE];

char cdrom_dev[SSTRSIZE];		/* Typically "cd0a" */
char fd_dev[SSTRSIZE];			/* Typically "/dev/fd0a" */
const char *fd_type;			/* "msdos", "ffs" or maybe "ados" */

char localfs_dev[SSTRSIZE];
char localfs_fs[SSTRSIZE];
char localfs_dir[STRSIZE];

char targetroot_mnt[SSTRSIZE];

int  mnt2_mounted;

char dist_postfix[SSTRSIZE];

/* needed prototypes */
void set_menu_numopts(int, int);
void remove_color_options(void);
void remove_raid_options(void);
void remove_lvm_options(void);
void remove_cgd_options(void);
void remove_gpt_options(void);

/* Machine dependent functions .... */
void	md_init(void);
void	md_prelim_menu(void);
void	md_init_set_status(int); /* SFLAG_foo */

 /* MD functions if user selects install - in order called */
int	md_get_info(void);
int	md_make_bsd_partitions(void);
int	md_check_partitions(void);
int	md_pre_disklabel(void);
int	md_post_disklabel(void);
int	md_pre_mount(void);
int	md_post_newfs(void);
int	md_post_extract(void);
void	md_cleanup_install(void);

 /* MD functions if user selects upgrade - in order called */
int	md_pre_update(void);
int	md_update(void);
/* Also calls md_post_extract() */

/* from main.c */
void	toplevel(void);

/* from disks.c */
const char *get_default_cdrom(void);
int	find_disks(const char *);
struct menudesc;
void	fmt_fspart(struct menudesc *, int, void *);
void	disp_cur_fspart(int, int);
int	write_disklabel(void);
int	make_filesystems(void);
int	make_fstab(void);
int	mount_disks(void);
int	set_swap(const char *, partinfo *);
int	check_swap(const char *, int);
char *bootxx_name(void);
void label_read(void);
int get_dkwedges(struct dkwedge_info **, const char *);
const char *get_gptfs_by_id(int);


/* from disks_lfs.c */
int	fs_is_lfs(void *);

/* from label.c */
const char *get_last_mounted(int, int, partinfo *);
int	savenewlabel(partinfo *, int);
int	incorelabel(const char *, partinfo *);
int	edit_and_check_label(partinfo *, int, int, int);
void	set_bsize(partinfo *, int);
void	set_fsize(partinfo *, int);
void	set_ptype(partinfo *, int, int);
int edit_ptn(menudesc *, void *);
int checkoverlap(partinfo *, int, int, int);

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
extern int network_up;
extern char net_namesvr6[STRSIZE];
int	get_via_ftp(const char *);
int	get_via_nfs(void);
int	config_network(void);
void	mnt_net_config(void);
void	make_url(char *, struct ftpinfo *, const char *);
int	get_pkgsrc(void);

/* From run.c */
int	collect(int, char **, const char *, ...) __printflike(3, 4);
int	run_program(int, const char *, ...) __printflike(2, 3);
void	do_logging(void);
int	do_system(const char *);

/* from upgrade.c */
void	do_upgrade(void);
void	do_reinstall_sets(void);
void	restore_etc(void);

/* from util.c */
int	dir_exists_p(const char *);
int	file_exists_p(const char *);
int	file_mode_match(const char *, unsigned int);
uint	get_ramsize(void);
void	ask_sizemult(int);
void	run_makedev(void);
int	boot_media_still_needed(void);
int	get_via_floppy(void);
int	get_via_cdrom(void);
int	get_via_localfs(void);
int	get_via_localdir(void);
void	show_cur_distsets(void);
void	make_ramdisk_dir(const char *);
void    set_kernel_set(unsigned int);
unsigned int    get_kernel_set(void);
unsigned int    set_X11_selected(void);
int 	get_and_unpack_sets(int, msg, msg, msg);
int	sanity_check(void);
int	set_timezone(void);
void	scripting_fprintf(FILE *, const char *, ...) __printflike(2, 3);
void	scripting_vfprintf(FILE *, const char *, va_list) __printflike(2, 0);
void	add_rc_conf(const char *, ...);
int	del_rc_conf(const char *);
void	add_sysctl_conf(const char *, ...) __printflike(1, 2);
void	enable_rc_conf(void);
void	set_sizemultname_cyl(void);
void	set_sizemultname_meg(void);
int	binary_available(const char *);
int	check_lfs_progs(void);
void	init_set_status(int);
void	customise_sets(void);
void	umount_mnt2(void);
int 	set_is_source(const char *);
const char *set_dir_for_set(const char *);
const char *ext_dir_for_set(const char *);
void	replace(const char *, const char *, ...);
void	get_tz_default(void);
int	extract_file(distinfo *, int);
void	do_coloring (unsigned int, unsigned int);
int set_menu_select(menudesc *, void *);

/* from target.c */
#if defined(DEBUG)  ||	defined(DEBUG_ROOT)
void	backtowin(void);
#endif
const	char *concat_paths(const char *, const char *);
const	char *target_expand(const char *);
void	make_target_dir(const char *);
void	append_to_target_file(const char *, const char *);
void	echo_to_target_file(const char *, const char *);
void	sprintf_to_target_file(const char *, const char *, ...)
    __printflike(2, 3);
void	trunc_target_file(const char *);
const	char *target_prefix(void);
int	target_chdir(const char *);
void	target_chdir_or_die(const char *);
int	target_already_root(void);
FILE	*target_fopen(const char *, const char *);
int	target_collect_file(int, char **, const char *);
int	is_active_rootpart(const char *, int);
int	cp_to_target(const char *, const char *);
void	dup_file_into_target(const char *);
void	mv_within_target_or_die(const char *, const char *);
int	cp_within_target(const char *, const char *, int);
int	target_mount(const char *, const char *, int, const char *);
int target_mount_do(const char *, const char *, const char *);
int	target_test(unsigned int, const char *);
int	target_dir_exists_p(const char *);
int	target_file_exists_p(const char *);
int	target_symlink_exists_p(const char *);
void	unwind_mounts(void);
int	target_mounted(void);

/* from partman.c */
int partman(void);
int pm_partusage(pm_devs_t *, int, int);
int pm_getrefdev(pm_devs_t *);
void pm_setfstype(pm_devs_t *, int, int);
int pm_editpart(int);
void pm_rename(pm_devs_t *);
int pm_shred(pm_devs_t *, int, int);
void pm_umount(pm_devs_t *, int);
int pm_unconfigure(pm_devs_t *);
int pm_cgd_edit(void *, part_entry_t *);
int pm_gpt_convert(pm_devs_t *);
void pm_wedges_fill(pm_devs_t *);
void pm_make_bsd_partitions(pm_devs_t *);

/* from bsddisklabel.c */
int	make_bsd_partitions(void);
int	save_ptn(int, daddr_t, daddr_t, int, const char *);
void	set_ptn_titles(menudesc *, int, void *);
void	set_ptn_menu(struct ptn_info *);
int	set_ptn_size(menudesc *, void *);
void	get_ptn_sizes(daddr_t, daddr_t, int);
int check_partitions(void);

/* from aout2elf.c */
int move_aout_libs(void);

#ifdef WSKBD
void	get_kb_encoding(void);
void	save_kb_encoding(void);
#else
#define	get_kb_encoding()
#define	save_kb_encoding()
#endif

/* from configmenu.c */
void	do_configmenu(void);

/* from checkrc.c */
int	check_rcvar(const char *);
int	check_rcdefault(const char *);
	WINDOW *mainwin;

#endif	/* _DEFS_H_ */
