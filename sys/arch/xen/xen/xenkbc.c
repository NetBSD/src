/* $NetBSD: xenkbc.c,v 1.3 2004/04/26 19:44:54 cl Exp $ */

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

/*
 * Copyright (c) 2004 Ben Harris.
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
__KERNEL_RCSID(0, "$NetBSD: xenkbc.c,v 1.3 2004/04/26 19:44:54 cl Exp $");

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

#define	XENKBC_NSLOTS	2

/* data per slave device */
struct xenkbc_slotdata {
	int xsd_polling;	/* don't process data in interrupt handler */
	int xsd_poll_data;	/* data read from inr handler if polling */
	int xsd_poll_stat;	/* status read from inr handler if polling */
#if NRND > 0
	rndsource_element_t	xsd_rnd_source;
#endif
};

struct xenkbc_internal {
	struct xenkbc_softc	*xi_sc;
	struct pckbport_tag	*xi_pt;
	struct xenkbc_slotdata	*xi_slotdata[XENKBC_NSLOTS];
	int			xi_flags;
	int			xi_data;
	int			xi_8042cmdbyte;
};

#define	XI_CONSOLE_FLAG		0x01
#define	XI_HASAUX_FLAG		0x02

#define	XI_CONSOLE(xi)		((xi)->xi_flags & XI_CONSOLE_FLAG)
#define	XI_HASAUX(xi)		((xi)->xi_flags & XI_HASAUX_FLAG)

#define	XI_SETCONSOLE(xi,on)	\
	((on) ? ((xi)->xi_flags |= XI_CONSOLE_FLAG) : \
		((xi)->xi_flags &= ~XI_CONSOLE_FLAG))
#define	XI_SETHASAUX(xi,on)	\
	((on) ? ((xi)->xi_flags |= XI_HASAUX_FLAG) : \
		((xi)->xi_flags &= ~XI_HASAUX_FLAG))

static int xenkbc_match(struct device *, struct cfdata *, void *);
static void xenkbc_attach(struct device *, struct device *, void *);

static int xenkbc_xt_translation(void *, pckbport_slot_t, int);
static void xenkbc_init_slotdata(struct xenkbc_slotdata *);

static int xenkbc_get8042cmd (struct xenkbc_internal *);
static int xenkbc_put8042cmd (struct xenkbc_internal *);
static int xenkbc_send_devcmd(void *, pckbport_slot_t, u_char);
static int xenkbc_send_cmd(void *, u_char);
static int xenkbc_send_data(void *, u_char);
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

static struct xenkbc_internal xenkbc_consdata;
static struct xenkbc_slotdata xenkbc_cons_slotdata;

/*  #define XENKBCDEBUG */
#ifdef XENKBCDEBUG
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
		xi->xi_data = 0;
		return 0;
	}
	xi->xi_data = KBD_CODE_SCANCODE(res);
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

	if ((xen_start_info.flags & SIF_PRIVILEGED) == 0)
		return 0;

	if (strcmp(xa->xa_device, "xenkbc"))
		return 0;

	return 1;
}

static int
xenkbc_attach_slot(struct xenkbc_softc *xs, pckbport_slot_t slot)
{
	struct xenkbc_internal *xi = xs->sc_xi;
	struct device *child;
	int alloced = 0;

	if (xi->xi_slotdata[slot] == NULL) {
		xi->xi_slotdata[slot] = malloc(sizeof(struct xenkbc_slotdata),
		    M_DEVBUF, M_NOWAIT);
		if (xi->xi_slotdata[slot] == NULL) {
			printf("%s: no memory\n", xs->sc_dev.dv_xname);
			return 0;
		}
		xenkbc_init_slotdata(xi->xi_slotdata[slot]);
		alloced++;
	}

	child = pckbport_attach_slot(&xs->sc_dev, xi->xi_pt, slot);

	if (child == NULL && alloced) {
		free(xi->xi_slotdata[slot], M_DEVBUF);
		xi->xi_slotdata[slot] = NULL;
	}

#if NRND > 0
	   if (child != NULL && xi->xi_slotdata[slot] != NULL)
		   rnd_attach_source(&xi->xi_slotdata[slot]->xsd_rnd_source,
		       child->dv_xname, RND_TYPE_TTY, 0);
#endif

	return child != NULL;
}

