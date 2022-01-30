/*	$NetBSD: defs.h,v 1.79 2022/01/30 11:58:29 martin Exp $	*/

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
#include <sys/disk.h>
#include <limits.h>
#include <uuid.h>

const char *getfslabelname(uint, uint);

#include "msg_defs.h"
#include "menu_defs.h"
#include "partitions.h"

#define min(a,b)	((a) < (b) ? (a) : (b))
#define max(a,b)	((a) > (b) ? (a) : (b))

/* constants */
#define MEG (1024UL * 1024UL)
#define GIG (1024UL * MEG)
#define STRSIZE		255
#define	MENUSTRSIZE	80
#define SSTRSIZE	30
#define	DISKNAMESIZE	24	/* max(strlen("/dev/rsd22c")) */

/* these are used for different alignment defaults */
#define	HUGE_DISK_SIZE	(daddr_t)(128 * (GIG / 512))
#define	TINY_DISK_SIZE	(daddr_t)(1800 * (MEG / 512))

/*
 * if a system does not have more ram (in MB) than this, swap will be enabled
 * very early (as soon as the swap partition has been created)
 */
#ifdef	EXTRACT_NEEDS_BIG_RAM	/* we use an expensive decompressor */
#define	TINY_RAM_SIZE		256
#else
#define	TINY_RAM_SIZE		32
#endif

/*
 * if a system has less ram (in MB) than this, we will not create a
 * tmpfs /tmp by default (to workaround PR misc/54886)
 */
#define	SMALL_RAM_SIZE		384

/* helper macros to create unique internal error messages */
#define STR_NO(STR)	#STR
#define	TO_STR(NO)	STR_NO(NO)
#define	INTERNAL_ERROR __FILE__ ":" TO_STR(__LINE__) ": internal error"

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
enum layout_type {
	LY_KEEPEXISTING,	/* keep existing partitions */
	LY_OTHERSCHEME,		/* delete all, select new partitioning scheme */
	LY_SETSIZES,		/* edit sizes */
	LY_USEDEFAULT,		/* use default sizes */
	LY_USEFULL,		/* use full disk for NetBSD */
	LY_ERROR		/* used for "abort" in menu */
};

enum setup_type { SY_NEWRAID, SY_NEWCGD, SY_NEWLVM };

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
    SET_DTB,		/* devicetree hardware descriptions */
    SET_GAMES,		/* text games */
    SET_GPUFW,		/* GPU firmware files */
    SET_MAN_PAGES,	/* online manual pages */
    SET_MISC,		/* miscellaneuous */
    SET_MODULES,	/* kernel modules */
    SET_RESCUE,		/* /rescue recovery tools */
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
#ifdef HAVE_MODULES
#define	WITH_MODULES	SET_MODULES,
#else
#define	WITH_MODULES
#endif
/* Core system sets */
#ifdef HAVE_DTB
#define	WITH_DTB	SET_DTB,
#else
#define	WITH_DTB
#endif
#define SET_CORE WITH_MODULES SET_BASE, WITH_DTB SET_GPUFW, SET_ETC
/* All system sets */
#define SET_SYSTEM SET_CORE, SET_COMPILER, SET_GAMES, \
		    SET_MAN_PAGES, SET_MISC, SET_RESCUE, \
		    SET_TESTS, SET_TEXT_TOOLS
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

/* Round up to the next full cylinder size */
#define NUMSEC(size, sizemult, cylsize) \
	((sizemult) == 1 ? (size) : \
	 roundup((size) * (sizemult), (cylsize)))

/* What FS type? */
#define PI_ISBSDFS(PI) (PI_FSTYPE(PI) == FS_BSDLFS || \
		        PI_FSTYPE(PI) == FS_BSDFFS)

/*
 * We do not offer CDs or floppies as installation target usually.
 * Architectures might want to undefine if they want to allow
 * these devices or redefine if they have unusual CD device names.
 * Do not define to empty or an empty string, undefine instead.
 */
