/*	$NetBSD: btnmgr.c,v 1.2 2000/02/27 16:34:13 uch Exp $	*/

/*-
 * Copyright (c) 1999
 *         Shin Takemura and PocketBSD Project. All rights reserved.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#define BTNMGRDEBUG

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/kernel.h>

#include <machine/config_hook.h>

#ifdef BTNMGRDEBUG
int	btnmgr_debug = 0;
#define	DPRINTF(arg) if (btnmgr_debug) printf arg;
#define	DPRINTFN(n, arg) if (btnmgr_debug > (n)) printf arg;
#else
#define	DPRINTF(arg)
#define DPRINTFN(n, arg)
#endif

cdev_decl(btnmgr);

struct btnmgr_softc {
	config_hook_tag	sc_hook_tag;
};

static struct btnmgr_softc *the_btnmgr_sc;

void		btnmgrattach  __P((int));
char*		btnmgr_name __P((long));
static int	btnmgr_hook __P((void *, int, long, void *));

static const struct {
	long id;
	char *name;
} button_names[] = {
	{ CONFIG_HOOK_BUTTONEVENT_POWER,	"Power"		},
	{ CONFIG_HOOK_BUTTONEVENT_OK,		"OK"		},
	{ CONFIG_HOOK_BUTTONEVENT_CANCEL,	"Cancel"	},
	{ CONFIG_HOOK_BUTTONEVENT_UP,		"Up"		},
	{ CONFIG_HOOK_BUTTONEVENT_DOWN,		"Down"		},
	{ CONFIG_HOOK_BUTTONEVENT_REC,		"Rec"		},
	{ CONFIG_HOOK_BUTTONEVENT_COVER,	"Cover"		},
	{ CONFIG_HOOK_BUTTONEVENT_LIGHT,	"Light"		},
	{ CONFIG_HOOK_BUTTONEVENT_CONTRAST,	"Contrast"	},
	{ CONFIG_HOOK_BUTTONEVENT_APP0,		"Application 0"	},
	{ CONFIG_HOOK_BUTTONEVENT_APP1,		"Application 1"	},
	{ CONFIG_HOOK_BUTTONEVENT_APP2,		"Application 2"	},
	{ CONFIG_HOOK_BUTTONEVENT_APP3,		"Application 3"	},
};
static const int n_button_names = 
	sizeof(button_names) / sizeof(*button_names);

void
btnmgrattach (n)
	int n;
{
	int id;

	printf("button manager\n");

	the_btnmgr_sc = malloc(sizeof(struct btnmgr_softc), M_DEVBUF,
			       cold ? M_NOWAIT : M_WAITOK);
	if (the_btnmgr_sc == NULL)
		panic("button manager: malloc failed");
	bzero(the_btnmgr_sc, sizeof(struct btnmgr_softc));

	for (id = 0; id < 16; id++)
		the_btnmgr_sc->sc_hook_tag =
			config_hook(CONFIG_HOOK_BUTTONEVENT,
				    id, CONFIG_HOOK_SHARE,
				    btnmgr_hook,
				    the_btnmgr_sc);
}

static int
btnmgr_hook(ctx, type, id, msg)
	void *ctx;
	int type;
	long id;
	void *msg;
{
#ifdef notyet
	struct btnmgr_softc *sc = ctx;
#endif /* not yet */

	DPRINTF(("%s button: %s\n", btnmgr_name(id), msg ? "ON" : "OFF"));

	return (0);
}

char*
btnmgr_name(id)
	long id;
{
	int i;

	for (i = 0; i < n_button_names; i++)
		if (button_names[i].id == id)
			return (button_names[i].name);
	return ("unknown");
}

#ifdef notyet
int
btnmgropen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	return (EINVAL);
}

int
btnmgrclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	return (EINVAL);
}

int
btnmgrread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	return (EINVAL);
}

int
btnmgrwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	return (EINVAL);
}

int
btnmgrioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	return (EINVAL);
}
#endif /* notyet */
