/*	$NetBSD: lvm-functions.c,v 1.1.1.1.2.1 2009/05/13 18:52:41 jym Exp $	*/

/*
 * Copyright (C) 2002-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2008 Red Hat, Inc. All rights reserved.
 *
 * This file is part of LVM2.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU General Public License v.2.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64

#include <configure.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <assert.h>
#include <libdevmapper.h>
#include <libdlm.h>

#include "lvm-types.h"
#include "clvm.h"
#include "clvmd-comms.h"
#include "clvmd.h"
#include "lvm-functions.h"

/* LVM2 headers */
#include "toolcontext.h"
#include "lvmcache.h"
#include "lvm-logging.h"
#include "lvm-globals.h"
#include "activate.h"
#include "locking.h"
#include "archiver.h"
#include "defaults.h"

static struct cmd_context *cmd = NULL;
static struct dm_hash_table *lv_hash = NULL;
static pthread_mutex_t lv_hash_lock;
static pthread_mutex_t lvm_lock;
static char last_error[1024];
static int suspended = 0;

struct lv_info {
	int lock_id;
	int lock_mode;
};

#define LCK_MASK (LCK_TYPE_MASK | LCK_SCOPE_MASK)

static const char *decode_locking_cmd(unsigned char cmdl)
{
	static char buf[128];
	const char *type;
	const char *scope;
	const char *command;

	switch (cmdl & LCK_TYPE_MASK) {
	case LCK_NULL:   
		type = "NULL";   
		break;
	case LCK_READ:   
		type = "READ";   
		break;
	case LCK_PREAD:  
		type = "PREAD";  
		break;
	case LCK_WRITE:  
		type = "WRITE";  
		break;
	case LCK_EXCL:   
		type = "EXCL";   
		break;
	case LCK_UNLOCK: 
		type = "UNLOCK"; 
		break;
	default:
		type = "unknown";
		break;
	}

	switch (cmdl & LCK_SCOPE_MASK) {
	case LCK_VG: 
		scope = "VG"; 
		break;
	case LCK_LV: 
		scope = "LV"; 
		break;
	default:
		scope = "unknown";
		break;
	}

	switch (cmdl & LCK_MASK) {
	case LCK_LV_EXCLUSIVE & LCK_MASK:
		command = "LCK_LV_EXCLUSIVE";  
		break;
	case LCK_LV_SUSPEND & LCK_MASK:    
		command = "LCK_LV_SUSPEND";    
		break;
	case LCK_LV_RESUME & LCK_MASK:     
		command = "LCK_LV_RESUME";     
		break;
	case LCK_LV_ACTIVATE & LCK_MASK:   
		command = "LCK_LV_ACTIVATE";   
		break;
	case LCK_LV_DEACTIVATE & LCK_MASK: 
		command = "LCK_LV_DEACTIVATE"; 
		break;
	default:
		command = "unknown";
		break;
	}

	sprintf(buf, "0x%x %s (%s|%s%s%s%s%s%s)", cmdl, command, type, scope,
		cmdl & LCK_NONBLOCK   ? "|NONBLOCK" : "",
		cmdl & LCK_HOLD       ? "|HOLD" : "",
		cmdl & LCK_LOCAL      ? "|LOCAL" : "",
		cmdl & LCK_CLUSTER_VG ? "|CLUSTER_VG" : "",
		cmdl & LCK_CACHE      ? "|CACHE" : "");

	return buf;
}

static const char *decode_flags(unsigned char flags)
{
	static char buf[128];

	sprintf(buf, "0x%x (%s%s)", flags,
		flags & LCK_MIRROR_NOSYNC_MODE	  ? "MIRROR_NOSYNC " : "",
		flags & LCK_DMEVENTD_MONITOR_MODE ? "DMEVENTD_MONITOR " : "");

	return buf;
}

char *get_last_lvm_error()
{
	return last_error;
}

/* Return the mode a lock is currently held at (or -1 if not held) */
static int get_current_lock(char *resource)
{
	struct lv_info *lvi;

	pthread_mutex_lock(&lv_hash_lock);
	lvi = dm_hash_lookup(lv_hash, resource);
	pthread_mutex_unlock(&lv_hash_lock);
	if (lvi) {
		return lvi->lock_mode;
	} else {
		return -1;
	}
}

