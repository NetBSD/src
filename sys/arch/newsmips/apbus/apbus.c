/*	$NetBSD: apbus.c,v 1.2 1999/12/23 06:52:30 tsubai Exp $	*/

/*-
 * Copyright (C) 1999 SHIMIZU Ryo.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/adrsmap.h>
#include <machine/autoconf.h>
#include <newsmips/apbus/apbusvar.h>

static int  apbusmatch __P((struct device *, struct cfdata *, void *));
static void apbusattach __P((struct device *, struct device *, void *));
static int apbusprint __P((void *, const char *));
/* static void *aptokseg0 __P((void *)); */

#define	MAXAPDEVNUM	32

struct apbus_softc {
	struct device apbs_dev;
};

struct cfattach ap_ca = {
	sizeof(struct apbus_softc), apbusmatch, apbusattach
};

#define	APBUS_DEVNAMELEN	16

struct ap_intrhand {
	int ai_mask;
	int ai_priority;
	void (*ai_func) __P((void*));	/* function */
	void *ai_aux;			/* softc */
	char ai_name[APBUS_DEVNAMELEN];
	int ai_ctlno;
};

#define	NLEVEL	2
#define	NBIT	16
#define	LEVELxBIT(l,b)	(((l)*NBIT)+(b))

static struct ap_intrhand apintr[NLEVEL*NBIT];

static int
apbusmatch(parent, cfdata, aux)
        struct device *parent;
        struct cfdata *cfdata;
        void *aux;
{
	struct confargs *ca = aux;

	if (strcmp(ca->ca_name, "ap") != 0)
		return 0;

	return 1;
}


static void
apbusattach(parent, self, aux)
        struct device *parent;
        struct device *self;
        void *aux;
{
	struct apbus_attach_args child;
	struct apbus_dev *apdev;
	struct apbus_ctl *apctl;

	*(volatile u_int *)(NEWS5000_APBUS_INTST) = 0xffffffff;
	*(volatile u_int *)(NEWS5000_APBUS_INTMSK) = 0xffffffff;
	*(volatile u_int *)(NEWS5000_APBUS_CTRL) = 0x00000004;
	*(volatile u_int *)(NEWS5000_APBUS_DMA) = 0xffffffff;

	printf("\n");

	/*
	 * get first ap-device
	 */
	apdev = apbus_lookupdev(NULL);

	/*
	 * trace device chain
	 */
	while (apdev) {
		apctl = apdev->apbd_ctl;

		/*
		 * probe physical device only
		 * (no pseudo device)
		 */
		if (apctl && apctl->apbc_hwbase) {
			/*
			 * ... and, all units
			 */
			while (apctl) {
				/* make apbus_attach_args for devices */
				child.apa_name = apdev->apbd_name;
				child.apa_ctlnum = apctl->apbc_ctlno;
				child.apa_slotno = apctl->apbc_sl;
				child.apa_hwbase = apctl->apbc_hwbase;

				config_found(self, &child, apbusprint);

				apctl = apctl->apbc_link;
			}
		}

		apdev = apdev->apbd_link;
	}
}

int
apbusprint(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct apbus_attach_args *a = aux;

	if (pnp)
		printf("%s at %s slot%d addr 0x%lx",
			a->apa_name, pnp, a->apa_slotno, a->apa_hwbase);

	return UNCONF;
}

#if 0
void *
aptokseg0(va)
	void *va;
{
	vaddr_t addr = (vaddr_t)va;

	if (addr >= 0xfff00000) {
		addr -= 0xfff00000;
		addr += physmem << PGSHIFT;
		addr += 0x80000000;
		va = (void *)addr;
	}
	return va;
}
#endif

void
apbus_wbflush()
{
	volatile int *wbflush = (int *)NEWS5000_WBFLUSH;

	(void)*wbflush;
}

/*
 * called by hardware interrupt routine
 */
int
apbus_intr_call(level, stat)
	int level;
	int stat;
{
	int i;
	int nintr = 0;
	struct ap_intrhand *aip = &apintr[LEVELxBIT(level,0)];

	for(i = 0; i < NBIT; i++) {
		if (aip->ai_mask & stat) {
			(*aip->ai_func)(aip->ai_aux);
			nintr++;
		}
		aip++;
	}
	return nintr;
}

/*
 * register device interrupt routine
 */
void *
apbus_intr_establish(level, mask, priority, func, aux, name, ctlno)
	int level;
	int mask;
	int priority;
	void (*func) __P((void *));
	void *aux;
	char *name;
	int ctlno;
{
	int i;
	int nbit = -1;
	struct ap_intrhand *aip;

	for (i = 0; i < NBIT; i++) {
		if (mask & (1 << i)) {
			nbit = i;
			break;
		}
	}

	if (nbit == -1)
		panic("apbus_intr_establish");

	aip = &apintr[LEVELxBIT(level,nbit)];
	aip->ai_mask = 1 << nbit;
	aip->ai_priority = priority;
	aip->ai_func = func;
	aip->ai_aux = aux;
	strncpy(aip->ai_name, name, APBUS_DEVNAMELEN-1);
	aip->ai_ctlno = ctlno;

	return (void *)aip;
}
