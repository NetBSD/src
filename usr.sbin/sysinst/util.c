/*	$NetBSD: util.c,v 1.67 2022/05/18 16:39:03 martin Exp $	*/

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

/* util.c -- routines that don't really fit anywhere else... */

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/dkio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <isofs/cd9660/iso.h>
#include <curses.h>
#include <err.h>
#include <errno.h>
#include <dirent.h>
#include <util.h>
#include "defs.h"
#include "md.h"
#include "defsizes.h"
#include "msg_defs.h"
#include "menu_defs.h"
#ifdef MD_MAY_SWAP_TO
#include <sys/drvctlio.h>
#endif
#ifdef CHECK_ENTROPY
#include <sha2.h>
#include <paths.h>
#endif

#define MAX_CD_DEVS	256	/* how many cd drives do we expect to attach */
#define ISO_BLKSIZE	ISO_DEFAULT_BLOCK_SIZE

static const char *msg_yes, *msg_no, *msg_all, *msg_some, *msg_none;
static int select_menu_width;

static uint8_t set_status[SET_GROUP_END];
#define SET_VALID	0x01
#define SET_SELECTED	0x02
#define SET_SKIPPED	0x04
#define SET_INSTALLED	0x08
#define	SET_NO_EXTRACT	0x10

struct  tarstats {
	int nselected;
	int nfound;
	int nnotfound;
	int nerror;
	int nsuccess;
	int nskipped;
} tarstats;

distinfo dist_list[] = {
#ifdef SET_KERNEL_1_NAME
	{SET_KERNEL_1_NAME,	SET_KERNEL_1,		false, MSG_set_kernel_1, NULL},
#endif
#ifdef SET_KERNEL_2_NAME
	{SET_KERNEL_2_NAME,	SET_KERNEL_2,		false, MSG_set_kernel_2, NULL},
#endif
#ifdef SET_KERNEL_3_NAME
	{SET_KERNEL_3_NAME,	SET_KERNEL_3,		false, MSG_set_kernel_3, NULL},
#endif
#ifdef SET_KERNEL_4_NAME
	{SET_KERNEL_4_NAME,	SET_KERNEL_4,		false, MSG_set_kernel_4, NULL},
#endif
#ifdef SET_KERNEL_5_NAME
	{SET_KERNEL_5_NAME,	SET_KERNEL_5,		false, MSG_set_kernel_5, NULL},
#endif
#ifdef SET_KERNEL_6_NAME
	{SET_KERNEL_6_NAME,	SET_KERNEL_6,		false, MSG_set_kernel_6, NULL},
#endif
#ifdef SET_KERNEL_7_NAME
	{SET_KERNEL_7_NAME,	SET_KERNEL_7,		false, MSG_set_kernel_7, NULL},
#endif
#ifdef SET_KERNEL_8_NAME
	{SET_KERNEL_8_NAME,	SET_KERNEL_8,		false, MSG_set_kernel_8, NULL},
#endif
#ifdef SET_KERNEL_9_NAME
	{SET_KERNEL_9_NAME,	SET_KERNEL_9,		false, MSG_set_kernel_9, NULL},
#endif

#ifdef HAVE_MODULES
	{"modules",		SET_MODULES,		false, MSG_set_modules, NULL},
#endif
	{"base",		SET_BASE,		false, MSG_set_base, NULL},
#ifdef HAVE_DTB
	{"dtb",			SET_DTB,		false, MSG_set_dtb, NULL},
#endif
	{"etc",			SET_ETC,		false, MSG_set_system, NULL},
	{"comp",		SET_COMPILER,		false, MSG_set_compiler, NULL},
	{"games",		SET_GAMES,		false, MSG_set_games, NULL},
	{"gpufw",		SET_GPUFW,		false, MSG_set_gpufw, NULL},
	{"man",			SET_MAN_PAGES,		false, MSG_set_man_pages, NULL},
	{"misc",		SET_MISC,		false, MSG_set_misc, NULL},
	{"rescue",		SET_RESCUE,		false, MSG_set_rescue, NULL},
	{"tests",		SET_TESTS,		false, MSG_set_tests, NULL},
	{"text",		SET_TEXT_TOOLS,		false, MSG_set_text_tools, NULL},

	{NULL,			SET_GROUP,		false, MSG_set_X11, NULL},
	{"xbase",		SET_X11_BASE,		false, MSG_set_X11_base, NULL},
	{"xcomp",		SET_X11_PROG,		false, MSG_set_X11_prog, NULL},
	{"xetc",		SET_X11_ETC,		false, MSG_set_X11_etc, NULL},
	{"xfont",		SET_X11_FONTS,		false, MSG_set_X11_fonts, NULL},
	{"xserver",		SET_X11_SERVERS,	false, MSG_set_X11_servers, NULL},
	{NULL,			SET_GROUP_END,		false, NULL, NULL},

#ifdef SET_MD_1_NAME
	{SET_MD_1_NAME,		SET_MD_1,		false, MSG_set_md_1, NULL},
#endif
#ifdef SET_MD_2_NAME
	{SET_MD_2_NAME,		SET_MD_2,		false, MSG_set_md_2, NULL},
#endif
#ifdef SET_MD_3_NAME
	{SET_MD_3_NAME,		SET_MD_3,		false, MSG_set_md_3, NULL},
#endif
#ifdef SET_MD_4_NAME
	{SET_MD_4_NAME,		SET_MD_4,		false, MSG_set_md_4, NULL},
#endif

	{NULL,			SET_GROUP,		true, MSG_set_source, NULL},
	{"syssrc",		SET_SYSSRC,		true, MSG_set_syssrc, NULL},
	{"src",			SET_SRC,		true, MSG_set_src, NULL},
	{"sharesrc",		SET_SHARESRC,		true, MSG_set_sharesrc, NULL},
	{"gnusrc",		SET_GNUSRC,		true, MSG_set_gnusrc, NULL},
	{"xsrc",		SET_XSRC,		true, MSG_set_xsrc, NULL},
	{"debug",		SET_DEBUG,		false, MSG_set_debug, NULL},
	{"xdebug",		SET_X11_DEBUG,		false, MSG_set_xdebug, NULL},
	{NULL,			SET_GROUP_END,		false, NULL, NULL},

	{NULL,			SET_LAST,		false, NULL, NULL},
};

#define MAX_CD_INFOS	16	/* how many media can be found? */
struct cd_info {
	char device_name[16];
	char menu[100];
};
static struct cd_info cds[MAX_CD_INFOS];

/* flags whether to offer the respective options (depending on helper
   programs available on install media */
int have_raid, have_vnd, have_cgd, have_lvm, have_gpt, have_dk;

/*
 * local prototypes
 */

static int check_for(unsigned int mode, const char *pathname);
static int get_iso9660_volname(int dev, int sess, char *volname,
		size_t volnamelen);
static int get_available_cds(void);
static int binary_available(const char *prog);

void
init_set_status(int flags)
{
	static const uint8_t sets_valid[] = {MD_SETS_VALID};
	static const uint8_t sets_selected_full[] = {MD_SETS_SELECTED};
	static const uint8_t sets_selected_minimal[] = {MD_SETS_SELECTED_MINIMAL};
	static const uint8_t sets_selected_nox[] = {MD_SETS_SELECTED_NOX};
	static const uint8_t *sets_selected;
	unsigned int nelem_selected;
	unsigned int i, len;
	const char *longest;

	if (flags & SFLAG_MINIMAL) {
		sets_selected = sets_selected_minimal;
		nelem_selected = __arraycount(sets_selected_minimal);
	} else if (flags & SFLAG_NOX) {
		sets_selected = sets_selected_nox;
		nelem_selected = __arraycount(sets_selected_nox);
	} else {
		sets_selected = sets_selected_full;
		nelem_selected = __arraycount(sets_selected_full);
	}

	for (i = 0; i < __arraycount(sets_valid); i++)
		set_status[sets_valid[i]] = SET_VALID;
	for (i = 0; i < nelem_selected; i++)
		set_status[sets_selected[i]] |= SET_SELECTED;

	set_status[SET_GROUP] = SET_VALID;

	/* Lookup some strings we need lots of times */
	msg_yes = msg_string(MSG_Yes);
	msg_no = msg_string(MSG_No);
	msg_all = msg_string(MSG_All);
	msg_some = msg_string(MSG_Some);
	msg_none = msg_string(MSG_None);

	/* Find longest and use it to determine width of selection menu */
	len = strlen(msg_no); longest = msg_no;
	i = strlen(msg_yes); if (i > len) {len = i; longest = msg_yes; }
	i = strlen(msg_all); if (i > len) {len = i; longest = msg_all; }
	i = strlen(msg_some); if (i > len) {len = i; longest = msg_some; }
	i = strlen(msg_none); if (i > len) {len = i; longest = msg_none; }
	select_menu_width = snprintf(NULL, 0, "%-30s %s", "", longest);

	/* Give the md code a chance to choose the right kernel, etc. */
	md_init_set_status(flags);
}

int
dir_exists_p(const char *path)
{

	return file_mode_match(path, S_IFDIR);
}

int
file_exists_p(const char *path)
{

	return file_mode_match(path, S_IFREG);
}

int
file_mode_match(const char *path, unsigned int mode)
{
	struct stat st;

	return (stat(path, &st) == 0 && (st.st_mode & S_IFMT) == mode);
}

/* return ram size in MB */
uint64_t
get_ramsize(void)
{
	static uint64_t ramsize;

	if (ramsize == 0) {
		size_t len = sizeof ramsize;
		int mib[2] = {CTL_HW, HW_PHYSMEM64};

		sysctl(mib, 2, &ramsize, &len, NULL, 0);
	}

	/* Find out how many Megs ... round up. */
	return (ramsize + MEG - 1) / MEG;
}