static void
xenkbc_attach(struct device *parent, struct device *self, void *aux)
{
	/*  struct xenkbc_attach_args *xa = aux; */
	struct xenkbc_softc *xs = (struct xenkbc_softc *)self;
	struct xenkbc_internal *xi;
	int res;
	u_char cmdbits = 0;

	if (XI_CONSOLE(&xenkbc_consdata))
		xi = &xenkbc_consdata;
	else {
		xi = malloc(sizeof(struct xenkbc_internal), M_DEVBUF,
		    M_NOWAIT | M_ZERO);
		if (xi == NULL) {
			aprint_error(": no memory\n");
			return;
		}
		xi->xi_8042cmdbyte = KC8_CPU;
	}

	aprint_normal(": Xen Keyboard/Mouse Device\n");

	xs->sc_xi = xi;
	xi->xi_sc = xs;

	event_set_handler(_EVENT_PS2, &xenkbc_intr, xi, IPL_TTY);
	hypervisor_enable_event(_EVENT_PS2);

	xi->xi_pt = pckbport_attach(xi, &xenkbc_ops);

	/* flush */
	xenkbc_poll_data1(xi, PCKBPORT_KBD_SLOT);

	/* set initial cmd byte */
	if (!xenkbc_put8042cmd(xi)) {
		printf("kbc: cmd word write error\n");
		return;
	}

	if (xenkbc_attach_slot(xs, PCKBPORT_KBD_SLOT))
		cmdbits |= KC8_KENABLE;

	/*
	 * Check aux port ok.
	 */
	if (!xenkbc_send_cmd(xi, KBC_AUXECHO)) {
		printf("kbc: aux echo error 1\n");
		goto nomouse;
	}
	if (!xenkbc_wait_output(xi)) {
		printf("kbc: aux echo error 2\n");
		goto nomouse;
	}
	XI_SETHASAUX(xi, 1);
	xenkbc_send_data(xi, 0x5a); /* a random value */
	res = xenkbc_poll_data1(xi, PCKBPORT_AUX_SLOT);
	if (res != -1) {
		/*
		 * In most cases, the 0x5a gets echoed.
		 * Some older controllers (Gateway 2000 circa 1993)
		 * return 0xfe here.
		 * We are satisfied if there is anything in the
		 * aux output buffer.
		 */
		if (xenkbc_attach_slot(xs, PCKBPORT_AUX_SLOT))
			cmdbits |= KC8_MENABLE;
	} else {
#ifdef XENKBCDEBUG
		printf("kbc: aux echo test failed\n");
#endif
		XI_SETHASAUX(xi, 0);
	}

 nomouse:
	/* enable needed interrupts */
	xi->xi_8042cmdbyte |= cmdbits;
	if (!xenkbc_put8042cmd(xi))
		printf("kbc: cmd word write error\n");
}

static void
xenkbc_init_slotdata(struct xenkbc_slotdata *xsd)
{

	xsd->xsd_polling = 0;
}

/*
 * Get the current command byte.
 */
static int
xenkbc_get8042cmd(struct xenkbc_internal *xi)
{
	int data;

	if (!xenkbc_send_cmd(xi, K_RDCMDBYTE))
		return 0;
	data = xenkbc_poll_data1(xi, PCKBPORT_KBD_SLOT);
	if (data == -1)
		return 0;
	xi->xi_8042cmdbyte = data;
	return 1;
}

/*
 * Pass command byte to keyboard controller (8042).
 */
static int
xenkbc_put8042cmd(struct xenkbc_internal *xi)
{

	if (!xenkbc_send_cmd(xi, K_LDCMDBYTE))
		return 0;
	if (!xenkbc_wait_output(xi))
		return 0;
	return xenkbc_send_data(xi, xi->xi_8042cmdbyte);
}

static int
xenkbc_send_devcmd(void *cookie, pckbport_slot_t slot, u_char devcmd)
{

	DPRINTF(("send_devcmd %x\n", devcmd));

	if (slot == PCKBPORT_AUX_SLOT) {
		if (!xenkbc_send_cmd(cookie, KBC_AUXWRITE)) {
			DPRINTF(("xenkbc_send_devcmd: KBC_AUXWRITE failed\n"));
			return 0;
		}
	}
	if (!xenkbc_wait_output(cookie)) {
		DPRINTF(("xenkbc_send_devcmd: wait_output failed\n"));
		return 0;
	}
	return xenkbc_send_data(cookie, devcmd);
}

static int
xenkbc_send_cmd(void *cookie, u_char cmd)
{
	struct xenkbc_internal *xi = cookie;

	DPRINTF(("send_cmd %x\n", cmd));
	xenkbc_wait_output(xi);
	return !HYPERVISOR_kbd_op(KBD_OP_WRITECOMMAND, cmd);
}

static int
xenkbc_send_data(void *cookie, u_char output)
{
	struct xenkbc_internal *xi = cookie;

	DPRINTF(("send_data %x\n", output));
	xenkbc_wait_output(xi);
	return !HYPERVISOR_kbd_op(KBD_OP_WRITEOUTPUT, output);
}

