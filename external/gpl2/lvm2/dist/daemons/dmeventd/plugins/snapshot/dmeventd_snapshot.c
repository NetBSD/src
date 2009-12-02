/*	$NetBSD: dmeventd_snapshot.c,v 1.1.1.2 2009/12/02 00:27:12 haad Exp $	*/

/*
 * Copyright (C) 2007-2008 Red Hat, Inc. All rights reserved.
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

#include "libdevmapper.h"
#include "libdevmapper-event.h"
#include "lvm2cmd.h"
#include "lvm-string.h"

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include <syslog.h> /* FIXME Replace syslog with multilog */
/* FIXME Missing openlog? */

/* First warning when snapshot is 80% full. */
#define WARNING_THRESH 80
/* Further warnings at 85%, 90% and 95% fullness. */
#define WARNING_STEP 5

static pthread_mutex_t _register_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Number of active registrations.
 */
static int _register_count = 0;

static struct dm_pool *_mem_pool = NULL;
static void *_lvm_handle = NULL;

struct snap_status {
	int invalid;
	int used;
	int max;
};

/*
 * Currently only one event can be processed at a time.
 */
static pthread_mutex_t _event_mutex = PTHREAD_MUTEX_INITIALIZER;

static void _temporary_log_fn(int level,
			      const char *file __attribute((unused)),
			      int line __attribute((unused)),
			      int dm_errno __attribute((unused)),
			      const char *format)
{
	if (!strncmp(format, "WARNING: ", 9) && (level < 5))
		syslog(LOG_CRIT, "%s", format);
	else
		syslog(LOG_DEBUG, "%s", format);
}

/* FIXME possibly reconcile this with target_percent when we gain
   access to regular LVM library here. */
static void _parse_snapshot_params(char *params, struct snap_status *stat)
{
	char *p;
	/*
	 * xx/xx	-- fractions used/max
	 * Invalid	-- snapshot invalidated
	 * Unknown	-- status unknown
	 */
	stat->used = stat->max = 0;

	if (!strncmp(params, "Invalid", 7)) {
		stat->invalid = 1;
		return;
	}

	/*
	 * When we return without setting non-zero max, the parent is
	 * responsible for reporting errors.
	 */
	if (!strncmp(params, "Unknown", 7))
		return;

	if (!(p = strstr(params, "/")))
		return;

	*p = '\0';
	p++;

	stat->used = atoi(params);
	stat->max = atoi(p);
}

void process_event(struct dm_task *dmt,
		   enum dm_event_mask event __attribute((unused)),
		   void **private)
{
	void *next = NULL;
	uint64_t start, length;
	char *target_type = NULL;
	char *params;
	struct snap_status stat = { 0 };
	const char *device = dm_task_get_name(dmt);
	int percent, *percent_warning = (int*)private;

	/* No longer monitoring, waiting for remove */
	if (!*percent_warning)
		return;

	if (pthread_mutex_trylock(&_event_mutex)) {
		syslog(LOG_NOTICE, "Another thread is handling an event.  Waiting...");
		pthread_mutex_lock(&_event_mutex);
	}

	dm_get_next_target(dmt, next, &start, &length, &target_type, &params);
	if (!target_type)
		goto out;

	_parse_snapshot_params(params, &stat);
	/*
	 * If the snapshot has been invalidated or we failed to parse
	 * the status string. Report the full status string to syslog.
	 */
	if (stat.invalid || !stat.max) {
		syslog(LOG_ERR, "Snapshot %s changed state to: %s\n", device, params);
		*percent_warning = 0;
		goto out;
	}

	percent = 100 * stat.used / stat.max;
	if (percent >= *percent_warning) {
		syslog(LOG_WARNING, "Snapshot %s is now %i%% full.\n", device, percent);
		/* Print warning on the next multiple of WARNING_STEP. */
		*percent_warning = (percent / WARNING_STEP) * WARNING_STEP + WARNING_STEP;
	}
out:
	pthread_mutex_unlock(&_event_mutex);
}

int register_device(const char *device,
		    const char *uuid __attribute((unused)),
		    int major __attribute((unused)),
		    int minor __attribute((unused)),
		    void **private)
{
	int r = 0;
	int *percent_warning = (int*)private;

	pthread_mutex_lock(&_register_mutex);

	/*
	 * Need some space for allocations.  1024 should be more
	 * than enough for what we need (device mapper name splitting)
	 */
	if (!_mem_pool && !(_mem_pool = dm_pool_create("snapshot_dso", 1024)))
		goto out;

	*percent_warning = WARNING_THRESH; /* Print warning if snapshot is full */

	if (!_lvm_handle) {
		lvm2_log_fn(_temporary_log_fn);
		if (!(_lvm_handle = lvm2_init())) {
			dm_pool_destroy(_mem_pool);
			_mem_pool = NULL;
			goto out;
		}
		lvm2_log_level(_lvm_handle, LVM2_LOG_SUPPRESS);
		/* FIXME Temporary: move to dmeventd core */
		lvm2_run(_lvm_handle, "_memlock_inc");
	}

	syslog(LOG_INFO, "Monitoring snapshot %s\n", device);

	_register_count++;
	r = 1;

out:
	pthread_mutex_unlock(&_register_mutex);

	return r;
}

int unregister_device(const char *device,
		      const char *uuid __attribute((unused)),
		      int major __attribute((unused)),
		      int minor __attribute((unused)),
		      void **unused __attribute((unused)))
{
	pthread_mutex_lock(&_register_mutex);

	syslog(LOG_INFO, "No longer monitoring snapshot %s\n",
	       device);

	if (!--_register_count) {
		dm_pool_destroy(_mem_pool);
		_mem_pool = NULL;
		lvm2_run(_lvm_handle, "_memlock_dec");
		lvm2_exit(_lvm_handle);
		_lvm_handle = NULL;
	}

	pthread_mutex_unlock(&_register_mutex);

	return 1;
}