void
run_makedev(void)
{
	char *owd;

	msg_display_add("\n\n");
	msg_display_add(MSG_makedev);

	owd = getcwd(NULL, 0);

	/* make /dev, in case the user  didn't extract it. */
	make_target_dir("/dev");
	target_chdir_or_die("/dev");
	run_program(0, "/bin/sh MAKEDEV all");

	chdir(owd);
	free(owd);
}

/*
 * Performs in-place replacement of a set of patterns in a file that lives
 * inside the installed system.  The patterns must be separated by a semicolon.
 * For example:
 *
 * replace("/etc/some-file.conf", "s/prop1=NO/prop1=YES/;s/foo/bar/");
 */
void
replace(const char *path, const char *patterns, ...)
{
	char *spatterns;
	va_list ap;

	va_start(ap, patterns);
	vasprintf(&spatterns, patterns, ap);
	va_end(ap);
	if (spatterns == NULL)
		err(1, "vasprintf(&spatterns, \"%s\", ...)", patterns);

	run_program(RUN_CHROOT, "sed -an -e '%s;H;$!d;g;w %s' %s", spatterns,
	    path, path);

	free(spatterns);
}

static int
floppy_fetch(const char *set_name)
{
	char post[4];
	msg errmsg;
	int menu;
	int status;
	const char *write_mode = ">";

	strcpy(post, "aa");

	errmsg = "";
	menu = MENU_fdok;
	for (;;) {
		umount_mnt2();
		msg_display(errmsg);
		msg_fmt_display_add(MSG_fdmount, "%s%s", set_name, post);
		process_menu(menu, &status);
		if (status != SET_CONTINUE)
			return status;
		menu = MENU_fdremount;
		errmsg = MSG_fdremount;
		if (run_program(0, "/sbin/mount -r -t %s %s /mnt2",
							fd_type, fd_dev))
			continue;
		mnt2_mounted = 1;
		errmsg = MSG_fdnotfound;

		/* Display this because it might take a while.... */
		if (run_program(RUN_DISPLAY,
			    "sh -c '/bin/cat /mnt2/%s.%s %s %s/%s/%s%s'",
			    set_name, post, write_mode,
			    target_prefix(), xfer_dir, set_name,
			    set_postfix(set_name)))
			/* XXX: a read error will give a corrupt file! */
			continue;

		/* We got that file, advance to next fragment */
		if (post[1] < 'z')
			post[1]++;
		else
			post[1] = 'a', post[0]++;
		write_mode = ">>";
		errmsg = "";
		menu = MENU_fdok;
	}
}

/*
 * Load files from floppy.  Requires a /mnt2 directory for mounting them.
 */
int
get_via_floppy(void)
{
	int rv = -1;

	process_menu(MENU_floppysource, &rv);
	if (rv == SET_RETRY)
		return SET_RETRY;

	fetch_fn = floppy_fetch;

	/* Set ext_dir for absolute path. */
	snprintf(ext_dir_bin, sizeof ext_dir_bin, "%s/%s", target_prefix(), xfer_dir);
	snprintf(ext_dir_src, sizeof ext_dir_src, "%s/%s", target_prefix(), xfer_dir);

	return SET_OK;
}

/*
 * Get the volume name of a ISO9660 file system
 */
static int
get_iso9660_volname(int dev, int sess, char *volname, size_t volnamelen)
{
	int blkno, error, last;
	char buf[ISO_BLKSIZE];
	struct iso_volume_descriptor *vd = NULL;
	struct iso_primary_descriptor *pd = NULL;

	for (blkno = sess+16; blkno < sess+16+100; blkno++) {
		error = pread(dev, buf, ISO_BLKSIZE, blkno*ISO_BLKSIZE);
		if (error == -1)
			return -1;
		vd = (struct iso_volume_descriptor *)&buf;
		if (memcmp(vd->id, ISO_STANDARD_ID, sizeof(vd->id)) != 0)
			return -1;
		if (isonum_711((const unsigned char *)&vd->type)
		     == ISO_VD_PRIMARY) {
			pd = (struct iso_primary_descriptor*)buf;
			strncpy(volname, pd->volume_id, volnamelen - 1);
			volname[volnamelen - 1] = '\0';
			last = volnamelen - 1;
			while (last >= 0
			    && (volname[last] == ' ' || volname[last] == 0))
				last--;
			volname[last+1] = 0;
			return 0;
		}
	}
	return -1;
}

/*
 * Local state while iterating CDs and collecting volumes
 */
struct get_available_cds_state {
	size_t num_mounted;
	struct statvfs *mounted;
	struct cd_info *info;
	size_t count;
};

/*
 * Callback function: if this is a CD, enumerate all volumes on it
 */
static bool
get_available_cds_helper(void *arg, const char *device)
{
	struct get_available_cds_state *state = arg;
	char dname[16], tname[16], volname[80], *t;
	struct disklabel label;
	int part, dev, error, sess, ready, tlen;

	if (!is_cdrom_device(device, false))
		return true;

	sprintf(dname, "/dev/r%s%c", device, 'a'+RAW_PART);
	tlen = sprintf(tname, "/dev/%s", device);

	/* check if this is mounted already */
	for (size_t i = 0; i < state->num_mounted; i++) {
		if (strncmp(state->mounted[i].f_mntfromname, tname, tlen)
		    == 0) {
			t = state->mounted[i].f_mntfromname + tlen;
			if (t[0] >= 'a' && t[0] <= 'z' && t[1] == 0)
				return true;
		}
	}

	dev = open(dname, O_RDONLY, 0);
	if (dev == -1)
		return true;

	ready = 0;
	error = ioctl(dev, DIOCTUR, &ready);
	if (error != 0 || ready == 0) {
		close(dev);
		return true;
	}
	error = ioctl(dev, DIOCGDINFO, &label);
	close(dev);
	if (error != 0)
		return true;

	for (part = 0; part < label.d_npartitions; part++) {

		if (label.d_partitions[part].p_fstype == FS_UNUSED
		    || label.d_partitions[part].p_size == 0)
			continue;

		if (label.d_partitions[part].p_fstype == FS_ISO9660) {
			sess = label.d_partitions[part].p_cdsession;
			sprintf(dname, "/dev/r%s%c", device, 'a'+part);
			dev = open(dname, O_RDONLY, 0);
			if (dev == -1)
				continue;
			error = get_iso9660_volname(dev, sess, volname,
			    sizeof volname);
			close(dev);
			if (error)
				continue;
			sprintf(state->info->device_name,
			    "%s%c", device, 'a'+part);
			sprintf(state->info->menu, "%s (%s)",
			    state->info->device_name, volname);
		} else {
			/*
			 * All install CDs use partition
			 * a for the sets.
			 */
			if (part > 0)
				continue;
			sprintf(state->info->device_name,
			    "%s%c", device, 'a'+part);
			strcpy(state->info->menu, state->info->device_name);
		}
		state->info++;
		if (++state->count >= MAX_CD_INFOS)
			return false;
	}

	return true;
}

/*
 * Get a list of all available CD media (not drives!), return
 * the number of entries collected.
 */
static int
get_available_cds(void)
{
	struct get_available_cds_state data;
	int n, m;

	memset(&data, 0, sizeof data);
	data.info = cds;

	n = getvfsstat(NULL, 0, ST_NOWAIT);
	if (n > 0) {
		data.mounted = calloc(n, sizeof(*data.mounted));
		m = getvfsstat(data.mounted, n*sizeof(*data.mounted),
		    ST_NOWAIT);
		assert(m >= 0 && m <= n);
		data.num_mounted = m;
	}

	enumerate_disks(&data, get_available_cds_helper);

	free(data.mounted);

	return data.count;
}

static int
cd_has_sets(void)
{

	/* sanity check */
	if (cdrom_dev[0] == 0)
		return 0;

	/* Mount it */
	if (run_program(RUN_SILENT, "/sbin/mount -rt cd9660 /dev/%s /mnt2",
	    cdrom_dev) != 0)
		return 0;

	mnt2_mounted = 1;

	snprintf(ext_dir_bin, sizeof ext_dir_bin, "%s/%s", "/mnt2", set_dir_bin);
	snprintf(ext_dir_src, sizeof ext_dir_src, "%s/%s", "/mnt2", set_dir_src);
	return dir_exists_p(ext_dir_bin);
}

/*
 * Check whether we can remove the boot media.
 * If it is not a local filesystem, return -1.
 * If we can not decide for sure (can not tell MD content from plain ffs
 * on hard disk, for example), return 0.
 * If it is a CD/DVD, return 1.
 */
int
boot_media_still_needed(void)
{
	struct statvfs sb;

	if (statvfs("/", &sb) == 0) {
		if (!(sb.f_flag & ST_LOCAL))
			return -1;
		if (strcmp(sb.f_fstypename, MOUNT_CD9660) == 0
			   || strcmp(sb.f_fstypename, MOUNT_UDF) == 0)
			return 1;
	}

	return 0;
}

bool
root_is_read_only(void)
{
	struct statvfs sb;

	if (statvfs("/", &sb) == 0)
		return sb.f_flag & ST_RDONLY;

	return false;
}

/*
 * Get from a CDROM distribution.
 * Also used on "installation using bootable install media"
 * as the default option in the "distmedium" menu.
 */
