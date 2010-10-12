/* $NetBSD: pickmode.c,v 1.2 2010/10/12 16:18:19 macallan Exp $ */

/*-
 * Copyright (c) 2006 The NetBSD Foundation
 * All rights reserved.
 *
 * this code was contributed to The NetBSD Foundation by Michael Lorenz
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE NETBSD FOUNDATION BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */ 

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pickmode.c,v 1.2 2010/10/12 16:18:19 macallan Exp $");

#include <sys/param.h>
#include <dev/videomode/videomode.h>
#include "opt_videomode.h"

#ifdef PICKMODE_DEBUG
#define DPRINTF printf
#else
#define DPRINTF while (0) printf
#endif

const struct videomode *
pick_mode_by_dotclock(int width, int height, int dotclock)
{
	const struct videomode *this, *best = NULL;
	int i;

	DPRINTF("%s: looking for %d x %d at up to %d kHz\n", __func__, width,
	    height, dotclock);
	for (i = 0; i < videomode_count; i++) {

		this = &videomode_list[i];
		if ((this->hdisplay != width) || (this->vdisplay != height) ||
		    (this->dot_clock > dotclock))
			continue;
		if (best != NULL) {

			if (this->dot_clock > best->dot_clock)
				best = this;
		} else
			best = this;
	}
	if (best!= NULL)
		DPRINTF("found %s\n", best->name);

	return best;
}

const struct videomode *
pick_mode_by_ref(int width, int height, int refresh)
{
	const struct videomode *this, *best = NULL;
	int mref, closest = 1000, i, diff;

	DPRINTF("%s: looking for %d x %d at up to %d Hz\n", __func__, width,
	    height, refresh);
	for (i = 0; i < videomode_count; i++) {

		this = &videomode_list[i];
		mref = this->dot_clock * 1000 / (this->htotal * this->vtotal);
		diff = abs(mref - refresh);
		if ((this->hdisplay != width) || (this->vdisplay != height))
			continue;
		DPRINTF("%s in %d hz, diff %d\n", this->name, mref, diff);
		if (best != NULL) {

			if (diff < closest) {

				best = this;
				closest = diff;
			}
		} else {
			best = this;
			closest = diff;
		}
	}
	if (best!= NULL)
		DPRINTF("found %s %d\n", best->name, best->dot_clock);

	return best;
}