#define CD_NAMES "cd*"
#define FLOPPY_NAMES "fd*"

/* Types */

/* pass a void* argument into a menu and also provide a int return value */
typedef struct arg_rv {
	void *arg;
	int rv;
} arg_rv;

/*
 * A minimal argument for menus using string replacements
 */
typedef struct arg_replace {
	const char **argv;
	size_t argc;
} arg_replace;

/*
 * pass a parameter array (for string replacements) into a menu and provide
 * an integer return value
 */
typedef struct arg_rep_int {
	arg_replace args;
	int rv;
} arg_rep_int;

typedef struct distinfo {
	const char	*name;
	uint		set;
	bool		force_tgz;	/* this set is always in .tgz format */
	const char	*desc;
	const char	*marker_file;	/* set assumed installed if exists */
} distinfo;

#define MOUNTLEN 20


/*
 * A description of a future partition and its usage.
 * A list of this is the output of the first stage partition
 * editor, before it gets transformed into a concrete partition
 * layout according to the partitioning scheme backend.
 */
struct part_usage_info {
	daddr_t	size;			/* thumb guestimate of size,
					 * [sec if positive, %-of-ram
					 * if TMPFS and negative]
					 */
	daddr_t def_size;		/* default size */
	daddr_t limit;			/* max size */
	char 	mount[MOUNTLEN];	/* where will we mount this? */
	enum part_type type;		/* PT_root/PT_swap/PT_EFI_SYSTEM */

#define	PUIFLAG_EXTEND		1	/* extend this part if free space
					 * is available */
#define	PUIFLAG_ADD_OUTER	2	/* Add this partition to the outer
					 * partitions (if available) */
#define	PUIFLG_IS_OUTER		4	/* this is an existing outer one */
#define	PUIFLG_ADD_INNER	8	/* add outer also to inner */
#define	PUIFLG_JUST_MOUNTPOINT	16	/* tmpfs of mfs mountpoints */
#define	PUIFLG_CLONE_PARTS	32	/* clone external partitions */
	uint flags;
	struct disk_partitions *parts;	/* Where does this partition live?
					 * We currently only support
					 * a single disk, but we plan to
					 * extend that.
					 * Use pm->parts to access
					 * the partitions. */
	part_id cur_part_id;		/* this may change, but we try to
					 * fix it up after all changes */
	daddr_t cur_start;		/* may change during editing, just
					 * used as a unique identifier */
	uint32_t cur_flags;		/* PTI_* flags from disk_part_info */

#define PUIMNT_ASYNC		0x0001	/* mount -o async */
#define PUIMNT_NOATIME		0x0002	/* mount -o noatime */
#define PUIMNT_NODEV		0x0004	/* mount -o nodev */
#define PUIMNT_NODEVMTIME	0x0008	/* mount -o nodevmtime */
#define PUIMNT_NOEXEC		0x0010	/* mount -o noexec */
#define PUIMNT_NOSUID		0x0020	/* mount -o nosuid */
#define PUIMNT_LOG		0x0040	/* mount -o log */
#define PUIMNT_NOAUTO		0x0080	/* "noauto" fstab flag */
	unsigned int mountflags;	/* flags for fstab */
#define PUIINST_NEWFS	0x0001		/* need to 'newfs' partition */
#define PUIINST_MOUNT	0x0002		/* need to mount partition */
#define	PUIINST_BOOT	0x0004		/* this is a boot partition */
	unsigned int instflags;		/* installer handling flags */
	uint fs_type, fs_version;	/* e.g. FS_LFS, or FS_BSDFS,
					 * version = 2 for FFSv2 */
	uint fs_opt1, fs_opt2, fs_opt3;	/* FS specific, FFS: block/frag */
#ifndef	NO_CLONES
	/*
	 * Only != NULL when PUIFLG_CLONE_PARTS is set, describes the
	 * source partitions to clone here.
	 */
	struct selected_partitions *clone_src;
	/*
	 * If clone_src != NULL, this record corresponds to a single
	 * selected source partition, if clone_ndx is a valid index in clone_src
	 * (>= 0 && <= clone_src->num_sel, or all of them if clone_ndx = ~0U.
	 */
	size_t clone_ndx;
#endif
};

