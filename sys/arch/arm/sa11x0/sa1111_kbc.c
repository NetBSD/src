/*      $NetBSD: sa1111_kbc.c,v 1.2 2003/07/15 00:24:50 lukem Exp $ */

/*
 * Copyright (c) 2002  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Genetec Corporation may not be used to endorse or 
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Driver for keyboard controller in SA-1111 companion chip.
 *
 * PC keyboard driver (sys/dev/pckbc/pckbd.c) works only with 8042
 * keyboard controller driver (sys/dev/ic/pckbc.c).  This file
 * provides same functions as those of 8042 driver.
 *
 * XXX: we need cleaner interface between the keyboard driver and
 *      keyboard controller drivers.
 */
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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sa1111_kbc.c,v 1.2 2003/07/15 00:24:50 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/callout.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/queue.h>
#include <sys/lock.h>

#include <machine/bus.h>
#include <arm/sa11x0/sa1111_reg.h>
#include <arm/sa11x0/sa1111_var.h>

#include <dev/ic/pckbcvar.h>		/* for prototypes */

#include "pckbd.h"
#include "rnd.h"
#include "locators.h"

/* descriptor for one device command */
struct pckbc_devcmd {
	TAILQ_ENTRY(pckbc_devcmd) next;
	int flags;
#define KBC_CMDFLAG_SYNC 1 /* give descriptor back to caller */
#define KBC_CMDFLAG_SLOW 2
	u_char cmd[4];
	int cmdlen, cmdidx, retries;
	u_char response[4];
	int status, responselen, responseidx;
};

struct sackbc_softc {
	struct device dev;

	bus_space_tag_t    iot;
	bus_space_handle_t ioh;

	void	*ih_rx;			/* receive interrupt */
	int	intr;			/* interrupt number */

	int	polling;	/* don't process data in interrupt handler */
	int	poll_stat;	/* data read from inr handler if polling */
	int	poll_data;	/* status read from intr handler if polling */

	TAILQ_HEAD(, pckbc_devcmd) cmdqueue; /* active commands */
	TAILQ_HEAD(, pckbc_devcmd) freequeue; /* free commands */
#define NCMD  5
	struct pckbc_devcmd cmd[NCMD];

	struct callout t_cleanup;
	pckbc_inputfcn inputhandler;
	void *inputarg;
	const char *subname;

};

#define CMD_IN_QUEUE(q) (TAILQ_FIRST(&(q)->cmdqueue) != NULL)

#define N_KBC_SLOTS  2
/*static struct sackbc_softc *sackbc_slot[N_KBC_SLOTS] = { NULL, NULL };*/

static	int	sackbc_match(struct device *, struct cfdata *, void *);
static	void	sackbc_attach(struct device *, struct device *, void *);
static int	 sackbc_cmdresponse( struct sackbc_softc *, int );

CFATTACH_DECL(sackbc, sizeof(struct sackbc_softc), sackbc_match,
    sackbc_attach, NULL, NULL);

/* XXX should not be here */
#define KBC_DEVCMD_ACK 0xfa
#define KBC_DEVCMD_RESEND 0xfe

#define	KBD_DELAY	DELAY(8)

/*#define SACKBCDEBUG*/

#ifdef SACKBCDEBUG
#define DPRINTF(arg)  printf arg
#else
#define DPRINTF(arg)
#endif

static void sackbc_poll_cmd1( struct sackbc_softc *, struct pckbc_devcmd * );



static int
sackbc_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct sa1111_attach_args *aa = (struct sa1111_attach_args *)aux;

	switch( aa->sa_addr ){
	case SACC_KBD0: case SACC_KBD1:
		return 1;
	}
	return 0;
}

#if 0
static int
sackbc_txint( void *cookie )
{
	struct sackbc_softc *sc = cookie;

	bus_space_read_4( sc->iot, sc->ioh, SACCKBD_STAT );

	return 0;
}
#endif