/* Called at shutdown to tidy the lockspace */
void unlock_all()
{
	struct dm_hash_node *v;

	pthread_mutex_lock(&lv_hash_lock);
	dm_hash_iterate(v, lv_hash) {
		struct lv_info *lvi = dm_hash_get_data(lv_hash, v);

		sync_unlock(dm_hash_get_key(lv_hash, v), lvi->lock_id);
	}
	pthread_mutex_unlock(&lv_hash_lock);
}

/* Gets a real lock and keeps the info in the hash table */
int hold_lock(char *resource, int mode, int flags)
{
	int status;
	int saved_errno;
	struct lv_info *lvi;

	flags &= LKF_NOQUEUE;	/* Only LKF_NOQUEUE is valid here */

	pthread_mutex_lock(&lv_hash_lock);
	lvi = dm_hash_lookup(lv_hash, resource);
	pthread_mutex_unlock(&lv_hash_lock);
	if (lvi) {
		/* Already exists - convert it */
		status =
		    sync_lock(resource, mode, LKF_CONVERT | flags,
			      &lvi->lock_id);
		saved_errno = errno;
		if (!status)
			lvi->lock_mode = mode;

		if (status) {
			DEBUGLOG("hold_lock. convert to %d failed: %s\n", mode,
				 strerror(errno));
		}
		errno = saved_errno;
	} else {
		lvi = malloc(sizeof(struct lv_info));
		if (!lvi)
			return -1;

		lvi->lock_mode = mode;
		status = sync_lock(resource, mode, flags, &lvi->lock_id);
		saved_errno = errno;
		if (status) {
			free(lvi);
			DEBUGLOG("hold_lock. lock at %d failed: %s\n", mode,
				 strerror(errno));
		} else {
		        pthread_mutex_lock(&lv_hash_lock);
			dm_hash_insert(lv_hash, resource, lvi);
			pthread_mutex_unlock(&lv_hash_lock);
		}
		errno = saved_errno;
	}
	return status;
}

/* Unlock and remove it from the hash table */
int hold_unlock(char *resource)
{
	struct lv_info *lvi;
	int status;
	int saved_errno;

	pthread_mutex_lock(&lv_hash_lock);
	lvi = dm_hash_lookup(lv_hash, resource);
	pthread_mutex_unlock(&lv_hash_lock);
	if (!lvi) {
		DEBUGLOG("hold_unlock, lock not already held\n");
		return 0;
	}

	status = sync_unlock(resource, lvi->lock_id);
	saved_errno = errno;
	if (!status) {
	    	pthread_mutex_lock(&lv_hash_lock);
		dm_hash_remove(lv_hash, resource);
		pthread_mutex_unlock(&lv_hash_lock);
		free(lvi);
	} else {
		DEBUGLOG("hold_unlock. unlock failed(%d): %s\n", status,
			 strerror(errno));
	}

	errno = saved_errno;
	return status;
}

/* Watch the return codes here.
   liblvm API functions return 1(true) for success, 0(false) for failure and don't set errno.
   libdlm API functions return 0 for success, -1 for failure and do set errno.
   These functions here return 0 for success or >0 for failure (where the retcode is errno)
*/

/* Activate LV exclusive or non-exclusive */
static int do_activate_lv(char *resource, unsigned char lock_flags, int mode)
{
	int oldmode;
	int status;
	int activate_lv;
	int exclusive = 0;
	struct lvinfo lvi;

	/* Is it already open ? */
	oldmode = get_current_lock(resource);
	if (oldmode == mode) {
		return 0;	/* Nothing to do */
	}

	/* Does the config file want us to activate this LV ? */
	if (!lv_activation_filter(cmd, resource, &activate_lv))
		return EIO;

	if (!activate_lv)
		return 0;	/* Success, we did nothing! */

	/* Do we need to activate exclusively? */
	if ((activate_lv == 2) || (mode == LKM_EXMODE)) {
		exclusive = 1;
		mode = LKM_EXMODE;
	}

	/* Try to get the lock if it's a clustered volume group */
	if (lock_flags & LCK_CLUSTER_VG) {
		status = hold_lock(resource, mode, LKF_NOQUEUE);
		if (status) {
			/* Return an LVM-sensible error for this.
			 * Forcing EIO makes the upper level return this text
			 * rather than the strerror text for EAGAIN.
			 */
			if (errno == EAGAIN) {
				sprintf(last_error, "Volume is busy on another node");
				errno = EIO;
			}
			return errno;
		}
	}

	/* If it's suspended then resume it */
	if (!lv_info_by_lvid(cmd, resource, &lvi, 0, 0))
		return EIO;

	if (lvi.suspended)
		if (!lv_resume(cmd, resource))
			return EIO;

	/* Now activate it */
	if (!lv_activate(cmd, resource, exclusive))
		return EIO;

	return 0;
}