static int
xenkbc_poll_data1(void *cookie, pckbport_slot_t slot)
{
	struct xenkbc_internal *xi = cookie;
	struct xenkbc_slotdata *xsd = xi->xi_slotdata[slot];
	int s;
	u_char stat, c;
	int i = 1000;

	s = splhigh();

	if (xsd && xsd->xsd_polling && xsd->xsd_poll_data != -1 &&
	    xsd->xsd_poll_stat != -1) {
		stat = xsd->xsd_poll_stat;
		c = xsd->xsd_poll_data;
		xsd->xsd_poll_data = -1;
		xsd->xsd_poll_stat = -1;
		goto process;
	}

	DELAY(10);
	for (; i; i--) {
		stat = xenkbc_getstatus(xi);
		if (stat & KBS_DIB) {
			c = xi->xi_data;
			DELAY(10);
		process:
			if (XI_HASAUX(xi) && (stat & 0x20)) { /* aux data */
				if (slot != PCKBPORT_AUX_SLOT) {
#ifdef XENKBCDEBUG
					printf("lost aux 0x%x\n", c);
#endif
					continue;
				}
			} else {
				if (slot == PCKBPORT_AUX_SLOT) {
#ifdef XENKBCDEBUG
					printf("lost kbd 0x%x\n", c);
#endif
					continue;
				}
			}
			splx(s);
			DPRINTF(("poll -> %x stat %x\n", c, stat));
			return c;
		}
	}

	DPRINTF(("poll failed -> -1\n"));
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
	struct xenkbc_internal *xi = cookie;
	int ison;

	if (slot != PCKBPORT_KBD_SLOT) {
		/* translation only for kbd slot */
		if (on)
			return 0;
		else
			return 1;
	}

	ison = xi->xi_8042cmdbyte & KC8_TRANS;
	if ((on && ison) || (!on && !ison))
		return 1;

	xi->xi_8042cmdbyte ^= KC8_TRANS;
	if (!xenkbc_put8042cmd(xi))
		return 0;

	/* read back to be sure */
	if (!xenkbc_get8042cmd(xi))
		return 0;

	ison = xi->xi_8042cmdbyte & KC8_TRANS;
	if ((on && ison) || (!on && !ison))
		return 1;
	return 0;
}

static const struct xenkbc_portcmd {
	u_char cmd_en, cmd_dis;
} xenkbc_portcmd[2] = {
	{
		KBC_KBDENABLE, KBC_KBDDISABLE,
	}, {
		KBC_AUXENABLE, KBC_AUXDISABLE,
	}
};

static void
xenkbc_slot_enable(void *cookie, pckbport_slot_t slot, int on)
{
	struct xenkbc_internal *xi = cookie;
	const struct xenkbc_portcmd *cmd;

	cmd = &xenkbc_portcmd[slot];

	DPRINTF(("slot enable %d -> %d\n", slot, on));
	xenkbc_send_cmd(xi, on ? cmd->cmd_en : cmd->cmd_dis);
}


static void
xenkbc_intr_establish(void *cookie, pckbport_slot_t slot)
{

}

static void
xenkbc_set_poll(void *cookie, pckbport_slot_t slot, int on)
{
	struct xenkbc_internal *xi = cookie;

	DPRINTF(("xenkbc_set_poll %d -> %d\n", slot, on));

	xi->xi_slotdata[slot]->xsd_polling = on;

	if (on) {
		xi->xi_slotdata[slot]->xsd_poll_data = -1;
		xi->xi_slotdata[slot]->xsd_poll_stat = -1;
	} else {
                int s;

                /*
                 * If disabling polling on a device that's been configured,
                 * make sure there are no bytes left in the FIFO, holding up
                 * the interrupt line.  Otherwise we won't get any further
                 * interrupts.
                 */
		s = spltty();
		xenkbc_intr(xi);
		splx(s);
	}
}

static int
xenkbc_intr(void *self)
{
	struct xenkbc_internal *xi = self;
	u_char stat;
	pckbport_slot_t slot;
	struct xenkbc_slotdata *xsd;
	int served = 0;

	for (;;) {
		stat = xenkbc_getstatus(xi);
		if (!(stat & KBS_DIB))
			break;

		served = 1;

		slot = (XI_HASAUX(xi) && (stat & 0x20)) ?
			PCKBPORT_AUX_SLOT : PCKBPORT_KBD_SLOT;
		xsd = xi->xi_slotdata[slot];

		if (xsd == NULL)
			continue;

#if NRND > 0
		rnd_add_uint32(&xsd->xsd_rnd_source,
		    (stat << 8) | xi->xi_data);
#endif

		if (xsd->xsd_polling) {
			xsd->xsd_poll_data = xi->xi_data;
			xsd->xsd_poll_stat = stat;
			break; /* xenkbc_poll_data() will get it */
		}

		pckbportintr(xi->xi_pt, slot, xi->xi_data);
	}

	return served;
}

int
xenkbc_cnattach(pckbport_slot_t slot)
{
	struct xenkbc_internal *xi = &xenkbc_consdata;
	int ret;

	/* flush */
	(void) xenkbc_poll_data1(xi, PCKBPORT_KBD_SLOT);

	/* init cmd byte, enable ports */
	xenkbc_consdata.xi_8042cmdbyte = KC8_CPU;
	if (!xenkbc_put8042cmd(xi)) {
		printf("kbc: cmd word write error\n");
		return EIO;
	}

	ret = pckbport_cnattach(xi, &xenkbc_ops, slot);

	xi->xi_slotdata[slot] = &xenkbc_cons_slotdata;
	xenkbc_init_slotdata(xi->xi_slotdata[slot]);
	XI_SETCONSOLE(xi, 1);

	return ret;
}