static int
sackbc_rxint( void *cookie )
{
	struct sackbc_softc *sc = cookie;
	int stat, code=-1;

	stat = bus_space_read_4( sc->iot, sc->ioh, SACCKBD_STAT );
	DPRINTF(( "sackbc_rxint stat=%x\n", stat ));
	if( stat & KBDSTAT_RXF ){
		code = bus_space_read_4( sc->iot, sc->ioh, SACCKBD_DATA );

		if( sc->polling ){
			sc->poll_data = code;
			sc->poll_stat = stat;
		}
		else if (CMD_IN_QUEUE(sc) && sackbc_cmdresponse(sc, code))
			;
		else if( sc->inputhandler ){
			(* sc->inputhandler)( sc->inputarg, code );
		}
		return 1;
	}

	return 0;
}

static int
sackbcprint(void *aux, const char *pnp)
{
	return (QUIET);
}

static void
sackbc_setup_intrhandler(struct sackbc_softc *sc)
{
	if( !(sc->polling) && sc->ih_rx==NULL ){
		sc->ih_rx = sacc_intr_establish( 
			(sacc_chipset_tag_t *)(sc->dev.dv_parent), 
			sc->intr+1, IST_EDGE_RAISE, IPL_TTY, sackbc_rxint, sc );
		if( sc->ih_rx == NULL ){
			printf( "%s: can't establish interrupt\n",
			    sc->dev.dv_xname );
		}
	}
}

static void
sackbc_disable_intrhandler( struct sackbc_softc *sc )
{
	if( sc->polling && sc->ih_rx ){
		sacc_intr_disestablish( 
			(sacc_chipset_tag_t *)(sc->dev.dv_parent),
			sc->ih_rx );
		sc->ih_rx = NULL;
	}
}

static int
sackbc_submatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct pckbc_attach_args *pa = aux;

	DPRINTF(( "slot = %d ", cf->cf_loc[SACKBCCF_SLOT] ));

	if( pa->pa_slot == PCKBCCF_SLOT_DEFAULT )
		pa->pa_slot = cf->cf_loc[SACKBCCF_SLOT];

	return config_match(parent, cf, aux);
}

static	void	
sackbc_attach(struct device *parent, struct device *self, void *aux)
{
	struct sackbc_softc *sc = (struct sackbc_softc *)self;
	struct sacc_softc *psc = (struct sacc_softc *)parent;
	struct sa1111_attach_args *aa = (struct sa1111_attach_args *)aux;
	uint32_t tmp, clock_bit;
	int i, found, intr;

	switch( aa->sa_addr ){
	case SACC_KBD0: clock_bit = (1<<6); intr = 21; break;
	case SACC_KBD1: clock_bit = (1<<5); intr = 18; break;
	default:
		return;
	}

	if( aa->sa_size <= 0 )
		aa->sa_size = SACCKBD_SIZE;
	if( aa->sa_intr == SACCCF_INTR_DEFAULT )
		aa->sa_intr = intr;

	sc->iot = psc->sc_iot;
	if( bus_space_subregion( psc->sc_iot, psc->sc_ioh, 
	    aa->sa_addr, aa->sa_size, &sc->ioh ) ){
		printf( ": can't map subregion\n" );
		return;
	}

	/* enable clock for PS/2 kbd or mouse */
	tmp = bus_space_read_4( psc->sc_iot, psc->sc_ioh, SACCSC_SKPCR );
	bus_space_write_4( psc->sc_iot, psc->sc_ioh, SACCSC_SKPCR,
	    tmp | clock_bit );

	sc->ih_rx = NULL;
	sc->intr = aa->sa_intr;
	sc->inputhandler = NULL;
	sc->subname = sc->dev.dv_xname;

	TAILQ_INIT(&sc->cmdqueue);
	TAILQ_INIT(&sc->freequeue);

	for (i = 0; i < NCMD; i++) {
		TAILQ_INSERT_TAIL(&sc->freequeue, &(sc->cmd[i]), next);
	}
	sc->polling = 0;

	tmp = bus_space_read_4( sc->iot, sc->ioh, SACCKBD_CR );
	bus_space_write_4( sc->iot, sc->ioh, SACCKBD_CR, tmp | KBDCR_ENA );

	/* XXX: this is necessary to get keyboard working. but I don't know why */
	bus_space_write_4( sc->iot, sc->ioh, SACCKBD_CLKDIV, 2 );

	tmp = bus_space_read_4( sc->iot, sc->ioh, SACCKBD_STAT );
	if( (tmp & KBDSTAT_ENA) == 0 ){
		printf("??? can't enable KBD controller\n");
		return;
	}

	printf("\n");

	{
		struct pckbc_attach_args pa;

		pa.pa_tag = sc;
		pa.pa_slot = PCKBCCF_SLOT_DEFAULT; /* Bogus */

		found = (config_found_sm(self, &pa,
		    sackbcprint, sackbc_submatch) != NULL);

#if 0 && NRND > 0			/* XXX: not yet */
		if (found && (t->t_slotdata[slot] != NULL))
			rnd_attach_source(&t->t_slotdata[slot]->rnd_source,
			    sc->subname[slot], RND_TYPE_TTY, 0);
#endif
	}

}


