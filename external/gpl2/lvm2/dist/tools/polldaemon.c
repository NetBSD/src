/*	$NetBSD: polldaemon.c,v 1.1.1.2 2009/12/02 00:25:53 haad Exp $	*/

/*
 * Copyright (C) 2003-2004 Sistina Software, Inc. All rights reserved.
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

#include "tools.h"
#include "polldaemon.h"
#include <signal.h>
#include <sys/wait.h>

static void _sigchld_handler(int sig __attribute((unused)))
{
	while (wait4(-1, NULL, WNOHANG | WUNTRACED, NULL) > 0) ;
}

static int _become_daemon(struct cmd_context *cmd)
{
	pid_t pid;
	struct sigaction act = {
		{_sigchld_handler},
		.sa_flags = SA_NOCLDSTOP,
	};

	log_verbose("Forking background process");

	sigaction(SIGCHLD, &act, NULL);

	if ((pid = fork()) == -1) {
		log_error("fork failed: %s", strerror(errno));
		return 1;
	}

	/* Parent */
	if (pid > 0)
		return 0;

	/* Child */
	if (setsid() == -1)
		log_error("Background process failed to setsid: %s",
			  strerror(errno));
	init_verbose(VERBOSE_BASE_LEVEL);

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	strncpy(*cmd->argv, "(lvm2copyd)", strlen(*cmd->argv));

	reset_locking();
	lvmcache_init();
	dev_close_all();

	return 1;
}

progress_t poll_mirror_progress(struct cmd_context *cmd,
				struct logical_volume *lv, const char *name,
				struct daemon_parms *parms)
{
	float segment_percent = 0.0, overall_percent = 0.0;
	percent_range_t percent_range, overall_percent_range;
	uint32_t event_nr = 0;

	if (!lv_mirror_percent(cmd, lv, !parms->interval, &segment_percent,
			       &percent_range, &event_nr) ||
	    (percent_range == PERCENT_INVALID)) {
		log_error("ABORTING: Mirror percentage check failed.");
		return PROGRESS_CHECK_FAILED;
	}

	overall_percent = copy_percent(lv, &overall_percent_range);
	if (parms->progress_display)
		log_print("%s: %s: %.1f%%", name, parms->progress_title,
			  overall_percent);
	else
		log_verbose("%s: %s: %.1f%%", name, parms->progress_title,
			    overall_percent);

	if (percent_range != PERCENT_100)
		return PROGRESS_UNFINISHED;

	if (overall_percent_range == PERCENT_100)
		return PROGRESS_FINISHED_ALL;

	return PROGRESS_FINISHED_SEGMENT;
}

static int _check_lv_status(struct cmd_context *cmd,
			    struct volume_group *vg,
			    struct logical_volume *lv,
			    const char *name, struct daemon_parms *parms,
			    int *finished)
{
	struct dm_list *lvs_changed;
	progress_t progress;

	/* By default, caller should not retry */
	*finished = 1;

	if (parms->aborting) {
		if (!(lvs_changed = lvs_using_lv(cmd, vg, lv))) {
			log_error("Failed to generate list of copied LVs: "
				  "can't abort.");
			return 0;
		}
		parms->poll_fns->finish_copy(cmd, vg, lv, lvs_changed);
		return 0;
	}

	progress = parms->poll_fns->poll_progress(cmd, lv, name, parms);
	if (progress == PROGRESS_CHECK_FAILED)
		return_0;

	if (progress == PROGRESS_UNFINISHED) {
		/* The only case the caller *should* try again later */
		*finished = 0;
		return 1;
	}

	if (!(lvs_changed = lvs_using_lv(cmd, vg, lv))) {
		log_error("ABORTING: Failed to generate list of copied LVs");
		return 0;
	}

	/* Finished? Or progress to next segment? */
	if (progress == PROGRESS_FINISHED_ALL) {
		if (!parms->poll_fns->finish_copy(cmd, vg, lv, lvs_changed))
			return 0;
	} else {
		if (!parms->poll_fns->update_metadata(cmd, vg, lv, lvs_changed,
						      0)) {
			log_error("ABORTING: Segment progression failed.");
			parms->poll_fns->finish_copy(cmd, vg, lv, lvs_changed);
			return 0;
		}
		*finished = 0;	/* Another segment */
	}

	return 1;
}

