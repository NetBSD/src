/* $NetBSD: piixpcib.c,v 1.21.12.1 2017/12/03 11:36:18 jdolecek Exp $ */

/*-
 * Copyright (c) 2004, 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Minoura Makoto, Matthew R. Green, and Jared D. McNeill.
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
 * Intel PIIX4 PCI-ISA bridge device driver with CPU frequency scaling support
 *
 * Based on the FreeBSD 'smist' cpufreq driver by Bruno Ducrot
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: piixpcib.c,v 1.21.12.1 2017/12/03 11:36:18 jdolecek Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/sysctl.h>
#include <sys/bus.h>

#include <machine/frame.h>
#include <machine/bioscall.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <i386/pci/piixreg.h>
#include <x86/pci/pcibvar.h>

#define		PIIX4_PIRQRC	0x60

struct piixpcib_softc {
	/* we call pcibattach() which assumes our softc starts like this: */

	struct pcib_softc sc_pcib;

	device_t	sc_dev;

	int		sc_smi_cmd;
	int		sc_smi_data;
	int		sc_command;
	int		sc_flags;

	bus_space_tag_t	sc_iot;
	bus_space_handle_t sc_ioh;

	pcireg_t	sc_pirqrc;
	uint8_t		sc_elcr[2];
};

static int piixpcibmatch(device_t, cfdata_t, void *);
static void piixpcibattach(device_t, device_t, void *);

static bool piixpcib_suspend(device_t, const pmf_qual_t *);
static bool piixpcib_resume(device_t, const pmf_qual_t *);

static void speedstep_configure(struct piixpcib_softc *,
				const struct pci_attach_args *);
static int speedstep_sysctl_helper(SYSCTLFN_ARGS);

static struct piixpcib_softc *speedstep_cookie;	/* XXX */

CFATTACH_DECL_NEW(piixpcib, sizeof(struct piixpcib_softc),
    piixpcibmatch, piixpcibattach, NULL, NULL);

/*
 * Autoconf callbacks.
 */
static int
piixpcibmatch(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;

	/* We are ISA bridge, of course */
	if (PCI_CLASS(pa->pa_class) != PCI_CLASS_BRIDGE ||
	    (PCI_SUBCLASS(pa->pa_class) != PCI_SUBCLASS_BRIDGE_ISA &&
	    PCI_SUBCLASS(pa->pa_class) != PCI_SUBCLASS_BRIDGE_MISC)) {
		return 0;
	}

	/* Matches only Intel PIIX4 */
	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_INTEL) {
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_INTEL_82371AB_ISA:	/* PIIX4 */
		case PCI_PRODUCT_INTEL_82440MX_PMC:	/* PIIX4 in MX440 */
			return 10;
		}
	}

	return 0;
}

static void
piixpcibattach(device_t parent, device_t self, void *aux)
{
	struct pci_attach_args *pa = aux;
	struct piixpcib_softc *sc = device_private(self);

	sc->sc_dev = self;
	sc->sc_iot = pa->pa_iot;

	pcibattach(parent, self, aux);

	/* Set up SpeedStep. */
	speedstep_configure(sc, pa);

	/* Map edge/level control registers */
	if (bus_space_map(sc->sc_iot, PIIX_REG_ELCR, PIIX_REG_ELCR_SIZE, 0,
	    &sc->sc_ioh)) {
		aprint_error_dev(self,
		    "can't map edge/level control registers\n");
		return;
	}

	if (!pmf_device_register(self, piixpcib_suspend, piixpcib_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");
}

static bool
piixpcib_suspend(device_t dv, const pmf_qual_t *qual)
{
	struct piixpcib_softc *sc = device_private(dv);

	/* capture PIRQX route control registers */
	sc->sc_pirqrc = pci_conf_read(sc->sc_pcib.sc_pc, sc->sc_pcib.sc_tag,
	    PIIX4_PIRQRC);

	/* capture edge/level control registers */
	sc->sc_elcr[0] = bus_space_read_1(sc->sc_iot, sc->sc_ioh, 0);
	sc->sc_elcr[1] = bus_space_read_1(sc->sc_iot, sc->sc_ioh, 1);

	return true;
}

static bool
piixpcib_resume(device_t dv, const pmf_qual_t *qual)
{
	struct piixpcib_softc *sc = device_private(dv);

	/* restore PIRQX route control registers */
	pci_conf_write(sc->sc_pcib.sc_pc, sc->sc_pcib.sc_tag, PIIX4_PIRQRC,
	    sc->sc_pirqrc);

	/* restore edge/level control registers */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, 0, sc->sc_elcr[0]);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, 1, sc->sc_elcr[1]);

	return true;
}