static inline int
sackbc_wait_output( struct sackbc_softc *sc )
{
	u_int i, stat;

	for (i = 100000; i; i--){
		stat = bus_space_read_4(sc->iot, sc->ioh, SACCKBD_STAT);
		delay(100);
		if( stat & KBDSTAT_TXE) 
			return 1;
	}
	return 0;
}

static int
sackbc_poll_data1( struct sackbc_softc *sc )
{
	int i, s, stat, c = -1;

	s = spltty();

	if (sc->polling){
		stat	= sc->poll_stat;
		c	= sc->poll_data;
		sc->poll_data = -1;
		sc->poll_stat = -1;
		if( stat >= 0 &&
		    (stat & (KBDSTAT_RXF|KBDSTAT_STP)) == KBDSTAT_RXF ){
			splx(s);
			return c;
		}
	}

	/* if 1 port read takes 1us (?), this polls for 100ms */
	for (i = 100000; i; i--) {
		stat = bus_space_read_4(sc->iot, sc->ioh, SACCKBD_STAT);
		if( (stat & (KBDSTAT_RXF|KBDSTAT_STP)) == KBDSTAT_RXF ){
			KBD_DELAY;
			c = bus_space_read_4(sc->iot, sc->ioh, SACCKBD_DATA);
			break;	
		}
	}

	splx(s);
	return (c);
}

static int
sackbc_send_cmd( struct sackbc_softc *sc, int val )
{
	if ( !sackbc_wait_output(sc) )
		return (0);
	bus_space_write_1( sc->iot, sc->ioh, SACCKBD_DATA, val );
	return (1);
}

#define sackbc_send_devcmd	sackbc_send_cmd

/*
 * Clean up a command queue, throw away everything.
 */
static void
sackbc_cleanqueue( struct sackbc_softc *sc )
{
	struct pckbc_devcmd *cmd;
#ifdef SACKBCDEBUG
	int i;
#endif

	while ((cmd = TAILQ_FIRST(&sc->cmdqueue))) {
		TAILQ_REMOVE(&sc->cmdqueue, cmd, next);
#ifdef SACKBCDEBUG
		printf("sackbc_cleanqueue: removing");
		for (i = 0; i < cmd->cmdlen; i++)
			printf(" %02x", cmd->cmd[i]);
		printf("\n");
#endif
		TAILQ_INSERT_TAIL(&sc->freequeue, cmd, next);
	}
}

/*
 * Timeout error handler: clean queues and data port.
 * XXX could be less invasive.
 */