static int _wait_for_single_lv(struct cmd_context *cmd, const char *name, const char *uuid,
				   struct daemon_parms *parms)
{
	struct volume_group *vg;
	struct logical_volume *lv;
	int finished = 0;

	/* Poll for completion */
	while (!finished) {
		/* FIXME Also needed in vg/lvchange -ay? */
		/* FIXME Use alarm for regular intervals instead */
		if (parms->interval && !parms->aborting) {
			sleep(parms->interval);
			/* Devices might have changed while we slept */
			init_full_scan_done(0);
		}

		/* Locks the (possibly renamed) VG again */
		vg = parms->poll_fns->get_copy_vg(cmd, name, uuid);
		if (vg_read_error(vg)) {
			vg_release(vg);
			log_error("ABORTING: Can't reread VG for %s", name);
			/* What more could we do here? */
			return 0;
		}

		if (!(lv = parms->poll_fns->get_copy_lv(cmd, vg, name, uuid,
							parms->lv_type))) {
			log_error("ABORTING: Can't find mirror LV in %s for %s",
				  vg->name, name);
			unlock_and_release_vg(cmd, vg, vg->name);
			return 0;
		}

		if (!_check_lv_status(cmd, vg, lv, name, parms, &finished)) {
			unlock_and_release_vg(cmd, vg, vg->name);
			return 0;
		}

		unlock_and_release_vg(cmd, vg, vg->name);
	}

	return 1;
}

static int _poll_vg(struct cmd_context *cmd, const char *vgname,
		    struct volume_group *vg, void *handle)
{
	struct daemon_parms *parms = (struct daemon_parms *) handle;
	struct lv_list *lvl;
	struct logical_volume *lv;
	const char *name;
	int finished;

	dm_list_iterate_items(lvl, &vg->lvs) {
		lv = lvl->lv;
		if (!(lv->status & parms->lv_type))
			continue;
		if (!(name = parms->poll_fns->get_copy_name_from_lv(lv)))
			continue;
		/* FIXME Need to do the activation from _set_up_pvmove here
		 *       if it's not running and we're not aborting */
		if (_check_lv_status(cmd, vg, lv, name, parms, &finished) &&
		    !finished)
			parms->outstanding_count++;
	}

	return ECMD_PROCESSED;

}

static void _poll_for_all_vgs(struct cmd_context *cmd,
			      struct daemon_parms *parms)
{
	while (1) {
		parms->outstanding_count = 0;
		process_each_vg(cmd, 0, NULL, READ_FOR_UPDATE, parms, _poll_vg);
		if (!parms->outstanding_count)
			break;
		sleep(parms->interval);
	}
}

int poll_daemon(struct cmd_context *cmd, const char *name, const char *uuid,
		unsigned background,
		uint32_t lv_type, struct poll_functions *poll_fns,
		const char *progress_title)
{
	struct daemon_parms parms;

	parms.aborting = arg_is_set(cmd, abort_ARG);
	parms.background = background;
	parms.interval = arg_uint_value(cmd, interval_ARG, DEFAULT_INTERVAL);
	parms.progress_display = 1;
	parms.progress_title = progress_title;
	parms.lv_type = lv_type;
	parms.poll_fns = poll_fns;

	if (parms.interval && !parms.aborting)
		log_verbose("Checking progress every %u seconds",
			    parms.interval);

	if (!parms.interval) {
		parms.progress_display = 0;

		/* FIXME Disabled multiple-copy wait_event */
		if (!name)
			parms.interval = DEFAULT_INTERVAL;
	}

	if (parms.background) {
		if (!_become_daemon(cmd))
			return ECMD_PROCESSED;	/* Parent */
		parms.progress_display = 0;
		/* FIXME Use wait_event (i.e. interval = 0) and */
		/*       fork one daemon per copy? */
	}

	/*
	 * Process one specific task or all incomplete tasks?
	 */
	if (name) {
		if (!_wait_for_single_lv(cmd, name, uuid, &parms)) {
			stack;
			return ECMD_FAILED;
		}
	} else
		_poll_for_all_vgs(cmd, &parms);

	return ECMD_PROCESSED;
}