int
get_via_cdrom(void)
{
	menu_ent cd_menu[MAX_CD_INFOS];
	struct stat sb;
	int rv, num_cds, menu_cd, i, selected_cd = 0;
	int mib[2];
	char rootdev[SSTRSIZE] = "";
	size_t varlen;

	/* If root is not md(4) and we have set dir, skip this step. */
	mib[0] = CTL_KERN;
	mib[1] = KERN_ROOT_DEVICE;
	varlen = sizeof(rootdev);
	(void)sysctl(mib, 2, rootdev, &varlen, NULL, 0);
	if (stat(set_dir_bin, &sb) == 0 && S_ISDIR(sb.st_mode) &&
	    strncmp("md", rootdev, 2) != 0) {
	    	strlcpy(ext_dir_bin, set_dir_bin, sizeof ext_dir_bin);
	    	strlcpy(ext_dir_src, set_dir_src, sizeof ext_dir_src);
		return SET_OK;
	}

	memset(cd_menu, 0, sizeof(cd_menu));
	num_cds = get_available_cds();
	if (num_cds <= 0) {
		msg_display(MSG_No_cd_found);
		cdrom_dev[0] = 0;
	} else if (num_cds == 1) {
		/* single CD found, check for sets on it */
		strcpy(cdrom_dev, cds[0].device_name);
		if (cd_has_sets())
			return SET_OK;
	} else {
		for (i = 0; i< num_cds; i++) {
			cd_menu[i].opt_name = cds[i].menu;
			cd_menu[i].opt_flags = OPT_EXIT;
			cd_menu[i].opt_action = set_menu_select;
		}
		/* create a menu offering available choices */
		menu_cd = new_menu(MSG_Available_cds,
			cd_menu, num_cds, -1, 4, 0, 0,
			MC_SCROLL | MC_NOEXITOPT,
			NULL, NULL, NULL, NULL, NULL);
		if (menu_cd == -1)
			return SET_RETRY;
		msg_display(MSG_ask_cd);
		process_menu(menu_cd, &selected_cd);
		free_menu(menu_cd);
		strcpy(cdrom_dev, cds[selected_cd].device_name);
		if (cd_has_sets())
			return SET_OK;
	}

	if (num_cds >= 1 && mnt2_mounted) {
		umount_mnt2();
		hit_enter_to_continue(MSG_cd_path_not_found, NULL);
	}

	/* ask for paths on the CD */
	rv = -1;
	process_menu(MENU_cdromsource, &rv);
	if (rv == SET_RETRY || rv == SET_ABANDON)
		return rv;

	if (cd_has_sets())
		return SET_OK;

	return SET_RETRY;
}


/*
 * Get from a pathname inside an unmounted local filesystem
 * (e.g., where sets were preloaded onto a local DOS partition)
 */
int
get_via_localfs(void)
{
	int rv = -1;

	/* Get device, filesystem, and filepath */
	process_menu (MENU_localfssource, &rv);
	if (rv == SET_RETRY)
		return SET_RETRY;

	/* Mount it */
	if (run_program(0, "/sbin/mount -rt %s /dev/%s /mnt2",
	    localfs_fs, localfs_dev))
		return SET_RETRY;

	mnt2_mounted = 1;

	snprintf(ext_dir_bin, sizeof ext_dir_bin, "%s/%s/%s",
		"/mnt2", localfs_dir, set_dir_bin);
	snprintf(ext_dir_src, sizeof ext_dir_src, "%s/%s/%s",
		"/mnt2", localfs_dir, set_dir_src);

	return SET_OK;
}

/*
 * Get from an already-mounted pathname.
 */

int
get_via_localdir(void)
{
	int rv = -1;

	/* Get filepath */
	process_menu(MENU_localdirsource, &rv);
	if (rv == SET_RETRY)
		return SET_RETRY;

	/*
	 * We have to have an absolute path ('cos pax runs in a
	 * different directory), make it so.
	 */
	snprintf(ext_dir_bin, sizeof ext_dir_bin, "/%s/%s", localfs_dir, set_dir_bin);
	snprintf(ext_dir_src, sizeof ext_dir_src, "/%s/%s", localfs_dir, set_dir_src);

	return SET_OK;
}


/*
 * Support for custom distribution fetches / unpacks.
 */

unsigned int
set_X11_selected(void)
{
	int i;

	for (i = SET_X11_FIRST; ++i < SET_X11_LAST;)
		if (set_status[i] & SET_SELECTED)
			return 1;
	return 0;
}

unsigned int
get_kernel_set(void)
{
	int i;

	for (i = SET_KERNEL_FIRST; ++i < SET_KERNEL_LAST;)
		if (set_status[i] & SET_SELECTED)
			return i;
	return SET_NONE;
}

void
set_kernel_set(unsigned int kernel_set)
{
	int i;

	/* only one kernel set is allowed */
	for (i = SET_KERNEL_FIRST; ++i < SET_KERNEL_LAST;)
		set_status[i] &= ~SET_SELECTED;
	set_status[kernel_set] |= SET_SELECTED;
}

void
set_noextract_set(unsigned int set)
{

	set_status[set] |= SET_NO_EXTRACT;
}

static int
set_toggle(menudesc *menu, void *arg)
{
	distinfo **distp = arg;
	int set = distp[menu->cursel]->set;

	if (set > SET_KERNEL_FIRST && set < SET_KERNEL_LAST &&
	    !(set_status[set] & SET_SELECTED))
		set_kernel_set(set);
	else
		set_status[set] ^= SET_SELECTED;
	return 0;
}

static int
set_all_none(menudesc *menu, void *arg, int set, int clr)
{
	distinfo **distp = arg;
	distinfo *dist = *distp;
	int nested;

	for (nested = 0; dist->set != SET_GROUP_END || nested--; dist++) {
		if (dist->set == SET_GROUP) {
			nested++;
			continue;
		}
		set_status[dist->set] = (set_status[dist->set] & ~clr) | set;
	}
	return 0;
}

static int
set_all(menudesc *menu, void *arg)
{
	return set_all_none(menu, arg, SET_SELECTED, 0);
}

static int
set_none(menudesc *menu, void *arg)
{
	return set_all_none(menu, arg, 0, SET_SELECTED);
}

static void
set_label(menudesc *menu, int opt, void *arg)
{
	distinfo **distp = arg;
	distinfo *dist = distp[opt];
	const char *selected;
	const char *desc;
	int nested;

	desc = dist->desc;

	if (dist->set != SET_GROUP)
		selected = set_status[dist->set] & SET_SELECTED ? msg_yes : msg_no;
	else {
		/* sub menu - display None/Some/All */
		nested = 0;
		selected = "unknown";
		while ((++dist)->set != SET_GROUP_END || nested--) {
			if (dist->set == SET_GROUP) {
				nested++;
				continue;
			}
			if (!(set_status[dist->set] & SET_VALID))
				continue;
			if (set_status[dist->set] & SET_SELECTED) {
				if (selected == msg_none) {
					selected = msg_some;
					break;
				}
				selected = msg_all;
			} else {
				if (selected == msg_all) {
					selected = msg_some;
					break;
				}
				selected = msg_none;
			}
		}
	}

	wprintw(menu->mw, "%-30s %s", msg_string(desc), selected);
}

static int set_sublist(menudesc *menu, void *arg);

static int
initialise_set_menu(distinfo *dist, menu_ent *me, distinfo **de, int all_none)
{
	int set;
	int sets;
	int nested;

	for (sets = 0; ; dist++) {
		set = dist->set;
		if (set == SET_LAST || set == SET_GROUP_END)
			break;
		if (!(set_status[set] & SET_VALID))
			continue;
		*de = dist;
		memset(me, 0, sizeof(*me));
		if (set != SET_GROUP)
			me->opt_action = set_toggle;
		else {
			/* Collapse sublist */
			nested = 0;
			while ((++dist)->set != SET_GROUP_END || nested--) {
				if (dist->set == SET_GROUP)
					nested++;
			}
			me->opt_action = set_sublist;
		}
		sets++;
		de++;
		me++;
	}

	if (all_none) {
		me->opt_name = MSG_select_all;
		me->opt_action = set_all;
		me++;
		me->opt_name = MSG_select_none;
		me->opt_action = set_none;
		sets += 2;
	}

	return sets;
}

static int
set_sublist(menudesc *menu, void *arg)
{
	distinfo *de[SET_LAST];
	menu_ent me[SET_LAST];
	distinfo **dist = arg;
	int menu_no;
	int sets;

	memset(me, 0, sizeof(me));
	sets = initialise_set_menu(dist[menu->cursel] + 1, me, de, 1);

	menu_no = new_menu(NULL, me, sets, 20, 10, 0, select_menu_width,
		MC_SUBMENU | MC_SCROLL | MC_DFLTEXIT,
		NULL, set_label, NULL, NULL,
		MSG_install_selected_sets);

	process_menu(menu_no, de);
	free_menu(menu_no);

	return 0;
}

void
customise_sets(void)
{
	distinfo *de[SET_LAST];
	menu_ent me[SET_LAST];
	int sets;
	int menu_no;

	msg_display(MSG_cur_distsets);
	msg_table_add(MSG_cur_distsets_header);

	memset(me, 0, sizeof(me));
	sets = initialise_set_menu(dist_list, me, de, 0);

	menu_no = new_menu(NULL, me, sets, 0, 5, 0, select_menu_width,
		MC_SCROLL | MC_NOBOX | MC_DFLTEXIT | MC_NOCLEAR,
		NULL, set_label, NULL, NULL,
		MSG_install_selected_sets);

	process_menu(menu_no, de);
	free_menu(menu_no);
}

