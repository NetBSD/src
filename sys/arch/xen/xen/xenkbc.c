/* $NetBSD: xenkbc.c,v 1.1 2004/04/24 21:33:32 cl Exp $ */

/*
 *
 * Copyright (c) 2004 Christian Limpach.
 * All rights reserved.
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
 *      This product includes software developed by Christian Limpach.
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
 */


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xenkbc.c,v 1.1 2004/04/24 21:33:32 cl Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <dev/pckbport/pckbportvar.h>
#include <dev/ic/i8042reg.h>

#include <machine/intr.h>

#include <machine/xenkbcvar.h>
#include <machine/xen.h>
#include <machine/hypervisor.h>
#include <machine/hypervisor-ifs/kbd.h>
#include <machine/events.h>

#define	KBC_DELAY	DELAY(1000)
#define	KBC_TIMEOUT	250

struct xenkbc_internal {
	struct xenkbc_softc	*xi_sc;
	struct pckbport_tag	*xi_pt;
	pckbport_slot_t		xi_slot;
	int			xi_flags;
	int			xi_scancode;
};

#define	XI_CONSOLE_FLAG		0x01

#define	XI_CONSOLE(xi)		((xi)->xi_flags & XI_CONSOLE_FLAG)

#define	XI_SETCONSOLE(xi,on)	\
	((on) ? ((xi)->xi_flags |= XI_CONSOLE_FLAG) : \
		((xi)->xi_flags &= ~XI_CONSOLE_FLAG))

static int xenkbc_match(struct device *, struct cfdata *, void *);
static void xenkbc_attach(struct device *, struct device *, void *);

static int xenkbc_xt_translation(void *, pckbport_slot_t, int);
static int xenkbc_send_devcmd(void *, pckbport_slot_t, u_char);
static int xenkbc_poll_data1(void *, pckbport_slot_t);
static void xenkbc_slot_enable(void *, pckbport_slot_t, int);
static void xenkbc_intr_establish(void *, pckbport_slot_t);
static void xenkbc_set_poll(void *, pckbport_slot_t, int);

static int xenkbc_intr(void *);

CFATTACH_DECL(xenkbc, sizeof(struct xenkbc_softc),
    xenkbc_match, xenkbc_attach, NULL, NULL);

static struct pckbport_accessops const xenkbc_ops = {
	xenkbc_xt_translation,
	xenkbc_send_devcmd,
	xenkbc_poll_data1,
	xenkbc_slot_enable,
	xenkbc_intr_establish,
	xenkbc_set_poll
};

static struct xenkbc_internal xenkbc_cntag;

#if 0
#define	DPRINTF(x) printf x
#else
#define	DPRINTF(x)
#endif


static int
xenkbc_getstatus(struct xenkbc_internal *xi)
{
	long res;

	res = HYPERVISOR_kbd_op(KBD_OP_READ, 0);
	if (res < 0) {
		xi->xi_scancode = 0;
		return 0;
	}
	xi->xi_scancode = KBD_CODE_SCANCODE(res);
	return KBD_CODE_STATUS(res);
}

static int
xenkbc_wait_output(struct xenkbc_internal *xi)
{
	u_int i;

	for (i = KBC_TIMEOUT; i; i--) {
		if ((xenkbc_getstatus(xi) & KBS_IBF) == 0)
			return (1);
		KBC_DELAY;
	}
	return (0);
}

static int
xenkbc_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct xenkbc_attach_args *xa = aux;

	if (strcmp(xa->xa_device, "xenkbc"))
		return 0;

	return 1;
}

static void
xenkbc_attach(struct device *parent, struct device *self, void *aux)
{
	/*  struct xenkbc_attach_args *xa = aux; */
	struct xenkbc_softc *xs = (struct xenkbc_softc *)self;
	struct xenkbc_internal *xi;

	if (XI_CONSOLE(&xenkbc_cntag))
		xi = &xenkbc_cntag;
	else {
		xi = malloc(sizeof(struct xenkbc_internal), M_DEVBUF,
		    M_NOWAIT | M_ZERO);
		if (xi == NULL) {
			aprint_error(": no memory");
			return;
		}
		xi->xi_slot = PCKBPORT_KBD_SLOT;
	}

	aprint_normal(": Xen Keyboard Device\n");

	xs->sc_xi = xi;
	xi->xi_sc = xs;

	event_set_handler(_EVENT_PS2, &xenkbc_intr, xi, IPL_TTY);
	hypervisor_enable_event(_EVENT_PS2);

	xi->xi_pt = pckbport_attach(xi, &xenkbc_ops);

	/* flush */
	xenkbc_poll_data1(xi, PCKBPORT_KBD_SLOT);

	pckbport_attach_slot(&xs->sc_dev, xi->xi_pt, PCKBPORT_KBD_SLOT);
}

static int
xenkbc_send_devcmd(void *cookie, pckbport_slot_t slot, u_char cmd)
{
	struct xenkbc_internal *xi = cookie;

	DPRINTF(("devcmd %x\n", cmd));
	xenkbc_wait_output(xi);
	return !HYPERVISOR_kbd_op(KBD_OP_WRITEOUTPUT, cmd);
}

static int
xenkbc_poll_data1(void *cookie, pckbport_slot_t slot)
{
	struct xenkbc_internal *xi = cookie;
	int s;
	u_char stat, c;
	int i = 1000;

	s = splhigh();

	DELAY(10);
	for (; i; i--) {
		stat = xenkbc_getstatus(xi);
		if (stat & KBS_DIB) {
			c = xi->xi_scancode;
			DELAY(10);
			splx(s);
			DPRINTF(("poll -> %x stat %x\n", c, stat));
			return c;
		}
	}

	DPRINTF(("poll -> -1\n"));
	splx(s);
	return -1;
}

/*
 * switch scancode translation on / off
 * return nonzero on success
 */
static int
xenkbc_xt_translation(void *cookie, pckbport_slot_t slot, int on)
{

	if (on)
		return 0; /* Can't do XT translation */
	else
		return 1;
}

static void
xenkbc_slot_enable(void *cookie, pckbport_slot_t slot, int on)
{

}


static void
xenkbc_intr_establish(void *cookie, pckbport_slot_t slot)
{

}

static void
xenkbc_set_poll(void *cookie, pckbport_slot_t slot, int on)
{

}

static int
xenkbc_intr(void *self)
{
	struct xenkbc_internal *xi = self;
	int status;

	status = xenkbc_getstatus(xi);
	if (status == 0)
		return 0;

	pckbportintr(xi->xi_pt, xi->xi_slot, xi->xi_scancode);
	return 1;
}

int
xenkbc_cnattach(pckbport_slot_t slot)
{
	struct xenkbc_internal *xi = &xenkbc_cntag;

	XI_SETCONSOLE(xi, 1);
	xi->xi_slot = slot;

	return pckbport_cnattach(xi, &xenkbc_ops, slot);
}
