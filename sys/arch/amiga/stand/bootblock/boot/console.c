/* $NetBSD: console.c,v 1.13.22.1 2017/12/03 11:35:49 jdolecek Exp $ */

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

#include <lib/libsa/stand.h>
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

#ifdef SERCONSOLE
static int use_serconsole;
extern char default_command[];

static void
conspreinit(void)
{
	char *p = default_command;
	char c;

	/*
	 * preparse the default command to check for -C option
	 * that selects the serial console
	 */
	while ((c = *p)) {
		while (c == ' ')
			c = *++p;
		if (c == '-') {
			while ((c = *++p) && c != ' ') {
				switch (c) {
				case 'C':
					use_serconsole = 1;
					break;
				}
			}
		} else {
			while ((c = *++p) && c != ' ')
				;
		}
	}
}
#endif

int
consinit(void *consptr) {
	struct Console *mc;

	if (consptr != NULL) {
		/* Check magic? */
		mc = consptr;		/* Use existing console */
		goto done;
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

done:

#ifdef SERCONSOLE
	conspreinit();
	if (use_serconsole)
		RawIOInit();
#endif

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
consclose(void)
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
putchar(int c)
{
	struct Console *mc = ConsoleBase;
	char buf = c;

	mc->cnior->length = 1;
	mc->cnior->buf = &buf;
	mc->cnior->cmd = Cmd_Wr;

#ifdef SERCONSOLE
	if (use_serconsole)
		RawPutChar((int32_t)c);
#endif

	(void)DoIO(mc->cnior);
}

void
puts(char *s)
{
	struct Console *mc = ConsoleBase;

	mc->cnior->length = -1;
	mc->cnior->buf = s;
	mc->cnior->cmd = Cmd_Wr;

#ifdef SERCONSOLE
	if (use_serconsole) {
		while (*s)
			RawPutChar(*s++);
	}
#endif

	(void)DoIO(mc->cnior);
}

int
getchar(void)
{
	struct AmigaIO *ior;
	char c = '\n';
	struct Console *mc = ConsoleBase;
	unsigned long ticks;
#ifdef SERCONSOLE
	int32_t r;
#endif

	mc->cnior->length = 1;
	mc->cnior->buf = &c;
	mc->cnior->cmd = Cmd_Rd;

	SendIO(mc->cnior);

	ticks = 10 * timelimit;
	do {
		if (timelimit == 0)
			ticks = 2;

		mc->tmior->cmd = Cmd_Addtimereq;
		mc->tmior->secs = 0;
		mc->tmior->usec = 100000;
		SendIO((struct AmigaIO *)mc->tmior);

		ior = WaitPort(mc->cnmp);
		if (ior == mc->cnior) {
			AbortIO((struct AmigaIO *)mc->tmior);
			ticks = 1;
		} else /* if (ior == mc->tmior) */ {
#ifdef SERCONSOLE
			if (use_serconsole) {
				r = RawMayGetChar();
				if (r != -1) {
					c = r;
					ticks = 1;
				}
			}
#endif
			if (ticks == 1)
				AbortIO((struct AmigaIO *)mc->cnior);
		}
		WaitIO((struct AmigaIO *)mc->tmior);

		--ticks;
	} while (ticks != 0);
	timelimit = 0;

	(void)WaitIO(mc->cnior);
	return c;
}