/*
 * Extract_file **REQUIRES** an absolute path in ext_dir.  Any code
 * that sets up xfer_dir for use by extract_file needs to put in the
 * full path name to the directory.
 */

int
extract_file(distinfo *dist, int update)
{
	const char *dest_dir = NULL;

	if (update && (dist->set == SET_ETC || dist->set == SET_X11_ETC)) {
		dest_dir = "/.sysinst";
		make_target_dir(dest_dir);
	} else if (dist->set == SET_PKGSRC)
		dest_dir = "/usr";
	else
		dest_dir = "/";

	return extract_file_to(dist, update, dest_dir, NULL, true);
}

int
extract_file_to(distinfo *dist, int update, const char *dest_dir,
    const char *extr_pattern, bool do_stats)
{
	char path[STRSIZE];
	char *owd;
	int   rval;

	/* If we might need to tidy up, ensure directory exists */
	if (fetch_fn != NULL)
		make_target_dir(xfer_dir);

	(void)snprintf(path, sizeof path, "%s/%s%s",
	    ext_dir_for_set(dist->name), dist->name, set_postfix(dist->name));

	owd = getcwd(NULL, 0);

	/* Do we need to fetch the file now? */
	if (fetch_fn != NULL) {
		rval = fetch_fn(dist->name);
		if (rval != SET_OK)
			return rval;
	}

	/* check tarfile exists */
	if (!file_exists_p(path)) {

#ifdef SUPPORT_8_3_SOURCE_FILESYSTEM
		/*
		 * Update path to use dist->name truncated to the first eight
		 * characters and check again
		 */
		(void)snprintf(path, sizeof path,
		    "%s/%.8s%.4s", /* 4 as includes '.' */
		    ext_dir_for_set(dist->name), dist->name,
		    set_postfix(dist->name));

		if (!file_exists_p(path)) {
#endif /* SUPPORT_8_3_SOURCE_FILESYSTEM */
			if (do_stats)
				tarstats.nnotfound++;

			char *err = str_arg_subst(msg_string(MSG_notarfile),
			    1, &dist->name);
			hit_enter_to_continue(err, NULL);
			free(err);
			free(owd);
			return SET_RETRY;
		}
#ifdef SUPPORT_8_3_SOURCE_FILESYSTEM
	}
#endif /* SUPPORT_8_3_SOURCE_FILESYSTEM */

	if (do_stats)
		tarstats.nfound++;
	/* cd to the target root. */
	target_chdir_or_die(dest_dir);

	/*
	 * /usr/X11R7/lib/X11/xkb/symbols/pc was a directory in 5.0
	 * but is a file in 5.1 and beyond, so on upgrades we need to
	 * delete it before extracting the xbase set.
	 */
	if (update && dist->set == SET_X11_BASE)
		run_program(0, "rm -rf usr/X11R7/lib/X11/xkb/symbols/pc");

	/* now extract set files into "./". */
	if (extr_pattern != NULL) {
		rval = run_program(RUN_DISPLAY | RUN_PROGRESS,
				"progress -zf %s tar --chroot "
				TAR_EXTRACT_FLAGS " - '%s'",
				path, extr_pattern);
	} else {
		rval = run_program(RUN_DISPLAY | RUN_PROGRESS,
				"progress -zf %s tar --chroot "
				TAR_EXTRACT_FLAGS " -", path);
	}

	chdir(owd);
	free(owd);

	/* Check rval for errors and give warning. */
	if (rval != 0) {
		if (do_stats)
			tarstats.nerror++;
		msg_fmt_display(MSG_tarerror, "%s", path);
		hit_enter_to_continue(NULL, NULL);
		return SET_RETRY;
	}

	if (fetch_fn != NULL && clean_xfer_dir) {
		run_program(0, "rm %s", path);
		/* Plausibly we should unlink an empty xfer_dir as well */
	}

	set_status[dist->set] |= SET_INSTALLED;
	if (do_stats)
		tarstats.nsuccess++;
	return SET_OK;
}

static void
skip_set(distinfo *dist, int skip_type)
{
	int nested;
	int set;

	nested = 0;
	while ((++dist)->set != SET_GROUP_END || nested--) {
		set = dist->set;
		if (set == SET_GROUP) {
			nested++;
			continue;
		}
		if (set == SET_LAST)
			break;
		if (set_status[set] == (SET_SELECTED | SET_VALID))
			set_status[set] |= SET_SKIPPED;
		tarstats.nskipped++;
	}
}

distinfo*
get_set_distinfo(int opt)
{
	distinfo *dist;
	int set;

	for (dist = dist_list; (set = dist->set) != SET_LAST; dist++) {
		if (set != opt)
			continue;
		if (dist->name == NULL)
			continue;
		if ((set_status[set] & (SET_VALID | SET_SELECTED))
		    != (SET_VALID | SET_SELECTED))
			continue;
		return dist;
	}

	return NULL;
}

#ifdef CHECK_ENTROPY

char entropy_file[PATH_MAX];

/*
 * Are we short of entropy?
 */
size_t
entropy_needed(void)
{
	int needed;
	size_t len;

	len = sizeof(needed);
	if (sysctlbyname("kern.entropy.needed", &needed, &len, NULL, 0))
		return 0;

	if (needed < 0)
		return 0;

	return needed;
}

static void
entropy_write_to_kernel(const uint8_t *data, size_t len)
{
	int fd;

	fd = open(_PATH_RANDOM, O_RDWR, 0);
	if (fd >= 0) {
		write(fd, data, len);
		close(fd);
	}
}

static void
entropy_add_manual(void)
{
	SHA256_CTX ctx;
	char buf[256];
	uint8_t digest[SHA256_DIGEST_LENGTH];
	static const char prompt[] = "> ";
	size_t l;
	int txt_y;

	msg_display(MSG_entropy_enter_manual1);
	msg_printf("\n\n");
	msg_display_add(MSG_entropy_enter_manual2);
	msg_printf("\n\n   dd if=/dev/random bs=32 count=1 | openssl base64\n\n");
	msg_display_add(MSG_entropy_enter_manual3);
	msg_printf("\n\n");
	SHA256_Init(&ctx);
	txt_y = getcury(mainwin)+1;

	echo();
	wmove(mainwin, txt_y, 0);
	msg_fmt_table_add(prompt, prompt);
	mvwgetnstr(mainwin, txt_y, 2, buf, sizeof buf);
	l = strlen(buf);
	if (l > 0)
		SHA256_Update(&ctx, (const uint8_t*)buf, l);
	noecho();
	SHA256_Final(digest, &ctx);

	wmove(mainwin, txt_y-1, 0);
	wclrtobot(mainwin);
	wrefresh(mainwin);

	entropy_write_to_kernel(digest, sizeof digest);
}

/*
 * Get a file by some means and return a (potentially only
 * temporary valid) path to the local copy.
 * If mountpt is nonempty, the caller should unmount that
 * directory after processing the file.
 * Return success if the file is available, or failure if
 * the user cancelled the request or network transfer failed.
 */
static bool
entropy_get_file(bool use_netbsd_seed, char *path)
{
	static struct ftpinfo server = { .user = "ftp" };
	char url[STRSIZE], tmpf[PATH_MAX], mountpt[PATH_MAX];
	const char *ftp_opt;
	arg_rv arg;
	int rv = 0;
	const char *file_desc = msg_string(use_netbsd_seed ?
	    MSG_entropy_seed : MSG_entropy_data);
	char *dir;

	path[0] = 0;
	mountpt[0] = 0;

	sprintf(tmpf, "/tmp/entr.%06x", getpid());

	msg_display(use_netbsd_seed ?
	    MSG_entropy_seed_hdr : MSG_entropy_data_hdr);
	msg_printf("\n\n    %s\n\n",
	    use_netbsd_seed ?
	    "rndctl -S /tmp/entropy-file" :
	    "dd if=/dev/random bs=32 count=1 of=/tmp/random.tmp");
	strcpy(entropy_file, use_netbsd_seed ?
	    "entropy-file" : "random.tmp");
	process_menu(MENU_entropy_select_file, &rv);
	switch (rv) {
	case 1:
	case 2:
#ifndef DEBUG
		if (!network_up)
			config_network(0);
#endif
		server.xfer = rv == 1 ? XFER_HTTP : XFER_FTP;
		arg.arg = &server;
		arg.rv = -1;
		msg_display_add_subst(MSG_entropy_via_download, 1, file_desc);
		msg_printf("\n\n");
		process_menu(MENU_entropy_ftpsource, &arg);
		if (arg.rv == SET_RETRY)
			return false;
		make_url(url, &server, entropy_file);
		if (server.xfer == XFER_FTP &&
		    strcmp("ftp", server.user) == 0 && server.pass[0] == 0) {
			/* do anon ftp */
			ftp_opt = "-a ";
		} else {
			ftp_opt = "";
		}
		rv = run_program(RUN_DISPLAY | RUN_PROGRESS,
		    "/usr/bin/ftp %s -o %s %s",
		    ftp_opt, tmpf, url);
		strcpy(path, tmpf);
		return rv == 0;
	case 3:
#ifndef DEBUG
		if (!network_up)
			config_network(0);
#endif
		rv = -1;
		msg_display_add_subst(MSG_entropy_via_nfs, 1, file_desc);
		msg_printf("\n\n");
		process_menu(MENU_entropy_nfssource, &rv);
		if (rv == SET_RETRY)
			return false;
		if (nfs_host[0] != 0 && nfs_dir[0] != 0 &&
		    entropy_file[0] != 0) {
			strcpy(mountpt, "/tmp/ent-mnt.XXXXXX");
			dir = mkdtemp(mountpt);
			if (dir == NULL)
				return false;
			sprintf(path, "%s/%s", mountpt, entropy_file);
			if (run_program(RUN_SILENT,
			    "mount -t nfs -r %s:/%s %s",
			    nfs_host, nfs_dir, mountpt) == 0) {
				run_program(RUN_SILENT,
				    "cp %s %s", path, tmpf);
				run_program(RUN_SILENT,
				    "umount %s", mountpt);
				rmdir(mountpt);
				strcpy(path, tmpf);
			}
		}
		break;
	case 4:
		rv = -1;
		/* Get device, filesystem, and filepath */
		process_menu (MENU_entropy_localfs, &rv);
		if (rv == SET_RETRY)
			return false;
		if (localfs_dev[0] != 0 && localfs_fs[0] != 0 &&
		    entropy_file[0] != 0) {
			strcpy(mountpt, "/tmp/ent-mnt.XXXXXX");
			dir = mkdtemp(mountpt);
			if (dir == NULL)
				return false;
			sprintf(path, "%s/%s", mountpt, entropy_file);
			if (run_program(RUN_SILENT,
			    "mount -t %s -r /dev/%s %s",
			    localfs_fs, localfs_dev, mountpt) == 0) {
				run_program(RUN_SILENT,
				    "cp %s %s", path, tmpf);
				run_program(RUN_SILENT,
				    "umount %s", mountpt);
				rmdir(mountpt);
				strcpy(path, tmpf);
			}
		}
		break;
	}
	return path[0] != 0;
}