/*
 * A list of partition suggestions, bundled for editing
 */
struct partition_usage_set {
	struct disk_partitions *parts;	/* main partition table */
	size_t num;			/* number of infos */
	struct part_usage_info *infos;	/* 0 .. num-1 */
	struct disk_partitions **write_back;
					/* partition tables from which we
					 * did delete some partitions and
					 * that need updating, even if
					 * no active partition remains. */
	size_t num_write_back;		/* number of write_back */
	daddr_t cur_free_space;		/* estimate of free sectors */
	daddr_t reserved_space;		/* space we are not allowed to use */
	menu_ent *menu_opts;		/* 0 .. num+N */
	int menu;			/* the menu to edit this */
	bool ok;			/* ok to continue (all fit) */
};

/*
 * A structure we pass around in menus that edit a single partition out
 * of a partition_usage_set.
 */
struct single_part_fs_edit {
 	struct partition_usage_set *pset;
	size_t index, first_custom_attr, offset, mode;
	part_id id;
	struct disk_part_info info;	/* current partition data */
	struct part_usage_info *wanted;	/* points at our edit data */

	/*
	 * "Backup" of old data, so we can restore previous values
	 * ("undo").
	 */
	struct part_usage_info old_usage;
	struct disk_part_info old_info;

	/* menu return value */
	int rv;
};

/*
 * Description of a full target installation, all partitions and
 * devices (may be accross several struct pm_devs / disks).
 */
struct install_partition_desc {
	size_t num;				/* how many entries in infos */
	struct part_usage_info *infos;		/* individual partitions */
	struct disk_partitions **write_back;	/* partition tables from
						 * which we did delete some
						 * partitions and that need
						 * updating, even if no
						 * active partition remains. */
	size_t num_write_back;			/* number of write_back */
	bool cur_system;			/* target is the life system */
};

/* variables */

extern int debug;		/* set by -D option */

extern char machine[SSTRSIZE];

extern int ignorerror;
extern int ttysig_ignore;
extern pid_t ttysig_forward;
extern uint sizemult;
extern const char *multname;
extern const char *err_outofmem;
extern int partman_go; /* run extended partition manager */

/* logging variables */

extern FILE *logfp;
extern FILE *script;

#define MAX_DISKS 15

extern daddr_t root_limit;    /* BIOS (etc) read limit */

enum SHRED_T { SHRED_NONE=0, SHRED_ZEROS, SHRED_RANDOM };

/* All information that is unique for each drive */
extern SLIST_HEAD(pm_head_t, pm_devs) pm_head;

struct pm_devs {
	/*
	 * If device is blocked (e.g. part of a raid)
	 * this is a pointers to the  parent dev
	 */
	void *refdev;

	char diskdev[SSTRSIZE];		/* Actual name of the disk. */
	char diskdev_descr[STRSIZE];	/* e.g. IDENTIFY result */

	/*
	 * What the disk layout should look like.
	 */
	struct disk_partitions *parts;

	/*
	 * The device does not take a MBR, even if we usually use
	 * MBR master / disklabel secondary partitioning.
	 * Used e.g. for raid* pseudo-disks.
	 */
	bool no_mbr;	/* userd for raid (etc) */

	/*
	 * This device can not be partitioned (in any way).
	 * Used for wedges (dk*) or LVM devices.
	 */
	bool no_part;

	/*
	 * This is a pseudo-device representing the currently running
	 * system (i.e. all mounted file systems).
	 */
	bool cur_system;

	/* Actual values for current disk geometry - set by find_disks() or
	 *  md_get_info()
	 */
	uint sectorsize, dlcyl, dlhead, dlsec, dlcylsize, current_cylsize;
	/*
	 * Total size of the disk - in 'sectorsize' units (!)
	 */
	daddr_t dlsize;	/* total number of disk sectors */

