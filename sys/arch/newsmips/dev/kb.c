/*	$NetBSD: kb.c,v 1.4 1999/12/18 06:54:05 tsubai Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Sony Corp. and Kazumasa Utashiro of Software Research Associates, Inc.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 * from: $Hdr: kb.c,v 4.300 91/06/09 06:42:44 root Rel41 $ SONY
 *
 *	@(#)kb.c	8.1 (Berkeley) 6/11/93
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/systm.h>
#include <sys/uio.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/keyboard.h>

#include <newsmips/dev/kbreg.h>

#ifdef CPU_SINGLE
#include <newsmips/dev/scc.h>
#else
#include "../iop/iopvar.h"
#endif

struct kb_softc {
	struct device sc_dev;
	int sc_busy;
};

extern Key_table key_table[];
extern Key_table default_table[];
extern int country;

extern void kbd_init(void);
extern int kbd_ioctl(int, int, int *);
extern int kbd_write(int, char *, int);
extern void init_key_table(void);

static int	kbmatch __P((struct device *, struct cfdata *, void *));
static void	kbattach __P((struct device *, struct device *, void *));

static int kbintr __P((void *));
extern int kbopen(dev_t, int, int, struct proc *);
extern int kbclose(dev_t, int, int, struct proc *);
#ifdef KBDEBUG
extern int kbread(dev_t, struct uio *);
#endif
extern int kbwrite(dev_t, struct uio *);
extern int kbioctl(dev_t, int, caddr_t, int, struct proc *);
static int kbchange(Key_tab_info *);
static int kb_probe(void);
static void kb_attach(void);
static int kb_open(void);
static int kb_close(void);
#ifdef KBDEBUG
static int kb_read(struct uio *);
#endif
static int kb_write(struct uio *);
static int kb_ctrl(int, void *);

struct cfattach kb_ca = {
	sizeof(struct kb_softc), kbmatch, kbattach
};

extern struct cfdriver kb_cd;

#ifdef CPU_SINGLE
extern	Key_table *key_table_addr;
#endif

int
kbmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct confargs *ca = aux;

	if (strcmp(ca->ca_name, "kb"))
		return 0;

	return kb_probe();
}

void
kbattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	int intr = self->dv_cfdata->cf_level;

	kb_attach();
	init_key_table();

	if (intr == -1)
		intr = 2;	/* XXX */

	hb_intr_establish(intr, IPL_TTY, kbintr, self);
	printf(" level %d\n", intr);
}

int
kbintr(v)
	void *v;
{
	kbm_rint(SCC_KEYBOARD);
	return 1;
}

int
kbopen(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;

{
	struct kb_softc *sc;
	int unit = minor(dev);

	if (unit >= kb_cd.cd_ndevs)
		return ENXIO;
	sc = kb_cd.cd_devs[unit];
	if (sc == NULL)
		return ENXIO;

	if (sc->sc_busy)
		return EBUSY;
	/* need spl? */
	sc->sc_busy = 1;
	return kb_open();
}

int
kbclose(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	struct kb_softc *sc = kb_cd.cd_devs[minor(dev)];

	kb_close();
	sc->sc_busy = 0;
	return 0;
}

#ifdef KBDEBUG
int
kbread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	int error = 0;

	while (uio->uio_resid > 0)
		if (error = kb_read(uio))
			break;
	return error;
}
#endif /* KBDEBUG */

int
kbwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	int error = 0;

	while (uio->uio_resid > 0)
		if (error = kb_write(uio))
			break;

	return error;
}

int
kbioctl(dev, cmd, data, flag, p)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	int error = 0;

	switch (cmd) {

	case KBIOCBELL:
		error = kb_ctrl(KIOCBELL, (int *)data);
		break;

	case KBIOCREPT:
		error = kb_ctrl(KIOCREPT, 0);
		break;

	case KBIOCNRPT:
		error = kb_ctrl(KIOCNRPT, 0);
		break;

	case KBIOCSETLOCK:
		error = kb_ctrl(KIOCSETLOCK, (int *)data);
		break;

	case KBIOCSETTBL:
		error = kbchange((Key_tab_info *)data);
		break;

	case KBIOCGETCNUM:
		error = kb_ctrl(KIOCGETCNUM, (int *)data);
		break;

	case KBIOCSETCNUM:
		error = kb_ctrl(KIOCSETCNUM, (int *)data);
		break;

	case KBIOCGETSTAT:
		error = kb_ctrl(KIOCGETSTAT, (int *)data);
		break;

	case KBIOCSETSTAT:
		error = kb_ctrl(KIOCSETSTAT, (int *)data);
		break;

	default:
		return EIO;
	}
	return error;
}