static void
entropy_add_bin_file(void)
{
	char fname[PATH_MAX];

	if (!entropy_get_file(false, fname))
		return;
	if (access(fname, R_OK) == 0)
		run_program(RUN_SILENT, "dd if=%s of=" _PATH_RANDOM,
		    fname);
}

static void
entropy_add_seed(void)
{
	char fname[PATH_MAX];

	if (!entropy_get_file(true, fname))
		return;
	if (access(fname, R_OK) == 0)
		run_program(RUN_SILENT, "rndctl -L %s", fname);
}

/*
 * return true if we have enough entropy
 */
bool
do_add_entropy(void)
{
	int rv;

	if (entropy_needed() == 0)
		return true;

	for (;;) {
		if (entropy_needed() == 0)
			break;

		msg_clear();
		rv = 0;
		process_menu(MENU_not_enough_entropy, &rv);
		switch (rv) {
		case 0:
			return false;
		case 1:
			entropy_add_manual();
			break;
		case 2:
			entropy_add_seed();
			break;
		case 3:
			entropy_add_bin_file();
			break;
		default:
			/*
			 * retry after small delay to give a new USB device
			 * a chance to attach and do deliver some
			 * entropy
			 */
			msg_display(".");
			for (size_t i = 0; i < 10; i++) {
				if (entropy_needed() == 0)
					return true;
				sleep(1);
				msg_display_add(".");
			}
		}
	}

	/*
	 * Save entropy (maybe again) to give the seed file a good
	 * entropy estimate.
	 */
	run_program(RUN_SILENT | RUN_CHROOT | RUN_ERROR_OK,
	    "/etc/rc.d/random_seed stop");

	return true;
}
#endif



/*
 * Get and unpack the distribution.
 * Show success_msg if installation completes.
 * Otherwise show failure_msg and wait for the user to ack it before continuing.
 * success_msg and failure_msg must both be 0-adic messages.
 */
int
get_and_unpack_sets(int update, msg setupdone_msg, msg success_msg, msg failure_msg)
{
	distinfo *dist;
	int status;
	int set, olderror, oldfound;
	bool entropy_loaded = false;

	/* Ensure mountpoint for distribution files exists in current root. */
	(void)mkdir("/mnt2", S_IRWXU| S_IRGRP|S_IXGRP | S_IROTH|S_IXOTH);
	if (script)
		(void)fprintf(script, "mkdir -m 755 /mnt2\n");

	/* reset failure/success counters */
	memset(&tarstats, 0, sizeof(tarstats));

	/* Find out which files to "get" if we get files. */

	/* Accurately count selected sets */
	for (dist = dist_list; (set = dist->set) != SET_LAST; dist++) {
		if (dist->name == NULL)
			continue;
		if (set_status[set] & SET_NO_EXTRACT)
			continue;
		if ((set_status[set] & (SET_VALID | SET_SELECTED))
		    == (SET_VALID | SET_SELECTED))
			tarstats.nselected++;
	}

	status = SET_RETRY;
	for (dist = dist_list; (set = dist->set) != SET_LAST; dist++) {
		if (dist->name == NULL)
			continue;
		if (set_status[set] != (SET_VALID | SET_SELECTED))
			continue;

		/* save stats, in case we will retry */
		oldfound = tarstats.nfound;
		olderror = tarstats.nerror;

		if (status != SET_OK) {
			/* This might force a redraw.... */
			clearok(curscr, 1);
			touchwin(stdscr);
			wrefresh(stdscr);
			/* Sort out the location of the set files */
			do {
				umount_mnt2();
				msg_fmt_display(MSG_distmedium, "%d%d%s",
				    tarstats.nselected,
				    tarstats.nsuccess + tarstats.nskipped,
				    dist->name);
				fetch_fn = NULL;
				process_menu(MENU_distmedium, &status);
			} while (status == SET_RETRY);

			if (status == SET_SKIP) {
				set_status[set] |= SET_SKIPPED;
				tarstats.nskipped++;
				continue;
			}
			if (status == SET_SKIP_GROUP) {
				skip_set(dist, status);
				continue;
			}
			if (status != SET_OK) {
				hit_enter_to_continue(failure_msg, NULL);
				return 1;
			}
		}

		if (set_status[set] & SET_NO_EXTRACT)
			continue;

		/* Try to extract this set */
		status = extract_file(dist, update);
		if (status == SET_RETRY) {
			/* do this set again */
			dist--;
			/* and reset statistics to what we had before this
			 * set */
			tarstats.nfound = oldfound;
			tarstats.nerror = olderror;
		}
	}

#ifdef MD_SET_EXTRACT_FINALIZE
	MD_SET_EXTRACT_FINALIZE(update);
#endif

	if (tarstats.nerror == 0 && tarstats.nsuccess == tarstats.nselected) {
		msg_display(MSG_endtarok);
		/* Give user a chance to see the success message */
		sleep(1);
	} else {
		/* We encountered errors. Let the user know. */
		msg_fmt_display(MSG_endtar, "%d%d%d%d%d%d",
		    tarstats.nselected, tarstats.nnotfound, tarstats.nskipped,
		    tarstats.nfound, tarstats.nsuccess, tarstats.nerror);
		hit_enter_to_continue(NULL, NULL);
	}

	/*
	 * postinstall needs to be run after extracting all sets, because
	 * otherwise /var/db/obsolete will only have current information
	 * from the base, comp, and etc sets.
	 */
	if (update && (set_status[SET_ETC] & SET_INSTALLED)) {
		int oldsendmail;
		oldsendmail = run_program(RUN_DISPLAY | RUN_CHROOT |
					  RUN_ERROR_OK | RUN_PROGRESS,
					  "/usr/sbin/postinstall -s /.sysinst -d / check mailerconf");
		if (oldsendmail == 1) {
			msg_display(MSG_oldsendmail);
			if (ask_yesno(NULL)) {
				run_program(RUN_DISPLAY | RUN_CHROOT,
					    "/usr/sbin/postinstall -s /.sysinst -d / fix mailerconf");
			}
		}
		run_program(RUN_DISPLAY | RUN_CHROOT,
			"/usr/sbin/postinstall -s /.sysinst -d / fix");

		/* Don't discard the system's old entropy if any */
		run_program(RUN_CHROOT | RUN_SILENT,
		    "/etc/rc.d/random_seed start");
		entropy_loaded = true;
	}

	/* Configure the system */
	if (set_status[SET_BASE] & SET_INSTALLED)
		run_makedev();

	if (!update) {
		struct stat sb1, sb2;

		if (stat(target_expand("/"), &sb1) == 0
		    && stat(target_expand("/var"), &sb2) == 0
		    && sb1.st_dev != sb2.st_dev) {
			add_rc_conf("random_file=/etc/entropy-file\n");
			if (target_file_exists_p("/boot.cfg")) {
				run_program(RUN_CHROOT|RUN_FATAL,
					    "sh -c 'sed -e s./var/db/./etc/. "
					    "< /boot.cfg "
					    "> /tmp/boot.cfg.tmp'");
				mv_within_target_or_die("/tmp/boot.cfg.tmp",
							"/boot.cfg");

			}
		}

#ifdef MD_BOOT_CFG_FINALIZE
		if (target_file_exists_p("/boot.cfg")) {
			MD_BOOT_CFG_FINALIZE("/boot.cfg");
		}
#endif

		/* Save keyboard type */
		save_kb_encoding();

		/* Other configuration. */
		mnt_net_config();
	}

	/* Mounted dist dir? */
	umount_mnt2();

#ifdef CHECK_ENTROPY
	entropy_loaded |= entropy_needed() == 0;
#endif

	/* Save entropy -- on some systems it's ~all we'll ever get */
	if (!update || entropy_loaded)
		run_program(RUN_SILENT | RUN_CHROOT | RUN_ERROR_OK,
		    "/etc/rc.d/random_seed stop");
	/* Install/Upgrade complete ... reboot or exit to script */
	hit_enter_to_continue(success_msg, NULL);
	return 0;
}