	/* Area of disk we can allocate, start and size in sectors. */
	daddr_t ptstart, ptsize;

	/* For some bootblocks we need to know the CHS addressable limit */
	daddr_t max_chs;	/* bcyl * bhead * bsec */

	/* If we have an MBR boot partition, start and size in sectors */
	daddr_t bootstart, bootsize;

	/*
	 * In extended partitioning: all partitions in parts (number of
	 * entries is parts->num_part) may actually be mounted (temporarily)
	 * somewhere, e.g. to access a vnd device on them. This list has
	 * a pointer to the current mount point (strdup()'d) if mounted,
	 * or NULL if not.
	 */
	char **mounted;

	bool unsaved;	/* Flag indicating to partman that device need saving */
	bool found;	/* Flag to delete unplugged and unconfigured devices */
	int blocked;	/* Device is busy and cannot be changed */

	SLIST_ENTRY(pm_devs) l;
};
extern struct pm_devs *pm; /* Pointer to current device with which we work */
extern struct pm_devs *pm_new; /* Pointer for next allocating device in find_disks() */

/* Generic structure for partman */
struct part_entry {
	part_id id;
	struct disk_partitions *parts;
	void *dev_ptr;
	size_t index;	/* e.g. if PM_RAID: this is raids[index] */
	int dev_ptr_delta;
	char fullname[SSTRSIZE];
	enum {PM_DISK=1, PM_PART, PM_SPEC,
	    PM_RAID, PM_CGD, PM_VND, PM_LVM, PM_LVMLV} type;
};

/* Relative file name for storing a distribution. */
extern char xfer_dir[STRSIZE];
extern int  clean_xfer_dir;

#if !defined(SYSINST_FTP_HOST)
#define SYSINST_FTP_HOST	"ftp.NetBSD.org"
#endif

#if !defined(SYSINST_HTTP_HOST)
#define SYSINST_HTTP_HOST	"cdn.NetBSD.org"
#endif

#if !defined(SYSINST_FTP_DIR)
#if defined(NETBSD_OFFICIAL_RELEASE)
#define SYSINST_FTP_DIR		"pub/NetBSD/NetBSD-" REL
#elif defined(REL_PATH)
#define SYSINST_FTP_DIR		"pub/NetBSD-daily/" REL_PATH "/latest"
#else
#define SYSINST_FTP_DIR		"pub/NetBSD/NetBSD-" REL
#endif
#endif

#if !defined(ARCH_SUBDIR)
#define	ARCH_SUBDIR	MACH
#endif
#if !defined(PKG_ARCH_SUBDIR)
#define	PKG_ARCH_SUBDIR	MACH
#endif

#if !defined(SYSINST_PKG_HOST)
#define SYSINST_PKG_HOST	"ftp.NetBSD.org"
#endif
#if !defined(SYSINST_PKG_HTTP_HOST)
#define SYSINST_PKG_HTTP_HOST	"cdn.NetBSD.org"
#endif

#if !defined(SYSINST_PKG_DIR)
#define SYSINST_PKG_DIR		"pub/pkgsrc/packages/NetBSD"
#endif

#if !defined(PKG_SUBDIR)
#define	PKG_SUBDIR		REL
#endif

#if !defined(SYSINST_PKGSRC_HOST)
#define SYSINST_PKGSRC_HOST	SYSINST_PKG_HOST
#endif
#if !defined(SYSINST_PKGSRC_HTTP_HOST)
#define SYSINST_PKGSRC_HTTP_HOST	SYSINST_PKG_HTTP_HOST
#endif

#ifndef SETS_TAR_SUFF
#define	SETS_TAR_SUFF	 "tgz"
#endif

#ifdef	USING_PAXASTAR
#define	TAR_EXTRACT_FLAGS	"-xhepf"
#else
#define	TAR_EXTRACT_FLAGS	"-xpf"
#endif

/* Abs. path we extract binary sets from */
extern char ext_dir_bin[STRSIZE];

