/* $NetBSD: action.c,v 1.2 2003/08/06 23:58:40 jmmv Exp $ */

/*
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julio M. Merino Vidal.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name authors may not be used to endorse or promote products
 *    derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: action.c,v 1.2 2003/08/06 23:58:40 jmmv Exp $");
#endif /* not lint */

#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/tty.h>
#include <dev/wscons/wsconsio.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>

#include "pathnames.h"
#include "wsmoused.h"

/* ---------------------------------------------------------------------- */

/*
 * Public interface exported by the `action' mode.
 */

int  action_startup(struct mouse *m);
int  action_cleanup(void);
void action_wsmouse_event(struct wscons_event);

struct mode_bootstrap Action_Mode = {
	"action",
	action_startup,
	action_cleanup,
	action_wsmouse_event,
	NULL,
	NULL
};

/* ---------------------------------------------------------------------- */

/*
 * Global variables.
 */

static int Initialized = 0;

/* ---------------------------------------------------------------------- */

/*
 * Prototypes for functions private to this module.
 */

static void run_action(int, const char *);

/* ---------------------------------------------------------------------- */

/* Initializes the action mode.  Does nothing, aside from checking it is
 * not initialized twice. */
/* ARGSUSED */
int
action_startup(struct mouse *m)
{

	if (Initialized) {
		log_warnx("action mode already initialized");
		return 1;
	}

	Initialized = 1;

	return 1;
}

/* ---------------------------------------------------------------------- */

/* Mode cleanup. */
int
action_cleanup(void)
{

	return 1;
}

/* ---------------------------------------------------------------------- */

/* Parses wsmouse events.  Only button events are picked. */
void
action_wsmouse_event(struct wscons_event evt)
{

	if (IS_BUTTON_EVENT(evt.type)) {
		switch (evt.type) {
		case WSCONS_EVENT_MOUSE_UP:
			run_action(evt.value, "up");
			break;

		case WSCONS_EVENT_MOUSE_DOWN:
			run_action(evt.value, "down");
			break;

		default:
			log_warnx("unknown button event");
		}
	}
}

/* ---------------------------------------------------------------------- */

/* Executes a command.  `button' specifies the number of the button that
 * was pressed or released.  `ud' contains the word `up' or `down', which
 * specify the type of event received. */
static void
run_action(int button, const char *ud)
{
	char buf[20];
	const char *cmd;
	struct block *conf;

	(void)snprintf(buf, sizeof(buf), "button_%d_%s", button, ud);

	conf = config_get_mode("action");
	cmd = block_get_propval(conf, buf, NULL);
	if (cmd != NULL) {
		log_info("running command `%s'", cmd);
		system(cmd);
	}
}