static void
sackbc_cleanup(void *self)
{
	struct sackbc_softc *sc = self;
	int s;

	printf("sackbc: command timeout\n");

	s = spltty();

	sackbc_cleanqueue(sc);

	while (bus_space_read_4(sc->iot, sc->ioh, SACCKBD_STAT) & KBDSTAT_RXF) {
		KBD_DELAY;
		(void) bus_space_read_4(sc->iot, sc->ioh, SACCKBD_DATA);
	}

	/* reset KBC? */

	splx(s);
}


/*
 * Pass command to device during normal operation.
 * to be called at spltty()
 */
static void
sackbc_start( struct sackbc_softc *sc )
{
	struct pckbc_devcmd *cmd = TAILQ_FIRST(&sc->cmdqueue);

	if (sc->polling) {
		while(cmd){
			sackbc_poll_cmd1(sc, cmd);
			if (cmd->status)
				printf("sackbc_start: command error\n");

			TAILQ_REMOVE(&sc->cmdqueue, cmd, next);
			if (cmd->flags & KBC_CMDFLAG_SYNC)
				wakeup(cmd);
			else {
				callout_stop(&sc->t_cleanup);
				TAILQ_INSERT_TAIL(&sc->freequeue, cmd, next);
			}
			cmd = TAILQ_FIRST(&sc->cmdqueue);
		}
		return;
	}

	if (!sackbc_send_devcmd(sc, cmd->cmd[cmd->cmdidx])) {
		printf("sackbc_start: send error\n");
		/* XXX what now? */
		return;
	}
}

/*
 * Handle command responses coming in asynchonously,
 * return nonzero if valid response.
 * to be called at spltty()
 */
static int
sackbc_cmdresponse( struct sackbc_softc *sc, int data)
{
	struct pckbc_devcmd *cmd = TAILQ_FIRST(&sc->cmdqueue);
#ifdef DIAGNOSTIC
	if (!cmd)
		panic("sackbc_cmdresponse: no active command");
#endif
	if (cmd->cmdidx < cmd->cmdlen) {
		if (data != KBC_DEVCMD_ACK && data != KBC_DEVCMD_RESEND)
			return (0);

		if (data == KBC_DEVCMD_RESEND) {
			if (cmd->retries++ < 5) {
				/* try again last command */
				goto restart;
			} else {
				printf("pckbc: cmd failed\n");
				cmd->status = EIO;
				/* dequeue */
			}
		} else {
			if (++cmd->cmdidx < cmd->cmdlen)
				goto restart;
			if (cmd->responselen)
				return (1);
			/* else dequeue */
		}
	} else if (cmd->responseidx < cmd->responselen) {
		cmd->response[cmd->responseidx++] = data;
		if (cmd->responseidx < cmd->responselen)
			return (1);
		/* else dequeue */
	} else
		return (0);

	/* dequeue: */
	TAILQ_REMOVE(&sc->cmdqueue, cmd, next);
	if (cmd->flags & KBC_CMDFLAG_SYNC)
		wakeup(cmd);
	else {
		callout_stop(&sc->t_cleanup);
		TAILQ_INSERT_TAIL(&sc->freequeue, cmd, next);
	}
	if (!CMD_IN_QUEUE(sc))
		return (1);
restart:
	sackbc_start(sc);
	return (1);
}

/*
 * Pass command to device, poll for ACK and data.
 * to be called at spltty()
 */