void
umount_mnt2(void)
{
	if (!mnt2_mounted)
		return;
	run_program(RUN_SILENT, "/sbin/umount /mnt2");
	mnt2_mounted = 0;
}


/*
 * Do a quick sanity check that  the target can reboot.
 * return 1 if everything OK, 0 if there is a problem.
 * Uses a table of files we expect to find after a base install/upgrade.
 */

/* test flag and pathname to check for after unpacking. */
struct check_table { unsigned int mode; const char *path;} checks[] = {
  { S_IFREG, "/netbsd" },
  { S_IFDIR, "/etc" },
  { S_IFREG, "/etc/fstab" },
  { S_IFREG, "/sbin/init" },
  { S_IFREG, "/bin/sh" },
  { S_IFREG, "/etc/rc" },
  { S_IFREG, "/etc/rc.subr" },
  { S_IFREG, "/etc/rc.conf" },
  { S_IFDIR, "/dev" },
  { S_IFCHR, "/dev/console" },
/* XXX check for rootdev in target /dev? */
  { S_IFREG, "/sbin/fsck" },
  { S_IFREG, "/sbin/fsck_ffs" },
  { S_IFREG, "/sbin/mount" },
  { S_IFREG, "/sbin/mount_ffs" },
  { S_IFREG, "/sbin/mount_nfs" },
#if defined(DEBUG) || defined(DEBUG_CHECK)
  { S_IFREG, "/foo/bar" },		/* bad entry to exercise warning */
#endif
  { 0, 0 }

};

/*
 * Check target for a single file.
 */
static int
check_for(unsigned int mode, const char *pathname)
{
	int found;

	found = (target_test(mode, pathname) == 0);
	if (found == 0)
		msg_fmt_display(MSG_rootmissing, "%s", pathname);
	return found;
}

/*
 * Check that all the files in check_table are present in the
 * target root. Warn if not found.
 */
int
sanity_check(void)
{
	int target_ok = 1;
	struct check_table *p;

	for (p = checks; p->path; p++) {
		target_ok = target_ok && check_for(p->mode, p->path);
	}
	if (target_ok)
		return 0;

	/* Uh, oh. Something's missing. */
	hit_enter_to_continue(MSG_badroot, NULL);
	return 1;
}

/*
 * Some globals to pass things back from callbacks
 */
static char zoneinfo_dir[STRSIZE];
static int zonerootlen;
static char *tz_selected;	/* timezonename (relative to share/zoneinfo */
const char *tz_default;		/* UTC, or whatever /etc/localtime points to */
static char tz_env[STRSIZE];
static int save_cursel, save_topline;
static int time_menu = -1;

static void
update_time_display(void)
{
	time_t t;
	struct tm *tm;
	char cur_time[STRSIZE], *p;

	t = time(NULL);
	tm = localtime(&t);
	strlcpy(cur_time, safectime(&t), sizeof cur_time);
	p = strchr(cur_time, '\n');
	if (p != NULL)
		*p = 0;

	msg_clear();
	msg_fmt_table_add(MSG_choose_timezone, "%s%s%s%s",
	    tz_default, tz_selected, cur_time, tm ? tm->tm_zone : "?");
}

/*
 * Callback from timezone menu
 */
static int
set_tz_select(menudesc *m, void *arg)
{
	char *new;

	if (m && strcmp(tz_selected, m->opts[m->cursel].opt_name) != 0) {
		/* Change the displayed timezone */
		new = strdup(m->opts[m->cursel].opt_name);
		if (new == NULL)
			return 0;
		free(tz_selected);
		tz_selected = new;
		snprintf(tz_env, sizeof tz_env, "%.*s%s",
			 zonerootlen, zoneinfo_dir, tz_selected);
		setenv("TZ", tz_env, 1);
	}
	if (m)
		/* Warp curser to 'Exit' line on menu */
		m->cursel = -1;

	update_time_display();
	if (time_menu >= 1) {
		WINDOW *w = get_menudesc(time_menu)->mw;
		if (w != NULL) {
			touchwin(w);
			wrefresh(w);
		}
	}
	return 0;
}

static int
set_tz_back(menudesc *m, void *arg)
{

	zoneinfo_dir[zonerootlen] = 0;
	m->cursel = save_cursel;
	m->topline = save_topline;
	return 0;
}

static int
set_tz_dir(menudesc *m, void *arg)
{

	strlcpy(zoneinfo_dir + zonerootlen, m->opts[m->cursel].opt_name,
		sizeof zoneinfo_dir - zonerootlen);
	save_cursel = m->cursel;
	save_topline = m->topline;
	m->cursel = 0;
	m->topline = 0;
	return 0;
}

/*
 * Alarm-handler to update example-display
 */
static void
/*ARGSUSED*/
timezone_sig(int sig)
{

	set_tz_select(NULL, NULL);
	alarm(60);
}

static int
tz_sort(const void *a, const void *b)
{
	return strcmp(((const menu_ent *)a)->opt_name, ((const menu_ent *)b)->opt_name);
}

static void
tzm_set_names(menudesc *m, void *arg)
{
	DIR *dir;
	struct dirent *dp;
	static int nfiles;
	static int maxfiles = 32;
	static menu_ent *tz_menu;
	static char **tz_names;
	void *p;
	int maxfname;
	char *fp;
	struct stat sb;

	if (tz_menu == NULL)
		tz_menu = calloc(maxfiles, sizeof *tz_menu);
	if (tz_names == NULL)
		tz_names = malloc(maxfiles * sizeof *tz_names);
	if (tz_menu == NULL || tz_names == NULL)
		return;	/* error - skip timezone setting */
	while (nfiles > 0)
		free(tz_names[--nfiles]);

	dir = opendir(zoneinfo_dir);
	fp = strchr(zoneinfo_dir, 0);
	if (fp != zoneinfo_dir + zonerootlen) {
		tz_names[0] = 0;
		tz_menu[0].opt_name = msg_string(MSG_tz_back);
		tz_menu[0].opt_action = set_tz_back;
		nfiles = 1;
	}
	maxfname = zoneinfo_dir + sizeof zoneinfo_dir - fp - 1;
	if (dir != NULL) {
		while ((dp = readdir(dir)) != NULL) {
			if (dp->d_namlen > maxfname || dp->d_name[0] == '.')
				continue;
			strlcpy(fp, dp->d_name, maxfname);
			if (stat(zoneinfo_dir, &sb) == -1)
				continue;
			if (nfiles >= maxfiles) {
				p = realloc(tz_menu,
				    2 * maxfiles * sizeof *tz_menu);
				if (p == NULL)
					break;
				tz_menu = p;
				memset(tz_menu + maxfiles, 0,
				    maxfiles * sizeof *tz_menu);
				p = realloc(tz_names,
				    2 * maxfiles * sizeof *tz_names);
				if (p == NULL)
					break;
				tz_names = p;
				memset(tz_names + maxfiles, 0,
				    maxfiles * sizeof *tz_names);
				maxfiles *= 2;
			}
			if (S_ISREG(sb.st_mode))
				tz_menu[nfiles].opt_action = set_tz_select;
			else if (S_ISDIR(sb.st_mode)) {
				tz_menu[nfiles].opt_action = set_tz_dir;
				strlcat(fp, "/",
				    sizeof(zoneinfo_dir) - (fp - zoneinfo_dir));
			} else
				continue;
			tz_names[nfiles] = strdup(zoneinfo_dir + zonerootlen);
			tz_menu[nfiles].opt_name = tz_names[nfiles];
			nfiles++;
		}
		closedir(dir);
	}
	*fp = 0;

	m->opts = tz_menu;
	m->numopts = nfiles;
	qsort(tz_menu, nfiles, sizeof *tz_menu, tz_sort);
}

void
get_tz_default(void)
{
	char localtime_link[STRSIZE];
	static char localtime_target[STRSIZE];
	int rc;

	strlcpy(localtime_link, target_expand("/etc/localtime"),
	    sizeof localtime_link);

	/* Add sanity check that /mnt/usr/share/zoneinfo contains
	 * something useful
	 */

	rc = readlink(localtime_link, localtime_target,
		      sizeof(localtime_target) - 1);
	if (rc < 0) {
		/* error, default to UTC */
		tz_default = "UTC";
	} else {
		localtime_target[rc] = '\0';
		tz_default = strchr(strstr(localtime_target, "zoneinfo"), '/') + 1;
	}
}

/*
 * Choose from the files in usr/share/zoneinfo and set etc/localtime
 */
int
set_timezone(void)
{
	char localtime_link[STRSIZE];
	char localtime_target[STRSIZE];
	int menu_no;

	strlcpy(zoneinfo_dir, target_expand("/usr/share/zoneinfo/"),
	    sizeof zoneinfo_dir - 1);
	zonerootlen = strlen(zoneinfo_dir);

	get_tz_default();

	tz_selected = strdup(tz_default);
	snprintf(tz_env, sizeof(tz_env), "%s%s", zoneinfo_dir, tz_selected);
	setenv("TZ", tz_env, 1);
	update_time_display();

	signal(SIGALRM, timezone_sig);
	alarm(60);

	menu_no = new_menu(NULL, NULL, 14, 23, 9,
			   12, 32, MC_ALWAYS_SCROLL | MC_NOSHORTCUT,
			   tzm_set_names, NULL, NULL,
			   "\nPlease consult the install documents.",
			   MSG_exit_menu_generic);
	if (menu_no >= 0) {
		time_menu = menu_no;
		process_menu(menu_no, NULL);
		time_menu = -1;

		free_menu(menu_no);
	}

	alarm(0);
	signal(SIGALRM, SIG_IGN);

	if (menu_no >= 0) {
		snprintf(localtime_target, sizeof(localtime_target),
			 "/usr/share/zoneinfo/%s", tz_selected);
		strlcpy(localtime_link, target_expand("/etc/localtime"),
		    sizeof localtime_link);
		unlink(localtime_link);
		symlink(localtime_target, localtime_link);
	}

	return 1;
}