/* Resume the LV if it was active */
static int do_resume_lv(char *resource)
{
	int oldmode;

	/* Is it open ? */
	oldmode = get_current_lock(resource);
	if (oldmode == -1) {
		DEBUGLOG("do_resume_lv, lock not already held\n");
		return 0;	/* We don't need to do anything */
	}

	if (!lv_resume_if_active(cmd, resource))
		return EIO;

	return 0;
}

/* Suspend the device if active */
static int do_suspend_lv(char *resource)
{
	int oldmode;
	struct lvinfo lvi;

	/* Is it open ? */
	oldmode = get_current_lock(resource);
	if (oldmode == -1) {
		DEBUGLOG("do_suspend_lv, lock held at %d\n", oldmode);
		return 0; /* Not active, so it's OK */
	}

	/* Only suspend it if it exists */
	if (!lv_info_by_lvid(cmd, resource, &lvi, 0, 0))
		return EIO;

	if (lvi.exists) {
		if (!lv_suspend_if_active(cmd, resource)) {
			return EIO;
		}
	}
	return 0;
}

static int do_deactivate_lv(char *resource, unsigned char lock_flags)
{
	int oldmode;
	int status;

	/* Is it open ? */
	oldmode = get_current_lock(resource);
	if (oldmode == -1 && (lock_flags & LCK_CLUSTER_VG)) {
		DEBUGLOG("do_deactivate_lock, lock not already held\n");
		return 0;	/* We don't need to do anything */
	}

	if (!lv_deactivate(cmd, resource))
		return EIO;

	if (lock_flags & LCK_CLUSTER_VG) {
		status = hold_unlock(resource);
		if (status)
			return errno;
	}

	return 0;
}

/* This is the LOCK_LV part that happens on all nodes in the cluster -
   it is responsible for the interaction with device-mapper and LVM */
int do_lock_lv(unsigned char command, unsigned char lock_flags, char *resource)
{
	int status = 0;

	DEBUGLOG("do_lock_lv: resource '%s', cmd = %s, flags = %s\n",
		 resource, decode_locking_cmd(command), decode_flags(lock_flags));

	pthread_mutex_lock(&lvm_lock);
	if (!cmd->config_valid || config_files_changed(cmd)) {
		/* Reinitialise various settings inc. logging, filters */
		if (do_refresh_cache()) {
			log_error("Updated config file invalid. Aborting.");
			pthread_mutex_unlock(&lvm_lock);
			return EINVAL;
		}
	}

	if (lock_flags & LCK_MIRROR_NOSYNC_MODE)
		init_mirror_in_sync(1);

	if (!(lock_flags & LCK_DMEVENTD_MONITOR_MODE))
		init_dmeventd_monitor(0);

	switch (command) {
	case LCK_LV_EXCLUSIVE:
		status = do_activate_lv(resource, lock_flags, LKM_EXMODE);
		break;

	case LCK_LV_SUSPEND:
		status = do_suspend_lv(resource);
		if (!status)
			suspended++;
		break;

	case LCK_UNLOCK:
	case LCK_LV_RESUME:	/* if active */
		status = do_resume_lv(resource);
		if (!status)
			suspended--;
		break;

	case LCK_LV_ACTIVATE:
		status = do_activate_lv(resource, lock_flags, LKM_CRMODE);
		break;

	case LCK_LV_DEACTIVATE:
		status = do_deactivate_lv(resource, lock_flags);
		break;

	default:
		DEBUGLOG("Invalid LV command 0x%x\n", command);
		status = EINVAL;
		break;
	}

	if (lock_flags & LCK_MIRROR_NOSYNC_MODE)
		init_mirror_in_sync(0);

	if (!(lock_flags & LCK_DMEVENTD_MONITOR_MODE))
		init_dmeventd_monitor(DEFAULT_DMEVENTD_MONITOR);

	/* clean the pool for another command */
	dm_pool_empty(cmd->mem);
	pthread_mutex_unlock(&lvm_lock);

	DEBUGLOG("Command return is %d\n", status);
	return status;
}

