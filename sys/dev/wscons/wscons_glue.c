/* $NetBSD: wscons_glue.c,v 1.5 1999/01/18 20:03:59 drochner Exp $ */

/*
 * Copyright (c) 1996, 1997 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

static const char _copyright[] __attribute__ ((unused)) =
    "Copyright (c) 1996, 1997 Christopher G. Demetriou.  All rights reserved.";
static const char _rcsid[] __attribute__ ((unused)) =
    "$NetBSD: wscons_glue.c,v 1.5 1999/01/18 20:03:59 drochner Exp $";

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/wscons/wsksymvar.h>
#include <dev/wscons/wscons_callbacks.h>

#include "wskbd.h"
#include "wsdisplay.h"

#if NWSKBD > 0 && NWSDISPLAY > 0
extern struct cfdriver wsdisplay_cd, wskbd_cd;
#endif

int		wscons_setup_glue_callback_set;

static void	wscons_setup_glue __P((void *v));

void
wscons_glue_set_callback()
{

	if (!wscons_setup_glue_callback_set) {
		/* doesn't really hurt to run it twice, though... */
		timeout(wscons_setup_glue, NULL, 0);
		wscons_setup_glue_callback_set = 1;
	}
}

static void
wscons_setup_glue(v)
	void *v;
{
#if NWSKBD > 0 && NWSDISPLAY > 0
	int i, kbddev, displaydev;
	struct device *kbddevice, *displaydevice;

	wscons_setup_glue_callback_set = 0;

	/*
	 * Two passes:
	 *
	 * First:
	 *	Find the console keyboard and display devices,
	 *	glue them together if they're not already attached.
	 */
	kbddev = displaydev = -1;
	for (i = 0; i < wskbd_cd.cd_ndevs; i++)
		if (wskbd_cd.cd_devs[i] != NULL &&
		    wskbd_is_console(wskbd_cd.cd_devs[i])) {
			kbddev = i;
			break;
	}
	for (i = 0; i < wsdisplay_cd.cd_ndevs; i++)
		if (wsdisplay_cd.cd_devs[i] != NULL &&
		    wsdisplay_is_console(wsdisplay_cd.cd_devs[i])) {
			displaydev = i;
			break;
	}
	if (kbddev != -1 && (wskbd_cd.cd_devs[kbddev] != NULL) &&
	    wskbd_display(wskbd_cd.cd_devs[kbddev]) == NULL &&
	    displaydev != -1 && (wsdisplay_cd.cd_devs[displaydev] != NULL) &&
	    wsdisplay_kbd(wsdisplay_cd.cd_devs[displaydev]) == NULL) {
		kbddevice = wskbd_cd.cd_devs[kbddev];
		displaydevice = wsdisplay_cd.cd_devs[displaydev];

		wskbd_set_display(kbddevice, displaydevice);
		wsdisplay_set_kbd(displaydevice, kbddevice);

		printf("wscons: %s glued to %s (console)\n",
		    kbddevice->dv_xname, displaydevice->dv_xname);
	}

	/*
	 * Second:
	 *
	 *	Attach remaining unattached keyboard and display
	 *	devices, in order by unit number.
	 */
	i = 0;
	for (kbddev = 0; kbddev < wskbd_cd.cd_ndevs; kbddev++) {
		if (wskbd_cd.cd_devs[kbddev] == NULL ||
		    wskbd_display(wskbd_cd.cd_devs[kbddev]) != NULL)
			continue;

		displaydev = -1;
		for (; i < wsdisplay_cd.cd_ndevs; i++) {
			if (wsdisplay_cd.cd_devs[i] == NULL ||
			    wsdisplay_kbd(wsdisplay_cd.cd_devs[i]) != NULL)
				continue;

			displaydev = i;
			break;
		}
		if (displaydev == -1)
			continue;

		kbddevice = wskbd_cd.cd_devs[kbddev];
		displaydevice = wsdisplay_cd.cd_devs[displaydev];

		KASSERT(kbddevice != NULL);
		KASSERT(wskbd_display(kbddevice) == NULL);
		KASSERT(displaydevice != NULL);
		KASSERT(wsdisplay_kbd(displaydevice) == NULL);

		wskbd_set_display(kbddevice, displaydevice);
		wsdisplay_set_kbd(displaydevice, kbddevice);

		printf("wscons: %s glued to %s\n", kbddevice->dv_xname,
		    displaydevice->dv_xname);
	}

	/* Now wasn't that simple? */
#endif /* NWSKBD > 0 && NWSDISPLAY > 0 */
}