/* Abs. path we extract source sets from */
extern char ext_dir_src[STRSIZE];

/* Abs. path we extract pkgsrc from */
extern char ext_dir_pkgsrc[STRSIZE];

/* Place we look for binary sets in all fs types */
extern char set_dir_bin[STRSIZE];

/* Place we look for source sets in all fs types */
extern char set_dir_src[STRSIZE];

/* Place we look for pkgs in all fs types */
extern char pkg_dir[STRSIZE];

/* Place we look for pkgsrc in all fs types */
extern char pkgsrc_dir[STRSIZE];

/* User shell */
extern const char *ushell;

#define	XFER_FTP	0
#define	XFER_HTTP	1
#define	XFER_MAX	XFER_HTTP

struct ftpinfo {
	char xfer_host[XFER_MAX+1][STRSIZE];
	char dir[STRSIZE] ;
	char user[SSTRSIZE];
	char pass[STRSIZE];
	char proxy[STRSIZE];
	unsigned int xfer;	/* XFER_FTP for "ftp" or XFER_HTTP for "http" */
};

/* use the same struct for sets ftp and to build pkgpath */
extern struct ftpinfo ftp, pkg, pkgsrc;

extern int (*fetch_fn)(const char *);
extern char nfs_host[STRSIZE];
extern char nfs_dir[STRSIZE];
extern char entropy_file[PATH_MAX];

extern char cdrom_dev[SSTRSIZE];		/* Typically "cd0a" */
extern char fd_dev[SSTRSIZE];			/* Typically "/dev/fd0a" */
extern const char *fd_type;			/* "msdos", "ffs" or maybe "ados" */

extern char localfs_dev[SSTRSIZE];
extern char localfs_fs[SSTRSIZE];
extern char localfs_dir[STRSIZE];

extern char targetroot_mnt[SSTRSIZE];

extern int  mnt2_mounted;

extern char dist_postfix[SSTRSIZE];
extern char dist_tgz_postfix[SSTRSIZE];

/* needed prototypes */
void set_menu_numopts(int, int);
void remove_color_options(void);
#ifdef CHECK_ENTROPY
bool do_add_entropy(void);
size_t entropy_needed(void);
#endif
void remove_raid_options(void);
void remove_lvm_options(void);
void remove_cgd_options(void);

/* Machine dependent functions .... */
void	md_init(void);
void	md_init_set_status(int); /* SFLAG_foo */

 /* MD functions if user selects install - in order called */
bool	md_get_info(struct install_partition_desc*);
/* returns -1 to restart partitioning, 0 for error, 1 for success */
int	md_make_bsd_partitions(struct install_partition_desc*);
bool	md_check_partitions(struct install_partition_desc*);
#ifdef HAVE_GPT
/*
 * New GPT partitions have been written, update bootloader or remember
 * data untill needed in md_post_newfs
 */
bool	md_gpt_post_write(struct disk_partitions*, part_id root_id,
	    bool root_is_new, part_id efi_id, bool efi_is_new);
#endif
/*
 * md_pre_disklabel and md_post_disklabel may be called
 * multiple times, for each affected device, with the
 * "inner" partitions pointer of the relevant partitions
 * passed.
 */
bool	md_pre_disklabel(struct install_partition_desc*, struct disk_partitions*);
bool	md_post_disklabel(struct install_partition_desc*, struct disk_partitions*);
int	md_pre_mount(struct install_partition_desc*, size_t);
int	md_post_newfs(struct install_partition_desc*);
int	md_post_extract(struct install_partition_desc*, bool upgrade);
void	md_cleanup_install(struct install_partition_desc*);

 /* MD functions if user selects upgrade - in order called */
int	md_pre_update(struct install_partition_desc*);
int	md_update(struct install_partition_desc*);
/* Also calls md_post_extract() */

/* from main.c */
void	toplevel(void);