/*
 * Intel PIIX4 (SMI) SpeedStep support.
 */

#define PIIXPCIB_GSIC		0x47534943
#define	PIIXPCIB_GETOWNER	0
#define	PIIXPCIB_GETSTATE	1
#define	PIIXPCIB_SETSTATE	2
#define	PIIXPCIB_GETFREQS	4

#define	PIIXPCIB_SPEEDSTEP_HIGH	0
#define	PIIXPCIB_SPEEDSTEP_LOW	1

static void
piixpcib_int15_gsic_call(int *sig, int *smicmd, int *cmd, int *smidata,
    int *flags)
{
	struct bioscallregs regs;

	memset(&regs, 0, sizeof(struct bioscallregs));
	regs.EAX = 0x0000e980;	/* IST support */
	regs.EDX = PIIXPCIB_GSIC;
	bioscall(0x15, &regs);

	if (regs.EAX == PIIXPCIB_GSIC) {
		*sig = regs.EAX;
		*smicmd = regs.EBX & 0xff;
		*cmd = (regs.EBX >> 16) & 0xff;
		*smidata = regs.ECX;
		*flags = regs.EDX;
	} else
		*sig = *smicmd = *cmd = *smidata = *flags = -1;

	return;
}

static int
piixpcib_set_ownership(struct piixpcib_softc *sc)
{
	int rv;
	u_long pmagic;
	static char magic[] = "Copyright (c) 1999 Intel Corporation";

	pmagic = vtophys((vaddr_t)magic);

	__asm__ __volatile__(
	    "movl $0, %%edi\n\t"
	    "out %%al, (%%dx)\n"
	    : "=D" (rv)
	    : "a" (sc->sc_command),
	      "b" (0),
	      "c" (0),
	      "d" (sc->sc_smi_cmd),
	      "S" (pmagic)
	);

	return (rv ? ENXIO : 0);
}

static int
piixpcib_getset_state(struct piixpcib_softc *sc, int *state, int function)
{
	int new;
	int rv;
	int eax;

#ifdef DIAGNOSTIC
	if (function != PIIXPCIB_GETSTATE &&
	    function != PIIXPCIB_SETSTATE) {
		aprint_error_dev(sc->sc_dev,
		    "GSI called with invalid function %d\n", function);
		return EINVAL;
	}
#endif

	__asm__ __volatile__(
	    "movl $0, %%edi\n\t"
	    "out %%al, (%%dx)\n"
	    : "=a" (eax),
	      "=b" (new),
	      "=D" (rv)
	    : "a" (sc->sc_command),
	      "b" (function),
	      "c" (*state),
	      "d" (sc->sc_smi_cmd),
	      "S" (0)
	);

	*state = new & 1;

	switch (function) {
	case PIIXPCIB_GETSTATE:
		if (eax)
			return ENXIO;
		break;
	case PIIXPCIB_SETSTATE:
		if (rv)
			return ENXIO;
		break;
	}

	return 0;
}

static int
piixpcib_get(struct piixpcib_softc *sc)
{
	int rv;
	int state;

	state = 0; 	/* XXX gcc */

	rv = piixpcib_getset_state(sc, &state, PIIXPCIB_GETSTATE);
	if (rv)
		return rv;

	return state & 1;
}

