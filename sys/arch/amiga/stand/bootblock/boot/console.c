/* $NetBSD: console.c,v 1.3.26.2 2002/02/28 04:07:07 nathanw Exp $ */

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ignatios Souvatzis.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Bootblock support routines for Intuition console support.
 */

#include <sys/types.h>

#include <stand.h>
#include "samachdep.h"

#include "amigatypes.h"
#include "amigagraph.h"
#include "amigaio.h"
#include "libstubs.h"

const u_int32_t screentags[] = {
	SA_Type, CUSTOMSCREEN,
	SA_DisplayID, 0x8000,
	SA_ShowTitle, 0,
	SA_Quiet, 1,
	0
};

u_int32_t windowtags[] = {
	WA_CustomScreen, 0L,
	WA_Borderless, 1L,
	WA_Backdrop, 1L,
	WA_Activate, 1L,
	0
};

struct Console {
	int magic;
	struct AmigaIO *cnior;
	struct TimerIO *tmior;
	struct MsgPort *cnmp;
	struct Screen *s;
	struct Window *w;
} *ConsoleBase;
static struct Console myConsole;

u_int16_t timelimit;

int
consinit(void *consptr) {
	struct Console *mc;

	if (consptr != NULL) {
		/* Check magic? */
		ConsoleBase = consptr;		/* Use existing console */
		return (0);
	}

	mc = &myConsole;
	IntuitionBase = OpenLibrary("intuition.library", 36L);
	if (IntuitionBase == 0)
		goto err;

	mc->s = OpenScreenTagList(0, screentags);
	if (!mc->s)
		goto err;

	windowtags[1] = (u_int32_t)mc->s;
	mc->w = OpenWindowTagList(0, windowtags);
	if (!mc->w)
		goto err;

	mc->cnmp = CreateMsgPort();

	if (!mc->cnmp)
		goto err;

	mc->cnior = (struct AmigaIO *)CreateIORequest(mc->cnmp, sizeof(struct AmigaIO));
	if (!mc->cnior)
		goto err;

	mc->cnior->buf = (void *)mc->w;
	if (OpenDevice("console.device", 0, mc->cnior, 0))
		goto err;

	mc->tmior = (struct TimerIO *)CreateIORequest(mc->cnmp, sizeof(struct TimerIO));
	if (!mc->tmior)
		goto err;

	if (OpenDevice("timer.device", 0, (struct AmigaIO*)mc->tmior, 0))
		goto err;

	ConsoleBase = mc;
	return 0;

err:
#ifdef notyet
	if (mc->tmior)
		DeleteIORequest(mc->tmior);

	if (mc->cnior)
		DeleteIORequest(mc->cnior);

	if (mc->cnmp)
		DeleteMsgPort(mc->cnmp);

	if (mc->w)
		CloseWindow(mc->w);

	if (mc->s)
		CloseScreen(mc->s);
	if (IntuitionBase)
		CloseLibrary(IntuitionBase);
#endif

	return 1;
}

#ifdef _PRIMARY_BOOT
int
consclose()
{
	struct Console *mc = ConsoleBase;

	if (mc == NULL)
		return 0;
	if (mc->tmior) {
		CloseDevice((struct AmigaIO *)mc->tmior);
		DeleteIORequest(mc->tmior);
	}

	if (mc->cnior) {
		CloseDevice(mc->cnior);
		DeleteIORequest(mc->cnior);
	}

	if (mc->cnmp)
		DeleteMsgPort(mc->cnmp);

	if (mc->w)
		CloseWindow(mc->w);

	if (mc->s)
		CloseScreen(mc->s);
	if (IntuitionBase)
		CloseLibrary(IntuitionBase);
	ConsoleBase = NULL;
	return 0;
}
#endif

void
putchar(c)
	char c;
{
	struct Console *mc = ConsoleBase;

	mc->cnior->length = 1;
	mc->cnior->buf = &c;
	mc->cnior->cmd = Cmd_Wr;
	(void)DoIO(mc->cnior);
}

void
puts(s)
	char *s;
{
	struct Console *mc = ConsoleBase;

	mc->cnior->length = -1;
	mc->cnior->buf = s;
	mc->cnior->cmd = Cmd_Wr;
	(void)DoIO(mc->cnior);
}

int
getchar()
{
	struct AmigaIO *ior;
	char c = -1;
	struct Console *mc = ConsoleBase;

	mc->cnior->length = 1;
	mc->cnior->buf = &c;
	mc->cnior->cmd = Cmd_Rd;

	SendIO(mc->cnior);

	if (timelimit) {
		mc->tmior->cmd = Cmd_Addtimereq;
		mc->tmior->secs = timelimit;
		mc->tmior->usec = 2; /* Paranoid */
		SendIO((struct AmigaIO *)mc->tmior);

		ior = WaitPort(mc->cnmp);
		if (ior == mc->cnior)
			AbortIO((struct AmigaIO *)mc->tmior);
		else /* if (ior == mc->tmior) */ {
			AbortIO(mc->cnior);
			c = '\n';
		}
		WaitIO((struct AmigaIO *)mc->tmior);
		timelimit = 0;
	}
	(void)WaitIO(mc->cnior);
	return c;
}