/* from disks.c */
bool	get_default_cdrom(char *, size_t);
int	find_disks(const char *, bool);
bool enumerate_disks(void *state,bool (*func)(void *state, const char *dev));
bool is_cdrom_device(const char *dev, bool as_target);
bool is_bootable_device(const char *dev);
bool is_partitionable_device(const char *dev);
bool convert_scheme(struct pm_devs *p, bool is_boot_drive, const char **err_msg);

#ifndef	NO_CLONES
/* a single partition selected for cloning (etc) */
struct selected_partition {
	struct disk_partitions *parts;
	part_id id;
};
struct selected_partitions {
	struct selected_partition *selection;
	size_t num_sel;
	bool with_data;		/* partitions and their data selected */
	bool free_parts;	/* caller should free parts */
};
bool select_partitions(struct selected_partitions *res,
    const struct disk_partitions *ignore);
daddr_t	selected_parts_size(struct selected_partitions *);
void	free_selected_partitions(struct selected_partitions *);

struct clone_target_menu_data {
	struct partition_usage_set usage;
	int res;
};

int	clone_target_select(menudesc *m, void *arg);
bool	clone_partition_data(struct disk_partitions *dest_parts, part_id did,
	struct disk_partitions *src_parts, part_id sid);
#endif

struct menudesc;
void	disp_cur_fspart(int, int);
int	make_filesystems(struct install_partition_desc *);
int	make_fstab(struct install_partition_desc *);
int	mount_disks(struct install_partition_desc *);
void	set_swap_if_low_ram(struct install_partition_desc *);
void	set_swap(struct install_partition_desc *);
void	clear_swap(void);
int	check_swap(const char *, int);
char *bootxx_name(struct install_partition_desc *);
int get_dkwedges(struct dkwedge_info **, const char *);

/* from disks_lfs.c */
int	fs_is_lfs(void *);

/* from label.c */
/*
 * Bits valid for "flags" in get_last_mounted.
 * Currently we return the real last mount from FFS, the volume label
 * from FAT32, and nothing otherwise. The NTFS support is currently
 * restricted to verify the partition has an NTFS (as some partitioning
 * schemes do not tell NTFS from FAT).
 */
#define GLM_LIKELY_FFS		1U
#define	GLM_MAYBE_FAT32		2U
#define	GLM_MAYBE_NTFS		4U
/*
 * possible fs_sub_types are currently:
 *  FS_BSDFFS:
 *	0	unknown
 *	1	FFSv1
 *	2	FFSv2
 * FS_MSDOS:
 *	0	unknown
 *	else	MBR_PTYPE_FAT* for the current FAT variant
 * FS_NTFS:
 *	0	unknown
 *	else	MBR_PTYPE_NTFS (if valid NTFS was found)
 *
 * The fs_type and fs_sub_type pointers may be NULL.
 */
const char *get_last_mounted(int fd, daddr_t offset, uint *fs_type,
     uint *fs_sub_type, uint flags);
void	canonicalize_last_mounted(char*);
int	edit_and_check_label(struct pm_devs *p, struct partition_usage_set *pset, bool install);
int edit_ptn(menudesc *, void *);
int checkoverlap(struct disk_partitions *parts);
daddr_t getpartsize(struct disk_partitions *parts, daddr_t orig_start,
    daddr_t partstart, daddr_t defpartsize);
daddr_t getpartoff(struct disk_partitions *parts, daddr_t defpartstart);

/* from install.c */
void	do_install(void);

/* from factor.c */
void	factor(long, long *, int, int *);

/* from fdisk.c */
void	get_disk_info(char *);
void	set_disk_info(char *);

/* from geom.c */
bool	disk_ioctl(const char *, unsigned long, void *);
bool	get_wedge_list(const char *, struct dkwedge_list *);
bool	get_wedge_info(const char *, struct dkwedge_info *);
bool	get_disk_geom(const char *, struct disk_geom *);
bool	get_label_geom(const char *, struct disklabel *);