int
kbchange(argp)
	Key_tab_info *argp;
{
	int keynumber;

	keynumber = ((Key_tab_info *)argp)->key_number;
	if (keynumber < 0 || keynumber > N_KEY)
		return (EINVAL);

	key_table[keynumber] = argp->key_num_table;
	return (kb_ctrl(KIOCCHTBL, (Key_table *)key_table));
}

/*
 * Machine dependent functions
 *
 *	kb_probe()
 *	kb_attach()
 *	kb_open()
 *	kb_close()
 *	kb_write()
 *	kb_ctrl()
 */

#ifdef CPU_SINGLE
extern int tty00_is_console;
extern Key_table *key_table_addr;
#define KBPRI	(PZERO+1)

int
kb_probe()
{
	return 1;
}

void
kb_attach()
{
	kbd_init();
	kbd_ioctl(0, KIOCSETCNUM, &country);
}

int
kb_open()
{
	if (tty00_is_console)
		kbm_open(SCC_KEYBOARD);
	return 0;
}

int
kb_close()
{
	if (tty00_is_console)
		kbm_close(SCC_KEYBOARD);
	return 0;
}

#ifdef KBDEBUG
int
kb_read(uio)
	struct uio *uio;
{
	int n;
	char buf[32];

	n = kbd_read_raw(SCC_KEYBOARD, buf, min(uio->uio_resid, sizeof (buf)));
	if (n == 0)
		return (0);
	return (uiomove((caddr_t)buf, n, uio));
}
#endif /* KBDEBUG */

int
kb_write(uio)
	struct uio *uio;
{
	int n, error;
	char buf[32];

	n = min(sizeof(buf), uio->uio_resid);
	if (error = uiomove((caddr_t)buf, n, uio))
		return (error);
	kbd_write(SCC_KEYBOARD, buf, n);
	return (0);
}

int
kb_ctrl(func, arg)
	int func;
	void *arg;
{
	return (kbd_ioctl(0, func, arg));
}
#endif /* CPU_SINGLE */

#ifdef IPC_MRX
#include "../ipc/newsipc.h"
#include "../mrx/h/kbms.h"

#ifdef news3800
#define ipc_phys(x)	(caddr_t)(K0_TT0(x))
#endif

int	port_kboutput, port_kboutput_iop, port_kbctrl, port_kbctrl_iop;

kb_probe(ii)
	struct iop_device *ii;
{
	port_kboutput_iop = object_query("kbd_output");
	port_kbctrl_iop = object_query("kbd_io");
	if (port_kboutput_iop <= 0 || port_kbctrl_iop <= 0)
		return (0);
	port_kboutput = port_create("@kboutput", NULL, 0);
	port_kbctrl = port_create("@kbctrl", NULL, 0);
	return (1);
}

kb_attach(ii)
	struct iop_device *ii;
{
	(void) kb_ctrl(KIOCCHTBL, (Key_table *)key_table);
	(void) kb_ctrl(KIOCSETCNUM, &country);
}

kb_open()
{
	return (0);
}

kb_close()
{
	return (0);
}

#ifdef KBDEBUG
kb_read(uio)
	struct uio *uio;
{
	char *addr;
	int len;
	int error;

	len = uio->uio_resid;
	msg_send(port_kbinput_iop, port_kbinput, &len, sizeof(len), 0);
	msg_recv(port_kbinput, NULL, &addr, &len, 0);
	error = uiomove(addr, len, UIO_READ, uio);
	msg_free(port_kbinput);
	return (error);
}
#endif /* KBDEBUG */

kb_write(uio)
	struct uio *uio;
{
	int len;
	int error;
	char buf[MAX_CIO];

	len = min(MAX_CIO, uio->uio_resid);
	if (error = uiomove(buf, len, UIO_WRITE, uio))
		return (error);
	msg_send(port_kboutput_iop, port_kboutput, buf, len, 0);
	msg_recv(port_kboutput, NULL, NULL, NULL, 0);
	msg_free(port_kboutput);
	return (0);
}

kb_ctrl(func, arg)
	int func;
	int *arg;
{
	struct kb_ctrl_req req;
	int *reply, result;
	static int tmp;

	if (func == KIOCCHTBL || func == KIOCOYATBL)
		req.kb_arg = (int)ipc_phys(arg);
	else if (arg == NULL)
		req.kb_arg = (int)&tmp;
	else
		req.kb_arg = *arg;
	req.kb_func = func;
	msg_send(port_kbctrl_iop, port_kbctrl, &req, sizeof(req), 0);
	msg_recv(port_kbctrl, NULL, &reply, NULL, 0);
	result = *reply;
	msg_free(port_kbctrl);
	switch (func) {

	case KIOCGETCNUM:
	case KIOCGETSTAT:
		if (arg)
			*(int *)arg = result;
	}
	return (0);
}
#endif /* IPC_MRX */