static int
piixpcib_set(struct piixpcib_softc *sc, int state)
{
	int rv, s;
	int try;

	if (state != PIIXPCIB_SPEEDSTEP_HIGH &&
	    state != PIIXPCIB_SPEEDSTEP_LOW)
		return ENXIO;
	if (piixpcib_get(sc) == state)
		return 0;

	try = 5;

	s = splhigh();

	do {
		rv = piixpcib_getset_state(sc, &state, PIIXPCIB_SETSTATE);
		if (rv)
			delay(200);
	} while (rv && --try);

	splx(s);

	return rv;
}

static void
speedstep_configure(struct piixpcib_softc *sc,
    const struct pci_attach_args *pa)
{
	const struct sysctlnode	*node, *ssnode;
	int sig, smicmd, cmd, smidata, flags;
	int rv;

	piixpcib_int15_gsic_call(&sig, &smicmd, &cmd, &smidata, &flags);

	if (sig != -1) {
		sc->sc_smi_cmd = smicmd;
		sc->sc_smi_data = smidata;
		if (cmd == 0x80) {
			aprint_debug_dev(sc->sc_dev,
			    "GSIC returned cmd 0x80, should be 0x82\n");
			cmd = 0x82;
		}
		sc->sc_command = (sig & 0xffffff00) | (cmd & 0xff);
		sc->sc_flags = flags;
	} else {
		/* setup some defaults */
		sc->sc_smi_cmd = 0xb2;
		sc->sc_smi_data = 0xb3;
		sc->sc_command = 0x47534982;
		sc->sc_flags = 0;
	}

	if (piixpcib_set_ownership(sc) != 0) {
		aprint_error_dev(sc->sc_dev,
		    "unable to claim ownership from the BIOS\n");
		return;	/* If we can't claim ownership from the BIOS, bail */
	}

	/* Put in machdep.speedstep_state (0 for low, 1 for high). */
	if ((rv = sysctl_createv(NULL, 0, NULL, &node,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "machdep", NULL,
	    NULL, 0, NULL, 0, CTL_MACHDEP, CTL_EOL)) != 0)
		goto err;

	/* CTLFLAG_ANYWRITE? kernel option like EST? */
	if ((rv = sysctl_createv(NULL, 0, &node, &ssnode,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "speedstep_state", NULL,
	    speedstep_sysctl_helper, 0, NULL, 0, CTL_CREATE,
	    CTL_EOL)) != 0)
		goto err;

	/* XXX save the sc for IO tag/handle */
	speedstep_cookie = sc;

	aprint_verbose_dev(sc->sc_dev, "SpeedStep SMI enabled\n");
	return;

err:
	aprint_normal("%s: sysctl_createv failed (rv = %d)\n", __func__, rv);
}

/*
 * get/set the SpeedStep state: 0 == low power, 1 == high power.
 */
static int
speedstep_sysctl_helper(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct piixpcib_softc *sc;
	uint8_t	state, state2;
	int ostate, nstate, error;

	sc = speedstep_cookie;
	error = 0;

	state = piixpcib_get(sc);
	if (state == PIIXPCIB_SPEEDSTEP_HIGH)
		ostate = 1;
	else
		ostate = 0;
	nstate = ostate;

	node = *rnode;
	node.sysctl_data = &nstate;

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		goto out;

	/* Only two states are available */
	if (nstate != 0 && nstate != 1) {
		error = EINVAL;
		goto out;
	}

	state2 = piixpcib_get(sc);
	if (state2 == PIIXPCIB_SPEEDSTEP_HIGH)
		ostate = 1;
	else
		ostate = 0;

	if (ostate != nstate)
	{
		if (nstate == 0)
			state2 = PIIXPCIB_SPEEDSTEP_LOW;
		else
			state2 = PIIXPCIB_SPEEDSTEP_HIGH;

		error = piixpcib_set(sc, state2);
	}
out:
	return (error);
}