/* from net.c */
extern int network_up;
extern char net_namesvr[STRSIZE];
int	get_via_ftp(unsigned int);
int	get_via_nfs(void);
int	config_network(void);
void	mnt_net_config(void);
void	make_url(char *, struct ftpinfo *, const char *);
int	get_pkgsrc(void);
const char *url_proto(unsigned int);

/* From run.c */
int	collect(int, char **, const char *, ...) __printflike(3, 4);
int	run_program(int, const char *, ...) __printflike(2, 3);
void	do_logging(void);
int	do_system(const char *);

/* from upgrade.c */
void	do_upgrade(void);
void	do_reinstall_sets(void);
void	restore_etc(void);

/* from part_edit.c */
int	err_msg_win(const char*);
const struct disk_partitioning_scheme *select_part_scheme(struct pm_devs *dev,
    const struct disk_partitioning_scheme *skip, bool bootable,
    const char *title);
/*
 * return value:
 *  0 -> abort
 *  1 -> ok, continue
 *  -1 -> partitions have been deleted, start from scratch
*/
int	edit_outer_parts(struct disk_partitions*);
bool	parts_use_wholedisk(struct disk_partitions*,
	     size_t add_ext_parts, const struct disk_part_info *ext_parts);

/*
 * Machine dependent partitioning function, only used when
 * innern/outer partitioning schemes are in use - this sets
 * up the outer scheme for maximum NetBSD usage.
 */
bool	md_parts_use_wholedisk(struct disk_partitions*);

/* from util.c */
bool	root_is_read_only(void);
void	get_ptn_alignment(const struct disk_partitions *parts, daddr_t *align, daddr_t *p0off);
struct disk_partitions *get_inner_parts(struct disk_partitions *parts);
char*	str_arg_subst(const char *, size_t, const char **);
void	msg_display_subst(const char *, size_t, ...);
void	msg_display_add_subst(const char *, size_t, ...);
int	ask_yesno(const char *);
int	ask_noyes(const char *);
void	hit_enter_to_continue(const char *msg, const char *title);
/*
 * return value:
 *  0 -> abort
 *  1 -> re-edit
 *  2 -> continue installation
*/
int	ask_reedit(const struct disk_partitions *);
int	dir_exists_p(const char *);
int	file_exists_p(const char *);
int	file_mode_match(const char *, unsigned int);
uint64_t	get_ramsize(void);	/* in MB! */
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
void    set_noextract_set(unsigned int);
unsigned int    get_kernel_set(void);
unsigned int    set_X11_selected(void);
int 	get_and_unpack_sets(int, msg, msg, msg);
int	sanity_check(void);
int	set_timezone(void);
void	scripting_fprintf(FILE *, const char *, ...) __printflike(2, 3);
void	scripting_vfprintf(FILE *, const char *, va_list) __printflike(2, 0);
void	add_rc_conf(const char *, ...) __printflike(1, 2);
int	del_rc_conf(const char *);
void	add_sysctl_conf(const char *, ...) __printflike(1, 2);
void	enable_rc_conf(void);
void	set_sizemult(daddr_t, uint bps);
void	set_default_sizemult(const char *disk, daddr_t unit, uint bps);
int	check_lfs_progs(void);
void	init_set_status(int);
void	customise_sets(void);
void	umount_mnt2(void);
int 	set_is_source(const char *);
const char *set_dir_for_set(const char *);
const char *ext_dir_for_set(const char *);
void	replace(const char *, const char *, ...) __printflike(2, 3);
void	get_tz_default(void);
distinfo*	get_set_distinfo(int);
int	extract_file(distinfo *, int);
int extract_file_to(distinfo *dist, int update, const char *dest_dir,
    const char *extr_pattern, bool do_stats);
void	do_coloring (unsigned int, unsigned int);
int set_menu_select(menudesc *, void *);
const char *safectime(time_t *);
bool	use_tgz_for_set(const char*);
const char *set_postfix(const char*);
bool	usage_set_from_parts(struct partition_usage_set*,
	    struct disk_partitions*);
void	free_usage_set(struct partition_usage_set*);
bool	install_desc_from_parts(struct install_partition_desc *,
	    struct disk_partitions*);
