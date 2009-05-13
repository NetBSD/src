/*	$NetBSD: mediabay.c,v 1.15.8.1 2009/05/13 17:18:01 jym Exp $	*/

/*-
 * Copyright (C) 1999 Tsubai Masanari.  All rights reserved.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mediabay.c,v 1.15.8.1 2009/05/13 17:18:01 jym Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <dev/ofw/openfirm.h>

#include <machine/autoconf.h>
#include <machine/pio.h>

enum mediabay_controller {
	MB_CONTROLLER_KEYLARGO,
	MB_CONTROLLER_OTHER
};

struct mediabay_softc {
	struct device sc_dev;
	bus_space_tag_t sc_tag;
	int sc_node;
	u_int *sc_addr;
	u_int *sc_fcr;
	u_int sc_baseaddr;
	struct device *sc_content;
	lwp_t *sc_kthread;
	enum mediabay_controller sc_type;
};

static const char *mediabay_keylargo[] = {
	"keylargo-media-bay",
	NULL
};

void mediabay_attach(struct device *, struct device *, void *);
int mediabay_match(struct device *, struct cfdata *, void *);
int mediabay_print(void *, const char *);
void mediabay_attach_content(struct mediabay_softc *);
int mediabay_intr(void *);
void mediabay_kthread(void *);

CFATTACH_DECL(mediabay, sizeof(struct mediabay_softc),
    mediabay_match, mediabay_attach, NULL, NULL);

#ifdef MEDIABAY_DEBUG
# define DPRINTF printf
#else
# define DPRINTF while (0) printf
#endif

#define FCR_MEDIABAY_RESET	0x00000002
#define FCR_MEDIABAY_IDE_ENABLE	0x00000008
#define FCR_MEDIABAY_FD_ENABLE	0x00000010
#define FCR_MEDIABAY_ENABLE	0x00000080
#define FCR_MEDIABAY_CD_POWER	0x00800000

#define MBCR_MEDIABAY0_ENABLE		0x00000100
#define MBCR_MEDIABAY0_RESET		0x00000200
#define MBCR_MEDIABAY0_POWER		0x00000400
#define MBCR_MEDIABAY0_IDE_ENABLE	0x00001000
#define MBCR_MEDIABAY0_DEVMASK		0x00007800

#define FCR1_EIDE0_ENABLE	0x00800000
#define FCR1_EIDE0_RESET	0x01000000

#define MEDIABAY_ID(sc, x)				\
	((sc)->sc_type == MB_CONTROLLER_KEYLARGO ?	\
	 (((x) >> 4) & 0xf) :				\
	 (((x) >> 12) & 0xf))
#define MEDIABAY_ID_FD		0
#define MEDIABAY_ID_CD		3
#define MEDIABAY_ID_NONE	7

int
mediabay_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct confargs *ca = aux;

	if (strcmp(ca->ca_name, "media-bay") == 0)
		return 1;

	return 0;
}

/*
 * Attach all the sub-devices we can find
 */
void
mediabay_attach(struct device *parent, struct device *self, void *aux)
{
	struct mediabay_softc *sc = (struct mediabay_softc *)self;
	struct confargs *ca = aux;
	int irq, itype;

	ca->ca_reg[0] += ca->ca_baseaddr;

	sc->sc_addr = mapiodev(ca->ca_reg[0], PAGE_SIZE);
	sc->sc_node = ca->ca_node;
	sc->sc_baseaddr = ca->ca_baseaddr;
	sc->sc_tag = ca->ca_tag;
	irq = ca->ca_intr[0];
	itype = IST_EDGE;

	if (of_compatible(ca->ca_node, mediabay_keylargo) != -1) {
		sc->sc_type = MB_CONTROLLER_KEYLARGO;
		sc->sc_fcr = sc->sc_addr + 2;
	} else {
		sc->sc_type = MB_CONTROLLER_OTHER;
		sc->sc_fcr = sc->sc_addr + 1;
	}

	if (ca->ca_nintr == 8 && ca->ca_intr[1] != 0)
		itype = IST_LEVEL;

	printf(" irq %d %s\n", irq, intr_typename(itype));

	intr_establish(irq, itype, IPL_BIO, mediabay_intr, sc);

	sc->sc_content = NULL;

	if (MEDIABAY_ID(sc, in32rb(sc->sc_addr)) != MEDIABAY_ID_NONE)
		mediabay_attach_content(sc);

	kthread_create(PRI_NONE, 0, NULL, mediabay_kthread, sc,
	    &sc->sc_kthread, "media-bay");
}