void
scripting_vfprintf(FILE *f, const char *fmt, va_list ap)
{
	va_list ap2;

	va_copy(ap2, ap);
	if (f)
		(void)vfprintf(f, fmt, ap);
	if (script)
		(void)vfprintf(script, fmt, ap2);
}

void
scripting_fprintf(FILE *f, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	scripting_vfprintf(f, fmt, ap);
	va_end(ap);
}

void
add_rc_conf(const char *fmt, ...)
{
	FILE *f;
	va_list ap;

	va_start(ap, fmt);
	f = target_fopen("/etc/rc.conf", "a");
	if (f != 0) {
		scripting_fprintf(NULL, "cat <<EOF >>%s/etc/rc.conf\n",
		    target_prefix());
		scripting_vfprintf(f, fmt, ap);
		fclose(f);
		scripting_fprintf(NULL, "EOF\n");
	}
	va_end(ap);
}

int
del_rc_conf(const char *value)
{
	FILE *fp, *nfp;
	char buf[4096]; /* Ridiculously high, but should be enough in any way */
	char *rcconf, *tempname = NULL, *bakname = NULL;
	char *cp;
	int done = 0;
	int fd;
	int retval = 0;

	/* The paths might seem strange, but using /tmp would require copy instead
	 * of rename operations. */
	if (asprintf(&rcconf, "%s", target_expand("/etc/rc.conf")) < 0
			|| asprintf(&tempname, "%s", target_expand("/etc/rc.conf.tmp.XXXXXX")) < 0
			|| asprintf(&bakname, "%s", target_expand("/etc/rc.conf.bak.XXXXXX")) < 0) {
		if (rcconf)
			free(rcconf);
		if (tempname)
			free(tempname);
		msg_fmt_display(MSG_rcconf_delete_failed, "%s", value);
		hit_enter_to_continue(NULL, NULL);
		return -1;
	}

	if ((fd = mkstemp(bakname)) < 0) {
		msg_fmt_display(MSG_rcconf_delete_failed, "%s", value);
		hit_enter_to_continue(NULL, NULL);
		return -1;
	}
	close(fd);

	if (!(fp = fopen(rcconf, "r+")) || (fd = mkstemp(tempname)) < 0) {
		if (fp)
			fclose(fp);
		msg_fmt_display(MSG_rcconf_delete_failed, "%s", value);
		hit_enter_to_continue(NULL, NULL);
		return -1;
	}

	nfp = fdopen(fd, "w");
	if (!nfp) {
		fclose(fp);
		close(fd);
		msg_fmt_display(MSG_rcconf_delete_failed, "%s", value);
		hit_enter_to_continue(NULL, NULL);
		return -1;
	}

	while (fgets(buf, sizeof buf, fp) != NULL) {

		cp = buf + strspn(buf, " \t"); /* Skip initial spaces */
		if (strncmp(cp, value, strlen(value)) == 0) {
			cp += strlen(value);
			if (*cp != '=')
				scripting_fprintf(nfp, "%s", buf);
			else
				done = 1;
		} else {
			scripting_fprintf(nfp, "%s", buf);
		}
	}
	fclose(fp);
	fclose(nfp);

	if (done) {
		if (rename(rcconf, bakname)) {
			msg_display(MSG_rcconf_backup_failed);
			if (!ask_noyes(NULL)) {
				retval = -1;
				goto done;
			}
		}

		if (rename(tempname, rcconf)) {
			if (rename(bakname, rcconf)) {
				hit_enter_to_continue(MSG_rcconf_restore_failed,
				    NULL);
			} else {
				hit_enter_to_continue(MSG_rcconf_delete_failed,
				    NULL);
			}
		} else {
			(void)unlink(bakname);
		}
	}

done:
	(void)unlink(tempname);
	free(rcconf);
	free(tempname);
	free(bakname);
	return retval;
}

void
add_sysctl_conf(const char *fmt, ...)
{
	FILE *f;
	va_list ap;

	va_start(ap, fmt);
	f = target_fopen("/etc/sysctl.conf", "a");
	if (f != 0) {
		scripting_fprintf(NULL, "cat <<EOF >>%s/etc/sysctl.conf\n",
		    target_prefix());
		scripting_vfprintf(f, fmt, ap);
		fclose(f);
		scripting_fprintf(NULL, "EOF\n");
	}
	va_end(ap);
}

void
enable_rc_conf(void)
{

	replace("/etc/rc.conf", "s/^rc_configured=NO/rc_configured=YES/");
}

int
check_lfs_progs(void)
{

#ifndef NO_LFS
	return binary_available("fsck_lfs") && binary_available("mount_lfs")
	    && binary_available("newfs_lfs");
#else
	return 0;
#endif
}

int
set_is_source(const char *set_name) {
	int len = strlen(set_name);
	return len >= 3 && memcmp(set_name + len - 3, "src", 3) == 0;
}

const char *
set_dir_for_set(const char *set_name) {
	if (strcmp(set_name, "pkgsrc") == 0)
		return pkgsrc_dir;
	return set_is_source(set_name) ? set_dir_src : set_dir_bin;
}

const char *
ext_dir_for_set(const char *set_name) {
	if (strcmp(set_name, "pkgsrc") == 0)
		return ext_dir_pkgsrc;
	return set_is_source(set_name) ? ext_dir_src : ext_dir_bin;
}

void
do_coloring(unsigned int fg, unsigned int bg) {
	if (bg > COLOR_WHITE)
		bg = COLOR_BLUE;
	if (fg > COLOR_WHITE)
		fg = COLOR_WHITE;
	if (fg != bg && has_colors()) {
		init_pair(1, fg, bg);
		wbkgd(stdscr, COLOR_PAIR(1));
		wbkgd(mainwin, COLOR_PAIR(1));
		wattrset(stdscr, COLOR_PAIR(1));
		wattrset(mainwin, COLOR_PAIR(1));
	}
	/* redraw screen */
	touchwin(stdscr);
	touchwin(mainwin);
	wrefresh(stdscr);
	wrefresh(mainwin);
	return;
}

int
set_menu_select(menudesc *m, void *arg)
{
	*(int *)arg = m->cursel;
	return 1;
}

/*
 * check wether a binary is available somewhere in PATH,
 * return 1 if found, 0 if not.
 */
static int
binary_available(const char *prog)
{
        char *p, tmp[MAXPATHLEN], *path = getenv("PATH"), *opath;

        if (path == NULL)
                return access(prog, X_OK) == 0;
        path = strdup(path);
        if (path == NULL)
                return 0;

	opath = path;

        while ((p = strsep(&path, ":")) != NULL) {
                if (strlcpy(tmp, p, MAXPATHLEN) >= MAXPATHLEN)
                        continue;
                if (strlcat(tmp, "/", MAXPATHLEN) >= MAXPATHLEN)
                        continue;
                if (strlcat(tmp, prog, MAXPATHLEN) >= MAXPATHLEN)
                        continue;
                if (access(tmp, X_OK) == 0) {
                        free(opath);
                        return 1;
                }
        }
        free(opath);
        return 0;
}

const char *
safectime(time_t *t)
{
	const char *s = ctime(t);
	if (s != NULL)
		return s;

	// Debugging.
	fprintf(stderr, "Can't convert to localtime 0x%jx (%s)\n",
	    (intmax_t)*t, strerror(errno));
	      /*123456789012345678901234*/
	return "preposterous clock time\n";
}

int
ask_yesno(const char* msgtxt)
{
	arg_rv p;

	p.arg = __UNCONST(msgtxt);
	p.rv = -1;

	process_menu(MENU_yesno, &p);
	return p.rv;
}

int
ask_noyes(const char *msgtxt)
{
	arg_rv p;

	p.arg = __UNCONST(msgtxt);
	p.rv = -1;

	process_menu(MENU_noyes, &p);
	return p.rv;
}

int
ask_reedit(const struct disk_partitions *parts)
{
	const char *args[2];
	arg_rep_int arg;

	args[0] = msg_string(parts->pscheme->name);
	args[1] = msg_string(parts->pscheme->short_name);
	arg.args.argv = args;
	arg.args.argc = 2;
	arg.rv = 0;
	process_menu(MENU_reedit, &arg);

	return arg.rv;
}

bool
use_tgz_for_set(const char *set_name)
{
	const struct distinfo *dist;

	for (dist = dist_list; dist->set != SET_LAST; dist++) {
		if (dist->name == NULL)
			continue;
		if (strcmp(set_name, dist->name) == 0)
			return dist->force_tgz;
	}

	return true;
}

/* Return the postfix used for a given set */
const char *
set_postfix(const char *set_name)
{
	return use_tgz_for_set(set_name) ? dist_tgz_postfix : dist_postfix;
}

/*
 * Replace positional arguments (encoded as $0 .. $N) in the string
 * passed by the contents of the passed argument array.
 * Caller must free() the result string.
 */