void	free_install_desc(struct install_partition_desc*);
bool	may_swap_if_not_sdmmc(const char*);

/* from target.c */
#if defined(DEBUG)  ||	defined(DEBUG_ROOT)
void	backtowin(void);
#endif
bool	is_root_part_mount(const char *);
const	char *concat_paths(const char *, const char *);
const	char *target_expand(const char *);
bool	needs_expanding(const char *, size_t);
void	make_target_dir(const char *);
void	append_to_target_file(const char *, const char *);
void	echo_to_target_file(const char *, const char *);
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
int	target_mount(const char *, const char *, const char *);
int	target_mount_do(const char *, const char *, const char *);
int	target_test(unsigned int, const char *);
int	target_dir_exists_p(const char *);
int	target_file_exists_p(const char *);
int	target_symlink_exists_p(const char *);
void	unwind_mounts(void);
void	register_post_umount_delwedge(const char *disk, const char *wedge);
int	target_mounted(void);
void	umount_root(void);

/* from partman.c */
#ifndef NO_PARTMAN
int partman(void);
int pm_getrefdev(struct pm_devs *);
void update_wedges(const char *);
void pm_destroy_all(void);
#else
static inline int partman(void) { return -1; }
static inline int pm_getrefdev(struct pm_devs *x __unused) { return -1; }
#define update_wedges(x) __nothing
#endif
void pmdiskentry_enable(menudesc*, struct part_entry *);
int pm_partusage(struct pm_devs *, int, int);
void pm_setfstype(struct pm_devs *, part_id, int, int);
void pm_set_lvmpv(struct pm_devs *, part_id, bool);
bool pm_is_lvmpv(struct pm_devs *, part_id, const struct disk_part_info*);
int pm_editpart(int);
void pm_rename(struct pm_devs *);
void pm_shred(struct part_entry *, int);
void pm_umount(struct pm_devs *, int);
int pm_unconfigure(struct pm_devs *);
int pm_cgd_edit_new(struct pm_devs *pm, part_id id);
int pm_cgd_edit_old(struct part_entry *);
void pm_wedges_fill(struct pm_devs *);
void pm_edit_partitions(struct part_entry *);
part_id pm_whole_disk(struct part_entry *, int);
struct pm_devs * pm_from_pe(struct part_entry *);
bool pm_force_parts(struct pm_devs *);

/*
 * Parse a file system position or size in a common way, return
 * sector count and multiplicator.
 * If "extend" is supported, things like 120+ will be parsed as
 * 120 plus "extend this" flag.
 * Caller needs to init muliplicator upfront to the default value.
 */
daddr_t parse_disk_pos(
	const char *,	/* in: input string */
	daddr_t *,	/* in/out: multiplicator for return value */
	daddr_t bps,	/* in: sector size in bytes */
	daddr_t,	/* in: cylinder size in sectors */
	bool *);	/* NULL if "extend" is not supported, & of
			 * "extend" flag otherwise */

/* flags whether to offer the respective options (depending on helper
   programs available on install media */
extern int have_raid, have_vnd, have_cgd, have_lvm, have_gpt, have_dk;
/* initialize above variables */
void check_available_binaries(void);

/* from bsddisklabel.c */
/* returns -1 to restart partitioning, 0 for error, 1 for success */
int	make_bsd_partitions(struct install_partition_desc*);
void	set_ptn_titles(menudesc *, int, void *);
int	set_ptn_size(menudesc *, void *);
bool	get_ptn_sizes(struct partition_usage_set*);
bool	check_partitions(struct install_partition_desc*);

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
void	do_configmenu(struct install_partition_desc*);

/* from checkrc.c */
int	check_rcvar(const char *);
int	check_rcdefault(const char *);
extern	WINDOW *mainwin;

/* in menus.mi */
void expand_all_option_texts(menudesc *menu, void *arg);
void resize_menu_height(menudesc *);

#endif	/* _DEFS_H_ */