static void
sackbc_poll_cmd1( struct sackbc_softc *sc, struct pckbc_devcmd *cmd )
{
	int i, c = 0;

	while (cmd->cmdidx < cmd->cmdlen) {
		DPRINTF((" tx: %x ", cmd->cmd[cmd->cmdidx]));
		if (!sackbc_send_devcmd(sc, cmd->cmd[cmd->cmdidx])) {
			printf("sackbc_cmd: send error\n");
			cmd->status = EIO;
			return;
		}
		delay(1000);
		for (i = 10; i; i--) { /* 1s ??? */
			c = sackbc_poll_data1(sc);
			if (c != -1){
				DPRINTF((" rx: %x", c ));
				break;
			}
		}

		if (c == KBC_DEVCMD_ACK) {
			cmd->cmdidx++;
			continue;
		}
		if (c == KBC_DEVCMD_RESEND) {
			DPRINTF(("sackbc_cmd: RESEND\n"));

			if (cmd->retries++ < 5)
				continue;
			else {
				DPRINTF(("sackbc: cmd failed\n"));

				cmd->status = EIO;
				return;
			}
		}
		if (c == -1) {
			DPRINTF(("pckbc_cmd: timeout\n"));

			cmd->status = EIO;
			return;
		}
		DPRINTF(("pckbc_cmd: lost 0x%x\n", c));

	}

	while (cmd->responseidx < cmd->responselen) {
		if (cmd->flags & KBC_CMDFLAG_SLOW)
			i = 100; /* 10s ??? */
		else
			i = 10; /* 1s ??? */
		while (i--) {
			c = sackbc_poll_data1(sc);
			if (c != -1){
				DPRINTF((" resp: %x", c));
				break;
			}
		}
		if (c == -1) {
			DPRINTF(("pckbc_cmd: no response"));

			cmd->status = ETIMEDOUT;
			return;
		} else
			cmd->response[cmd->responseidx++] = c;
	}
	DPRINTF(("\n"));
}


/*
 * Glue functions for pckbd on sackbc.
 * These functions emulate those in dev/ic/pckbc.c.
 * 
 */

void
pckbc_set_inputhandler( pckbc_tag_t self, pckbc_slot_t slot, 
    pckbc_inputfcn func, void *arg, char *name)
{
	struct sackbc_softc *sc = (struct sackbc_softc *) self;

	if( sc == NULL )
		return;

	DPRINTF(( "set_inputhandler %p %p\n", func, arg ));

	sc->inputhandler = func;
	sc->inputarg = arg;
	sc->subname = name;

	sackbc_setup_intrhandler(sc);
}


/* for use in autoconfiguration */
int
pckbc_poll_cmd(pckbc_tag_t self, pckbc_slot_t slot, 
    u_char *cmd, int len, int responselen, u_char *respbuf, int slow)
{
	struct pckbc_devcmd nc;
struct sackbc_softc *sc = (struct sackbc_softc *) self;

	if( sc == NULL )
		return EINVAL;

	if ((len > 4) || (responselen > 4))
		return EINVAL;

	memset(&nc, 0, sizeof(nc));
	memcpy(nc.cmd, cmd, len);
	nc.cmdlen = len;
	nc.responselen = responselen;
	nc.flags = (slow ? KBC_CMDFLAG_SLOW : 0);

	sackbc_poll_cmd1(sc, &nc);

	if (nc.status == 0 && respbuf)
		memcpy(respbuf, nc.response, responselen);

	return (nc.status);
}


/*
 * switch scancode translation on / off
 * return nonzero on success
 */
int
pckbc_xt_translation(pckbc_tag_t self, pckbc_slot_t slot, int on)
{
	/* KBD/Mouse controller doesn't have scancode translation */
	return !on;
}

void
pckbc_slot_enable(pckbc_tag_t self, pckbc_slot_t slot, int on)
{
#if 0
	struct sackbc_softc *sc = (struct sackbc_softc *) self;
	int cmd;

	cmd = on ? KBC_KBDENABLE : KBC_KBDDISABLE;
	if ( !sackbc_send_cmd(sc, cmd ) )
		printf("sackbc_slot_enable(%d) failed\n", on);
#endif
}


void
pckbc_flush(pckbc_tag_t self, pckbc_slot_t slot)
{
	struct sackbc_softc *sc  = (struct sackbc_softc *)self;

	(void) sackbc_poll_data1(sc);
}

