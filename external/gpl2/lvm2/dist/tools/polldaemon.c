/*	$NetBSD: polldaemon.c,v 1.1.1.1 2008/12/22 00:19:05 haad Exp $	*/

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
	dev_close_all();

	return 1;
}

static int _check_mirror_status(struct cmd_context *cmd,
				struct volume_group *vg,
				struct logical_volume *lv_mirr,
				const char *name, struct daemon_parms *parms,
				int *finished)
{
	struct dm_list *lvs_changed;
	float segment_percent = 0.0, overall_percent = 0.0;
	uint32_t event_nr = 0;

	/* By default, caller should not retry */
	*finished = 1;

	if (parms->aborting) {
		if (!(lvs_changed = lvs_using_lv(cmd, vg, lv_mirr))) {
			log_error("Failed to generate list of copied LVs: "
				  "can't abort.");
			return 0;
		}
		parms->poll_fns->finish_copy(cmd, vg, lv_mirr, lvs_changed);
		return 0;
	}

	if (!lv_mirror_percent(cmd, lv_mirr, !parms->interval, &segment_percent,
			       &event_nr)) {
		log_error("ABORTING: Mirror percentage check failed.");
		return 0;
	}

	overall_percent = copy_percent(lv_mirr);
	if (parms->progress_display)
		log_print("%s: %s: %.1f%%", name, parms->progress_title,
			  overall_percent);
	else
		log_verbose("%s: %s: %.1f%%", name, parms->progress_title,
			    overall_percent);

	if (segment_percent < 100.0) {
		/* The only case the caller *should* try again later */
		*finished = 0;
		return 1;
	}

	if (!(lvs_changed = lvs_using_lv(cmd, vg, lv_mirr))) {
		log_error("ABORTING: Failed to generate list of copied LVs");
		return 0;
	}

	/* Finished? Or progress to next segment? */
	if (overall_percent >= 100.0) {
		if (!parms->poll_fns->finish_copy(cmd, vg, lv_mirr,
						  lvs_changed))
			return 0;
	} else {
		if (!parms->poll_fns->update_metadata(cmd, vg, lv_mirr,
						      lvs_changed, 0)) {
			log_error("ABORTING: Segment progression failed.");
			parms->poll_fns->finish_copy(cmd, vg, lv_mirr,
						     lvs_changed);
			return 0;
		}
		*finished = 0;	/* Another segment */
	}

	return 1;
}

static int _wait_for_single_mirror(struct cmd_context *cmd, const char *name,
				   struct daemon_parms *parms)
{
	struct volume_group *vg;
	struct logical_volume *lv_mirr;
	int finished = 0;

	/* Poll for mirror completion */
	while (!finished) {
		/* FIXME Also needed in vg/lvchange -ay? */
		/* FIXME Use alarm for regular intervals instead */
		if (parms->interval && !parms->aborting) {
			sleep(parms->interval);
			/* Devices might have changed while we slept */
			init_full_scan_done(0);
		}

		/* Locks the (possibly renamed) VG again */
		if (!(vg = parms->poll_fns->get_copy_vg(cmd, name))) {
			log_error("ABORTING: Can't reread VG for %s", name);
			/* What more could we do here? */
			return 0;
		}

		if (!(lv_mirr = parms->poll_fns->get_copy_lv(cmd, vg, name,
							     parms->lv_type))) {
			log_error("ABORTING: Can't find mirror LV in %s for %s",
				  vg->name, name);
			unlock_vg(cmd, vg->name);
			return 0;
		}

		if (!_check_mirror_status(cmd, vg, lv_mirr, name, parms,
					  &finished)) {
			unlock_vg(cmd, vg->name);
			return 0;
		}

		unlock_vg(cmd, vg->name);
	}

	return 1;
}

static int _poll_vg(struct cmd_context *cmd, const char *vgname,
		    struct volume_group *vg, int consistent, void *handle)
{
	struct daemon_parms *parms = (struct daemon_parms *) handle;
	struct lv_list *lvl;
	struct logical_volume *lv_mirr;
	const char *name;
	int finished;

	if (!vg) {
		log_error("Couldn't read volume group %s", vgname);
		return ECMD_FAILED;
	}

	if (!consistent) {
		log_error("Volume Group %s inconsistent - skipping", vgname);
		/* FIXME Should we silently recover it here or not? */
		return ECMD_FAILED;
	}

	if (!vg_check_status(vg, EXPORTED_VG))
		return ECMD_FAILED;

	dm_list_iterate_items(lvl, &vg->lvs) {
		lv_mirr = lvl->lv;
		if (!(lv_mirr->status & parms->lv_type))
			continue;
		if (!(name = parms->poll_fns->get_copy_name_from_lv(lv_mirr)))
			continue;
		/* FIXME Need to do the activation from _set_up_pvmove here
		 *       if it's not running and we're not aborting */
		if (_check_mirror_status(cmd, vg, lv_mirr, name,
					 parms, &finished) && !finished)
			parms->outstanding_count++;
	}

	return ECMD_PROCESSED;

}

static void _poll_for_all_vgs(struct cmd_context *cmd,
			      struct daemon_parms *parms)
{
	while (1) {
		parms->outstanding_count = 0;
		process_each_vg(cmd, 0, NULL, LCK_VG_WRITE, 1, parms, _poll_vg);
		if (!parms->outstanding_count)
			break;
		sleep(parms->interval);
	}
}

int poll_daemon(struct cmd_context *cmd, const char *name, unsigned background,
		uint32_t lv_type, struct poll_functions *poll_fns,
		const char *progress_title)
{
	struct daemon_parms parms;

	parms.aborting = arg_count(cmd, abort_ARG) ? 1 : 0;
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

	if (name) {
		if (!_wait_for_single_mirror(cmd, name, &parms))
			return ECMD_FAILED;
	} else
		_poll_for_all_vgs(cmd, &parms);

	return ECMD_PROCESSED;
}
