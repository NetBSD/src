/*	$NetBSD: file_locking.c,v 1.1.1.1.2.1 2009/05/13 18:52:42 jym Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2007 Red Hat, Inc. All rights reserved.
 *
 * This file is part of LVM2.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "lib.h"
#include "locking.h"
#include "locking_types.h"
#include "activate.h"
#include "config.h"
#include "defaults.h"
#include "lvm-file.h"
#include "lvm-string.h"
#include "lvmcache.h"

#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <signal.h>

struct lock_list {
	struct dm_list list;
	int lf;
	char *res;
};

static struct dm_list _lock_list;
static char _lock_dir[NAME_LEN];

static sig_t _oldhandler;
static sigset_t _fullsigset, _intsigset;
static volatile sig_atomic_t _handler_installed;

static int _release_lock(const char *file, int unlock)
{
	struct lock_list *ll;
	struct dm_list *llh, *llt;

	struct stat buf1, buf2;

	dm_list_iterate_safe(llh, llt, &_lock_list) {
		ll = dm_list_item(llh, struct lock_list);

		if (!file || !strcmp(ll->res, file)) {
			dm_list_del(llh);
			if (unlock) {
				log_very_verbose("Unlocking %s", ll->res);
				if (flock(ll->lf, LOCK_NB | LOCK_UN))
					log_sys_error("flock", ll->res);
			}

			if (!flock(ll->lf, LOCK_NB | LOCK_EX) &&
			    !stat(ll->res, &buf1) &&
			    !fstat(ll->lf, &buf2) &&
			    is_same_inode(buf1, buf2))
				if (unlink(ll->res))
					log_sys_error("unlink", ll->res);

			if (close(ll->lf) < 0)
				log_sys_error("close", ll->res);

			dm_free(ll->res);
			dm_free(llh);

			if (file)
				return 1;
		}
	}

	return 0;
}

static void _fin_file_locking(void)
{
	_release_lock(NULL, 1);
}

static void _reset_file_locking(void)
{
	_release_lock(NULL, 0);
}

static void _remove_ctrl_c_handler(void)
{
	siginterrupt(SIGINT, 0);
	if (!_handler_installed)
		return;

	_handler_installed = 0;

	sigprocmask(SIG_SETMASK, &_fullsigset, NULL);
	if (signal(SIGINT, _oldhandler) == SIG_ERR)
		log_sys_error("signal", "_remove_ctrl_c_handler");
}

static void _trap_ctrl_c(int sig __attribute((unused)))
{
	_remove_ctrl_c_handler();
	log_error("CTRL-c detected: giving up waiting for lock");
}

static void _install_ctrl_c_handler()
{
	_handler_installed = 1;

	if ((_oldhandler = signal(SIGINT, _trap_ctrl_c)) == SIG_ERR) {
		_handler_installed = 0;
		return;
	}

	sigprocmask(SIG_SETMASK, &_intsigset, NULL);
	siginterrupt(SIGINT, 1);
}

static int _lock_file(const char *file, uint32_t flags)
{
	int operation;
	int r = 1;
	int old_errno;

	struct lock_list *ll;
	struct stat buf1, buf2;
	char state;

	switch (flags & LCK_TYPE_MASK) {
	case LCK_READ:
		operation = LOCK_SH;
		state = 'R';
		break;
	case LCK_WRITE:
		operation = LOCK_EX;
		state = 'W';
		break;
	case LCK_UNLOCK:
		return _release_lock(file, 1);
	default:
		log_error("Unrecognised lock type: %d", flags & LCK_TYPE_MASK);
		return 0;
	}

	if (!(ll = dm_malloc(sizeof(struct lock_list))))
		return 0;

	if (!(ll->res = dm_strdup(file))) {
		dm_free(ll);
		return 0;
	}

	ll->lf = -1;

	log_very_verbose("Locking %s %c%c", ll->res, state,
			 flags & LCK_NONBLOCK ? ' ' : 'B');
	do {
		if ((ll->lf > -1) && close(ll->lf))
			log_sys_error("close", file);

		if ((ll->lf = open(file, O_CREAT | O_APPEND | O_RDWR, 0777))
		    < 0) {
			log_sys_error("open", file);
			goto err;
		}

		if ((flags & LCK_NONBLOCK))
			operation |= LOCK_NB;
		else
			_install_ctrl_c_handler();

		r = flock(ll->lf, operation);
		old_errno = errno;
		if (!(flags & LCK_NONBLOCK))
			_remove_ctrl_c_handler();

		if (r) {
			errno = old_errno;
			log_sys_error("flock", ll->res);
			close(ll->lf);
			goto err;
		}

		if (!stat(ll->res, &buf1) && !fstat(ll->lf, &buf2) &&
		    is_same_inode(buf1, buf2))
			break;
	} while (!(flags & LCK_NONBLOCK));

	dm_list_add(&_lock_list, &ll->list);
	return 1;

      err:
	dm_free(ll->res);
	dm_free(ll);
	return 0;
}

static int _file_lock_resource(struct cmd_context *cmd, const char *resource,
			       uint32_t flags)
{
	char lockfile[PATH_MAX];

	switch (flags & LCK_SCOPE_MASK) {
	case LCK_VG:
		/* Skip cache refresh for VG_GLOBAL - the caller handles it */
		if (strcmp(resource, VG_GLOBAL))
			lvmcache_drop_metadata(resource);

		/* LCK_CACHE does not require a real lock */
		if (flags & LCK_CACHE)
			break;

		if (*resource == '#')
			dm_snprintf(lockfile, sizeof(lockfile),
				     "%s/P_%s", _lock_dir, resource + 1);
		else
			dm_snprintf(lockfile, sizeof(lockfile),
				     "%s/V_%s", _lock_dir, resource);

		if (!_lock_file(lockfile, flags))
			return_0;
		break;
	case LCK_LV:
		switch (flags & LCK_TYPE_MASK) {
		case LCK_UNLOCK:
			log_very_verbose("Unlocking LV %s", resource);
			if (!lv_resume_if_active(cmd, resource))
				return 0;
			break;
		case LCK_NULL:
			log_very_verbose("Locking LV %s (NL)", resource);
			if (!lv_deactivate(cmd, resource))
				return 0;
			break;
		case LCK_READ:
			log_very_verbose("Locking LV %s (R)", resource);
			if (!lv_activate_with_filter(cmd, resource, 0))
				return 0;
			break;
		case LCK_PREAD:
			log_very_verbose("Locking LV %s (PR) - ignored", resource);
			break;
		case LCK_WRITE:
			log_very_verbose("Locking LV %s (W)", resource);
			if (!lv_suspend_if_active(cmd, resource))
				return 0;
			break;
		case LCK_EXCL:
			log_very_verbose("Locking LV %s (EX)", resource);
			if (!lv_activate_with_filter(cmd, resource, 1))
				return 0;
			break;
		default:
			break;
		}
		break;
	default:
		log_error("Unrecognised lock scope: %d",
			  flags & LCK_SCOPE_MASK);
		return 0;
	}

	return 1;
}

int init_file_locking(struct locking_type *locking, struct cmd_context *cmd)
{
	locking->lock_resource = _file_lock_resource;
	locking->reset_locking = _reset_file_locking;
	locking->fin_locking = _fin_file_locking;
	locking->flags = 0;

	/* Get lockfile directory from config file */
	strncpy(_lock_dir, find_config_tree_str(cmd, "global/locking_dir",
						DEFAULT_LOCK_DIR),
		sizeof(_lock_dir));

	if (!dm_create_dir(_lock_dir))
		return 0;

	/* Trap a read-only file system */
	if ((access(_lock_dir, R_OK | W_OK | X_OK) == -1) && (errno == EROFS))
		return 0;

	dm_list_init(&_lock_list);

	if (sigfillset(&_intsigset) || sigfillset(&_fullsigset)) {
		log_sys_error("sigfillset", "init_file_locking");
		return 0;
	}

	if (sigdelset(&_intsigset, SIGINT)) {
		log_sys_error("sigdelset", "init_file_locking");
		return 0;
	}

	return 1;
}