/* Functions to do on the local node only BEFORE the cluster-wide stuff above happens */
int pre_lock_lv(unsigned char command, unsigned char lock_flags, char *resource)
{
	/* Nearly all the stuff happens cluster-wide. Apart from SUSPEND. Here we get the
	   lock out on this node (because we are the node modifying the metadata)
	   before suspending cluster-wide.
	 */
	if (command == LCK_LV_SUSPEND) {
		DEBUGLOG("pre_lock_lv: resource '%s', cmd = %s, flags = %s\n",
			 resource, decode_locking_cmd(command), decode_flags(lock_flags));

		if (hold_lock(resource, LKM_PWMODE, LKF_NOQUEUE))
			return errno;
	}
	return 0;
}

/* Functions to do on the local node only AFTER the cluster-wide stuff above happens */
int post_lock_lv(unsigned char command, unsigned char lock_flags,
		 char *resource)
{
	int status;

	/* Opposite of above, done on resume after a metadata update */
	if (command == LCK_LV_RESUME) {
		int oldmode;

		DEBUGLOG
		    ("post_lock_lv: resource '%s', cmd = %s, flags = %s\n",
		     resource, decode_locking_cmd(command), decode_flags(lock_flags));

		/* If the lock state is PW then restore it to what it was */
		oldmode = get_current_lock(resource);
		if (oldmode == LKM_PWMODE) {
			struct lvinfo lvi;

			pthread_mutex_lock(&lvm_lock);
			status = lv_info_by_lvid(cmd, resource, &lvi, 0, 0);
			pthread_mutex_unlock(&lvm_lock);
			if (!status)
				return EIO;

			if (lvi.exists) {
				if (hold_lock(resource, LKM_CRMODE, 0))
					return errno;
			} else {
				if (hold_unlock(resource))
					return errno;
			}
		}
	}
	return 0;
}

/* Check if a VG is in use by LVM1 so we don't stomp on it */
int do_check_lvm1(const char *vgname)
{
	int status;

	status = check_lvm1_vg_inactive(cmd, vgname);

	return status == 1 ? 0 : EBUSY;
}

int do_refresh_cache()
{
	int ret;
	DEBUGLOG("Refreshing context\n");
	log_notice("Refreshing context");

	ret = refresh_toolcontext(cmd);
	init_full_scan_done(0);
	lvmcache_label_scan(cmd, 2);

	return ret==1?0:-1;
}


/* Only called at gulm startup. Drop any leftover VG or P_orphan locks
   that might be hanging around if we died for any reason
*/
static void drop_vg_locks()
{
	char vg[128];
	char line[255];
	FILE *vgs =
	    popen
	    ("lvm pvs  --config 'log{command_names=0 prefix=\"\"}' --nolocking --noheadings -o vg_name", "r");

	sync_unlock("P_" VG_ORPHANS, LCK_EXCL);
	sync_unlock("P_" VG_GLOBAL, LCK_EXCL);

	if (!vgs)
		return;

	while (fgets(line, sizeof(line), vgs)) {
		char *vgend;
		char *vgstart;

		if (line[strlen(line)-1] == '\n')
			line[strlen(line)-1] = '\0';

		vgstart = line + strspn(line, " ");
		vgend = vgstart + strcspn(vgstart, " ");
		*vgend = '\0';

		if (strncmp(vgstart, "WARNING:", 8) == 0)
			continue;

		sprintf(vg, "V_%s", vgstart);
		sync_unlock(vg, LCK_EXCL);

	}
	if (fclose(vgs))
		DEBUGLOG("vgs fclose failed: %s\n", strerror(errno));
}

/*
 * Drop lvmcache metadata
 */
void drop_metadata(const char *vgname)
{
	DEBUGLOG("Dropping metadata for VG %s\n", vgname);
	pthread_mutex_lock(&lvm_lock);
	lvmcache_drop_metadata(vgname);
	pthread_mutex_unlock(&lvm_lock);
}

/*
 * Ideally, clvmd should be started before any LVs are active
 * but this may not be the case...
 * I suppose this also comes in handy if clvmd crashes, not that it would!
 */