void
mediabay_attach_content(struct mediabay_softc *sc)
{
	int child;
	u_int fcr = 0;
	struct device *content;
	struct confargs ca;
	u_int reg[20], intr[5];
	char name[32];

	if (sc->sc_type != MB_CONTROLLER_KEYLARGO) {
		fcr = in32rb(sc->sc_fcr);

		/*
		 * if the mediabay isn't powered up we need to wait a few seconds
		 * before probing devices
		 */
		if ((fcr & (FCR_MEDIABAY_ENABLE | FCR_MEDIABAY_IDE_ENABLE
		    | FCR_MEDIABAY_CD_POWER)) != (FCR_MEDIABAY_ENABLE
		    | FCR_MEDIABAY_IDE_ENABLE | FCR_MEDIABAY_CD_POWER)) {
			fcr |= FCR_MEDIABAY_ENABLE | FCR_MEDIABAY_RESET;
			out32rb(sc->sc_fcr, fcr);
			delay(50000);

			fcr &= ~FCR_MEDIABAY_RESET;
			out32rb(sc->sc_fcr, fcr);
			delay(50000);

			fcr |= FCR_MEDIABAY_IDE_ENABLE | FCR_MEDIABAY_CD_POWER;
			out32rb(sc->sc_fcr, fcr);
			delay(50000);
			printf("%s: powering up...\n", sc->sc_dev.dv_xname);
			delay(2000000);
		}
	} else {
		printf("%s: powering up keylargo-media-bay..", sc->sc_dev.dv_xname);

		out32rb(sc->sc_addr, in32rb(sc->sc_addr) & ~MBCR_MEDIABAY0_RESET);
		out32rb(sc->sc_addr, in32rb(sc->sc_addr) & ~MBCR_MEDIABAY0_POWER);
		delay(50000);

		out32rb(sc->sc_addr, in32rb(sc->sc_addr) & ~MBCR_MEDIABAY0_ENABLE);
		delay(50000);

		out32rb(sc->sc_addr,
		    in32rb(sc->sc_addr) | MBCR_MEDIABAY0_IDE_ENABLE);
		delay(50000);

		out32rb(sc->sc_addr, in32rb(sc->sc_addr) | MBCR_MEDIABAY0_RESET);
		delay(50000);

		out32rb(sc->sc_fcr, in32rb(sc->sc_fcr) | FCR1_EIDE0_RESET);
		out32rb(sc->sc_fcr, in32rb(sc->sc_fcr) | FCR1_EIDE0_ENABLE);
		delay(50000);

		out32rb(sc->sc_addr, in32rb(sc->sc_addr) | MBCR_MEDIABAY0_ENABLE);
		__asm volatile ("eieio");
		delay(50000);

		out32rb(sc->sc_addr, in32rb(sc->sc_addr) & ~0xf);
		__asm volatile ("eieio");
		delay(50000);

		tsleep(sc, PRI_NONE, "mediabay", hz*1);

		printf(" done.\n");
	}

	for (child = OF_child(sc->sc_node); child; child = OF_peer(child)) {
		memset(name, 0, sizeof(name));
		if (OF_getprop(child, "name", name, sizeof(name)) == -1)
			continue;
		ca.ca_name = name;
		ca.ca_node = child;
		ca.ca_baseaddr = sc->sc_baseaddr;
		ca.ca_tag = sc->sc_tag;

		ca.ca_nreg  = OF_getprop(child, "reg", reg, sizeof(reg));
		ca.ca_nintr = OF_getprop(child, "AAPL,interrupts", intr,
				sizeof(intr));
		if (ca.ca_nintr == -1)
			ca.ca_nintr = OF_getprop(child, "interrupts", intr,
					sizeof(intr));
		ca.ca_reg = reg;
		ca.ca_intr = intr;

		content = config_found(&sc->sc_dev, &ca, mediabay_print);
		if (content) {
			sc->sc_content = content;
			return;
		}
	}

	if (sc->sc_type != MB_CONTROLLER_KEYLARGO) {
		/* No devices found.  Disable media-bay. */
		fcr &= ~(FCR_MEDIABAY_ENABLE | FCR_MEDIABAY_IDE_ENABLE |
			 FCR_MEDIABAY_CD_POWER | FCR_MEDIABAY_FD_ENABLE);
		out32rb(sc->sc_fcr, fcr);
	}
}

int
mediabay_print(void *aux, const char *mediabay)
{
	struct confargs *ca = aux;

	if (mediabay == NULL && ca->ca_nreg > 0)
		aprint_normal(" offset 0x%x", ca->ca_reg[0]);

	return QUIET;
}

int
mediabay_intr(void *v)
{
	struct mediabay_softc *sc = v;

	wakeup(&sc->sc_kthread);

	return 1;
}

void
mediabay_kthread(void *v)
{
	struct mediabay_softc *sc = v;
	u_int x, fcr;

	for (;;) {
		tsleep(&sc->sc_kthread, PRIBIO, "mbayev", 0);

		/* sleep 0.25 sec */
		tsleep(mediabay_kthread, PRIBIO, "mbayev", hz/4);

		DPRINTF("%s: ", sc->sc_dev.dv_xname);
		x = in32rb(sc->sc_addr);

		switch (MEDIABAY_ID(sc, x)) {
		case MEDIABAY_ID_NONE:
			DPRINTF("removed\n");
			if (sc->sc_content != NULL) {
				config_detach(sc->sc_content, DETACH_FORCE);
				DPRINTF("%s: detach done\n",
					sc->sc_dev.dv_xname);
				sc->sc_content = NULL;

				if (sc->sc_type != MB_CONTROLLER_KEYLARGO) {
					/* disable media-bay */
					fcr = in32rb(sc->sc_fcr);
					fcr &= ~(FCR_MEDIABAY_ENABLE |
						 FCR_MEDIABAY_IDE_ENABLE |
						 FCR_MEDIABAY_CD_POWER |
						 FCR_MEDIABAY_FD_ENABLE);
					out32rb(sc->sc_fcr, fcr);
				}
			}
			break;
		case MEDIABAY_ID_FD:
			DPRINTF("FD inserted\n");
			break;
		case MEDIABAY_ID_CD:
			DPRINTF("CD inserted\n");

			if (sc->sc_content == NULL)
				mediabay_attach_content(sc);
			break;
		default:
			printf("unknown event (0x%x)\n", x);
		}
	}
}

/* PBG3: 0x7025X0c0 */
/* 3400: 0x7070X080 */
/* 2400: 0x0070X0a8 */