char*
str_arg_subst(const char *src, size_t argc, const char **argv)
{
	const char *p, *last;
	char *out, *t;
	size_t len;

	len = strlen(src);
	for (p = strchr(src, '$'); p; p = strchr(p+1, '$')) {
		char *endp = NULL;
		size_t n;
		int e;

		/* $ followed by a correct numeric position? */
		n = strtou(p+1, &endp, 10, 0, INT_MAX, &e);
		if ((e == 0 || e == ENOTSUP) && n < argc) {
			len += strlen(argv[n]);
			len -= endp-p;
			p = endp-1;
		}
	}

	out = malloc(len+1);
	if (out == NULL)
		return NULL;

	t = out;
	for (last = src, p = strchr(src, '$'); p; p = strchr(p+1, '$')) {
		char *endp = NULL;
		size_t n;
		int e;

		/* $ followed by a correct numeric position? */
		n = strtou(p+1, &endp, 10, 0, INT_MAX, &e);
		if ((e == 0 || e == ENOTSUP) && n < argc) {
			size_t l = p-last;
			memcpy(t, last, l);
			t += l;
			strcpy(t, argv[n]);
			t += strlen(argv[n]);
			last = endp;
		}
	}
	if (*last) {
		strcpy(t, last);
		t += strlen(last);
	} else {
		*t = 0;
	}
	assert((size_t)(t-out) == len);

	return out;
}

/*
 * Does this string have any positional args that need expanding?
 */
bool
needs_expanding(const char *src, size_t argc)
{
	const char *p;

	for (p = strchr(src, '$'); p; p = strchr(p+1, '$')) {
		char *endp = NULL;
		size_t n;
		int e;

		/* $ followed by a correct numeric position? */
		n = strtou(p+1, &endp, 10, 0, INT_MAX, &e);
		if ((e == 0 || e == ENOTSUP) && n < argc)
			return true;
	}
	return false;
}

/*
 * Replace positional arguments (encoded as $0 .. $N) in the
 * message by the strings passed as ... and call outfunc
 * with the result.
 */
static void
msg_display_subst_internal(void (*outfunc)(msg),
    const char *master, size_t argc, va_list ap)
{
	const char **args, **arg;
	char *out;

	args = calloc(argc, sizeof(*args));
	if (args == NULL)
		return;

	arg = args;
	for (size_t i = 0; i < argc; i++)
		*arg++ = va_arg(ap, const char*);

	out = str_arg_subst(msg_string(master), argc, args);
	if (out != NULL) {
		outfunc(out);
		free(out);
	}
	free(args);
}

/*
 * Replace positional arguments (encoded as $0 .. $N) in the
 * message by the strings passed as ...
 */
void
msg_display_subst(const char *master, size_t argc, ...)
{
	va_list ap;

	va_start(ap, argc);
	msg_display_subst_internal(msg_display, master, argc, ap);
	va_end(ap);
}

/*
 * Same as above, but add to message instead of starting a new one
 */
void
msg_display_add_subst(const char *master, size_t argc, ...)
{
	va_list ap;

	va_start(ap, argc);
	msg_display_subst_internal(msg_display_add, master, argc, ap);
	va_end(ap);
}

/* initialize have_* variables */
void
check_available_binaries()
{
	static int did_test = false;

	if (did_test) return;
	did_test = 1;

	have_raid = binary_available("raidctl");
	have_vnd = binary_available("vndconfig");
	have_cgd = binary_available("cgdconfig");
	have_lvm = binary_available("lvm");
	have_gpt = binary_available("gpt");
	have_dk = binary_available("dkctl");
}

/*
 * Wait for enter and immediately clear the screen after user response
 * (in case some longer action follows, so the user has instant feedback)
 */
void
hit_enter_to_continue(const char *prompt, const char *title)
{
	if (prompt != NULL)
		msg_display(prompt);
        process_menu(MENU_ok, __UNCONST(title));
	msg_clear();
	wrefresh(mainwin);
}

/*
 * On auto pilot:
 * convert an existing set of partitions ot a list of part_usage_info
 * so that we "want" exactly what is there already.
 */
static bool
usage_info_list_from_parts(struct part_usage_info **list, size_t *count,
    struct disk_partitions *parts)
{
	struct disk_part_info info;
	part_id pno;
	size_t no, num;

	num = 0;
	for (pno = 0; pno < parts->num_part; pno++) {
		if (!parts->pscheme->get_part_info(parts, pno, &info))
			continue;
		num++;
	}

	*list = calloc(num, sizeof(**list));
	if (*list == NULL)
		return false;

	*count = num;
	for (no = pno = 0; pno < parts->num_part && no < num; pno++) {
		if (!parts->pscheme->get_part_info(parts, pno, &info))
			continue;
		(*list)[no].size = info.size;
		if (info.last_mounted != NULL && *info.last_mounted != 0) {
			strlcpy((*list)[no].mount, info.last_mounted,
			    sizeof((*list)[no].mount));
			(*list)[no].instflags |= PUIINST_MOUNT;
		}
		(*list)[no].parts = parts;
		(*list)[no].cur_part_id = pno;
		(*list)[no].cur_start = info.start;
		(*list)[no].cur_flags = info.flags;
		(*list)[no].type = info.nat_type->generic_ptype;
		if ((*list)[no].type == PT_swap) {
			(*list)[no].fs_type = FS_SWAP;
			(*list)[no].fs_version = 0;
		} else {
			(*list)[no].fs_type = info.fs_type;
			(*list)[no].fs_version = info.fs_sub_type;
		}
		(*list)[no].fs_opt1 = info.fs_opt1;
		(*list)[no].fs_opt2 = info.fs_opt2;
		(*list)[no].fs_opt3 = info.fs_opt3;
		no++;
	}
	return true;
}

bool
usage_set_from_parts(struct partition_usage_set *wanted,
    struct disk_partitions *parts)
{
	memset(wanted, 0, sizeof(*wanted));
	wanted->parts = parts;

	return usage_info_list_from_parts(&wanted->infos, &wanted->num, parts);
}

struct disk_partitions *
get_inner_parts(struct disk_partitions *parts)
{
	daddr_t start, size;
	part_id pno;
	struct disk_part_info info;

	if (parts->pscheme->secondary_scheme == NULL)
		return NULL;

	start = -1;
	size = -1;
	if (parts->pscheme->guess_install_target == NULL ||
	    !parts->pscheme->guess_install_target(parts, &start, &size)) {
		for (pno = 0; pno < parts->num_part; pno++) {
			if (!parts->pscheme->get_part_info(parts, pno, &info))
				continue;
			if (!(info.flags & PTI_SEC_CONTAINER))
				continue;
			start = info.start;
			size = info.size;
		}
	}

	if (size > 0)
		return parts->pscheme->secondary_partitions(parts, start,
		    false);

	return NULL;
}

bool
install_desc_from_parts(struct install_partition_desc *install,
    struct disk_partitions *parts)
{
	struct disk_partitions *inner_parts;

	memset(install, 0, sizeof(*install));
	inner_parts = get_inner_parts(parts);
	if (inner_parts != NULL)
		parts = inner_parts;

	return usage_info_list_from_parts(&install->infos, &install->num,
	    parts);
}

void
free_usage_set(struct partition_usage_set *wanted)
{
	/* XXX - free parts? free clone src? */
	free(wanted->write_back);
	free(wanted->menu_opts);
	free(wanted->infos);
}

void
free_install_desc(struct install_partition_desc *install)
{
	size_t i, j;

#ifndef NO_CLONES
	for (i = 0; i < install->num; i++) {
		struct selected_partitions *src = install->infos[i].clone_src;
		if (!(install->infos[i].flags & PUIFLG_CLONE_PARTS) ||
		    src == NULL)
			continue;
		free_selected_partitions(src);
		for (j = i+1; j < install->num; j++)
			if (install->infos[j].clone_src == src)
				install->infos[j].clone_src = NULL;
	}
#endif

	for (i = 0; i < install->num; i++) {
		struct disk_partitions * parts = install->infos[i].parts;

		if (parts == NULL)
			continue;

		if (parts->pscheme->free)
			parts->pscheme->free(parts);

		/* NULL all other references to this parts */
		for (j = i+1; j < install->num; j++)
			if (install->infos[j].parts == parts)
				install->infos[j].parts = NULL;
	}

	free(install->write_back);
	free(install->infos);
}

#ifdef MD_MAY_SWAP_TO
bool
may_swap_if_not_sdmmc(const char *disk)
{
	int fd, res;
	prop_dictionary_t command_dict, args_dict, results_dict, data_dict;
	prop_string_t string;
	prop_number_t number;
	const char *parent = "";

	fd = open(DRVCTLDEV, O_RDONLY, 0);
	if (fd == -1)
		return true;

	command_dict = prop_dictionary_create();
	args_dict = prop_dictionary_create();

	string = prop_string_create_nocopy("get-properties");
	prop_dictionary_set(command_dict, "drvctl-command", string);
	prop_object_release(string);

	string = prop_string_create_copy(disk);
	prop_dictionary_set(args_dict, "device-name", string);
	prop_object_release(string);

	prop_dictionary_set(command_dict, "drvctl-arguments",
	    args_dict);
	prop_object_release(args_dict);

	res = prop_dictionary_sendrecv_ioctl(command_dict, fd,
	    DRVCTLCOMMAND, &results_dict);
	prop_object_release(command_dict);
	close(fd);
	if (res)
		return true;

	number = prop_dictionary_get(results_dict, "drvctl-error");
	if (prop_number_signed_value(number) == 0) {
		data_dict = prop_dictionary_get(results_dict,
		    "drvctl-result-data");
		if (data_dict != NULL) {
			string = prop_dictionary_get(data_dict,
			    "device-parent");
			if (string != NULL)
				parent = prop_string_value(string);
		}
	}

	prop_object_release(results_dict);

	if (parent == NULL)
		return true;

	return strncmp(parent, "sdmmc", 5) != 0;
}
#endif