static void *get_initial_state()
{
	char lv[64], vg[64], flags[25], vg_flags[25];
	char uuid[65];
	char line[255];
	FILE *lvs =
	    popen
	    ("lvm lvs  --config 'log{command_names=0 prefix=\"\"}' --nolocking --noheadings -o vg_uuid,lv_uuid,lv_attr,vg_attr",
	     "r");

	if (!lvs)
		return NULL;

	while (fgets(line, sizeof(line), lvs)) {
	        if (sscanf(line, "%s %s %s %s\n", vg, lv, flags, vg_flags) == 4) {

			/* States: s:suspended a:active S:dropped snapshot I:invalid snapshot */
		        if (strlen(vg) == 38 &&                         /* is is a valid UUID ? */
			    (flags[4] == 'a' || flags[4] == 's') &&	/* is it active or suspended? */
			    vg_flags[5] == 'c') {			/* is it clustered ? */
				/* Convert hyphen-separated UUIDs into one */
				memcpy(&uuid[0], &vg[0], 6);
				memcpy(&uuid[6], &vg[7], 4);
				memcpy(&uuid[10], &vg[12], 4);
				memcpy(&uuid[14], &vg[17], 4);
				memcpy(&uuid[18], &vg[22], 4);
				memcpy(&uuid[22], &vg[27], 4);
				memcpy(&uuid[26], &vg[32], 6);
				memcpy(&uuid[32], &lv[0], 6);
				memcpy(&uuid[38], &lv[7], 4);
				memcpy(&uuid[42], &lv[12], 4);
				memcpy(&uuid[46], &lv[17], 4);
				memcpy(&uuid[50], &lv[22], 4);
				memcpy(&uuid[54], &lv[27], 4);
				memcpy(&uuid[58], &lv[32], 6);
				uuid[64] = '\0';

				DEBUGLOG("getting initial lock for %s\n", uuid);
				hold_lock(uuid, LKM_CRMODE, LKF_NOQUEUE);
			}
		}
	}
	if (fclose(lvs))
		DEBUGLOG("lvs fclose failed: %s\n", strerror(errno));
	return NULL;
}

static void lvm2_log_fn(int level, const char *file, int line,
			const char *message)
{

	/* Send messages to the normal LVM2 logging system too,
	   so we get debug output when it's asked for.
 	   We need to NULL the function ptr otherwise it will just call
	   back into here! */
	init_log_fn(NULL);
	print_log(level, file, line, "%s", message);
	init_log_fn(lvm2_log_fn);

	/*
	 * Ignore non-error messages, but store the latest one for returning
	 * to the user.
	 */
	if (level != _LOG_ERR && level != _LOG_FATAL)
		return;

	strncpy(last_error, message, sizeof(last_error));
	last_error[sizeof(last_error)-1] = '\0';
}

/* This checks some basic cluster-LVM configuration stuff */
static void check_config()
{
	int locking_type;

	locking_type = find_config_tree_int(cmd, "global/locking_type", 1);

	if (locking_type == 3) /* compiled-in cluster support */
		return;

	if (locking_type == 2) { /* External library, check name */
		const char *libname;

		libname = find_config_tree_str(cmd, "global/locking_library",
					  "");
		if (strstr(libname, "liblvm2clusterlock.so"))
			return;

		log_error("Incorrect LVM locking library specified in lvm.conf, cluster operations may not work.");
		return;
	}
	log_error("locking_type not set correctly in lvm.conf, cluster operations will not work.");
}

void init_lvhash()
{
	/* Create hash table for keeping LV locks & status */
	lv_hash = dm_hash_create(100);
	pthread_mutex_init(&lv_hash_lock, NULL);
	pthread_mutex_init(&lvm_lock, NULL);
}

/* Backups up the LVM metadata if it's changed */
void lvm_do_backup(const char *vgname)
{
	struct volume_group * vg;
	int consistent = 0;

	DEBUGLOG("Triggering backup of VG metadata for %s. suspended=%d\n", vgname, suspended);

	vg = vg_read(cmd, vgname, NULL /*vgid*/, &consistent);
	if (vg) {
		if (consistent)
			check_current_backup(vg);
	}
	else {
		log_error("Error backing up metadata, can't find VG for group %s", vgname);
	}
}

/* Called to initialise the LVM context of the daemon */
int init_lvm(int using_gulm)
{
	if (!(cmd = create_toolcontext(1))) {
		log_error("Failed to allocate command context");
		return 0;
	}

	/* Use LOG_DAEMON for syslog messages instead of LOG_USER */
	init_syslog(LOG_DAEMON);
	openlog("clvmd", LOG_PID, LOG_DAEMON);
	cmd->cmd_line = (char *)"clvmd";

	/* Check lvm.conf is setup for cluster-LVM */
	check_config();

	/* Remove any non-LV locks that may have been left around */
	if (using_gulm)
		drop_vg_locks();

	get_initial_state();

	/* Trap log messages so we can pass them back to the user */
	init_log_fn(lvm2_log_fn);

	return 1;
}
