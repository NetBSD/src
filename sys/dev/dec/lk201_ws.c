/* $NetBSD: lk201_ws.c,v 1.1 1998/09/17 20:01:57 drochner Exp $ */

/*
 * Copyright (c) 1998
 *	Matthias Drochner.  All rights reserved.
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
 *	This product includes software developed for the NetBSD Project
 *	by Matthias Drochner.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 *
 */

#include <sys/param.h>
#include <sys/systm.h>

#include <dev/wscons/wsconsio.h>

#include <dev/dec/lk201reg.h>
#include <dev/dec/lk201var.h>
#include <dev/dec/wskbdmap_lk201.h> /* for {MIN,MAX}_LK201_KEY */

void
lk201_init_keystate(lks)
	struct lk201_state *lks;
{
	int i;

	for (i = 0; i < LK_KLL; i++)
		lks->down_keys_list[i] = -1;
}

int
lk201_decode(lks, datain, type, dataout)
	struct lk201_state *lks;
	int datain;
	u_int *type;
	int *dataout;
{
	int i, freeslot;

	switch (datain) {
	    case LK_KEY_UP:
		for (i = 0; i < LK_KLL; i++)
			lks->down_keys_list[i] = -1;
		*type = WSCONS_EVENT_ALL_KEYS_UP;
		return (1);
	    case LK_POWER_UP:
		printf("lk201_decode: powerup detected\n");
		/* XXX should reinitialize here */
		return (0);
	    case LK_KDOWN_ERROR:
	    case LK_POWER_ERROR:
	    case LK_OUTPUT_ERROR:
	    case LK_INPUT_ERROR:
		printf("lk201_decode: error %x\n", datain);
		/* FALLTHRU */
	    case LK_KEY_REPEAT: /* autorepeat handled by wskbd */
	    case LK_MODE_CHANGE: /* ignore silently */
		return (0);
	}

	if (datain < MIN_LK201_KEY || datain > MAX_LK201_KEY) {
		printf("lk201_decode: %x\n", datain);
		return (0);
	}

	*dataout = datain - MIN_LK201_KEY;

	freeslot = -1;
	for (i = 0; i < LK_KLL; i++) {
		if (lks->down_keys_list[i] == datain) {
			*type = WSCONS_EVENT_KEY_UP;
			lks->down_keys_list[i] = -1;
			return (1);
		}
		if (lks->down_keys_list[i] == -1 && freeslot == -1)
			freeslot = i;
	}

	if (freeslot == -1) {
		printf("lk201_decode: down(%d) no free slot\n", datain);
		return (0);
	}

	*type = WSCONS_EVENT_KEY_DOWN;
	lks->down_keys_list[freeslot] = datain;
	return (1);
}