#if 0
int
sackbc_poll_data( struct sackbc_softc *sc )
{
	struct pckbc_internal *t = self;
	struct pckbc_slotdata *q = t->t_slotdata[slot];
	int c;

	c = pckbc_poll_data1(t, slot, t->t_haveaux);
	if (c != -1 && q && CMD_IN_QUEUE(q)) {
		/* we jumped into a running command - try to
		 deliver the response */
		if (pckbc_cmdresponse(t, slot, c))
			return (-1);
	}
	return (c);
}
#endif

void
pckbc_set_poll(pckbc_tag_t self, pckbc_slot_t slot, int on)
{
	struct sackbc_softc *sc = (struct sackbc_softc *)self;
	int s;

	s = spltty();

	if( sc->polling != on ){

		sc->polling = on;

		if( on ){
			sc->poll_data = sc->poll_stat = -1;
			sackbc_disable_intrhandler(sc);
		}
		else {
			/*
			 * If disabling polling on a device that's
			 * been configured, make sure there are no
			 * bytes left in the FIFO, holding up the
			 * interrupt line.  Otherwise we won't get any
			 * further interrupts.
			 */
			sackbc_rxint(sc);
			sackbc_setup_intrhandler(sc);
		}
	}
	splx(s);
}

/*
 * Put command into the device's command queue, return zero or errno.
 */
int
pckbc_enqueue_cmd( pckbc_tag_t self, pckbc_slot_t slot, u_char *cmd,
    int len, int responselen, int sync, u_char *respbuf)
{
	struct sackbc_softc *sc = (struct sackbc_softc *)self;
	struct pckbc_devcmd *nc;
	int s, isactive, res = 0;

	if ( sc == NULL || (len > 4) || (responselen > 4) )
		return (EINVAL);

	s = spltty();
	nc = TAILQ_FIRST(&sc->freequeue);
	if (nc) {
		TAILQ_REMOVE(&sc->freequeue, nc, next);
	}
	splx(s);
	if (!nc)
		return (ENOMEM);

	memset(nc, 0, sizeof(*nc));
	memcpy(nc->cmd, cmd, len);
	nc->cmdlen = len;
	nc->responselen = responselen;
	nc->flags = (sync ? KBC_CMDFLAG_SYNC : 0);

	s = spltty();

	if (sc->polling && sync) {
		/*
		 * XXX We should poll until the queue is empty.
		 * But we don't come here normally, so make
		 * it simple and throw away everything.
		 */
		sackbc_cleanqueue(sc);
	}

	isactive = CMD_IN_QUEUE(sc);
	TAILQ_INSERT_TAIL(&sc->cmdqueue, nc, next);
	if (!isactive)
		sackbc_start(sc);

	if (sc->polling)
		res = (sync ? nc->status : 0);
	else if (sync) {
		if ((res = tsleep(nc, 0, "kbccmd", 1*hz))) {
			TAILQ_REMOVE(&sc->cmdqueue, nc, next);
			sackbc_cleanup(sc);
		} else
			res = nc->status;
	} else
		callout_reset(&sc->t_cleanup, hz, sackbc_cleanup, sc);

	if (sync) {
		if (respbuf)
			memcpy(respbuf, nc->response, responselen);
		TAILQ_INSERT_TAIL(&sc->freequeue, nc, next);
	}

	splx(s);

	return (res);
}

int
pckbc_poll_data(pckbc_tag_t self, pckbc_slot_t slot)
{
	struct sackbc_softc *sc = (struct sackbc_softc *)self;
	int c;

	c = sackbc_poll_data1(sc);
	if (c != -1 && CMD_IN_QUEUE(sc)) {
		/* we jumped into a running command - try to
		 deliver the response */
		if (sackbc_cmdresponse(sc, c))
			return -1;
	}
	return (c);
}
