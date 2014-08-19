/*      $NetBSD: amdtemp.c,v 1.16.2.1 2014/08/20 00:03:29 tls Exp $ */
/*      $OpenBSD: kate.c,v 1.2 2008/03/27 04:52:03 cnst Exp $   */

/*
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christoph Egger.
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
 * Copyright (c) 2008 Constantine A. Murenin <cnst+openbsd@bugmail.mojo.ru>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: amdtemp.c,v 1.16.2.1 2014/08/20 00:03:29 tls Exp $ ");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/module.h>

#include <machine/specialreg.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/sysmon/sysmonvar.h>

/*
 * AMD K8:
 * http://www.amd.com/us-en/assets/content_type/white_papers_and_tech_docs/32559.pdf
 * AMD K8 Errata: #141
 * http://support.amd.com/us/Processor_TechDocs/33610_PUB_Rev3%2042v3.pdf
 *
 * Family10h:
 * http://www.amd.com/us-en/assets/content_type/white_papers_and_tech_docs/31116.PDF
 * Family10h Errata: #319
 * http://support.amd.com/de/Processor_TechDocs/41322.pdf
 *
 * Family11h:
 * http://www.amd.com/us-en/assets/content_type/white_papers_and_tech_docs/41256.pdf
 */

/* AMD Processors, Function 3 -- Miscellaneous Control
 */

/* Function 3 Registers */
#define THERMTRIP_STAT_R      0xe4
#define NORTHBRIDGE_CAP_R     0xe8
#define CPUID_FAMILY_MODEL_R  0xfc

/*
 * AMD NPT Family 0Fh Processors, Function 3 -- Miscellaneous Control
 */

/* Bits within Thermtrip Status Register */
#define K8_THERM_SENSE_SEL       (1 << 6)
#define K8_THERM_SENSE_CORE_SEL  (1 << 2)

/* Flip core and sensor selection bits */
#define K8_T_SEL_C0(v)           (v |= K8_THERM_SENSE_CORE_SEL)
#define K8_T_SEL_C1(v)           (v &= ~(K8_THERM_SENSE_CORE_SEL))
#define K8_T_SEL_S0(v)           (v &= ~(K8_THERM_SENSE_SEL))
#define K8_T_SEL_S1(v)           (v |= K8_THERM_SENSE_SEL)



/*
 * AMD Family 10h Processors, Function 3 -- Miscellaneous Control
 */

/* Function 3 Registers */
#define F10_TEMPERATURE_CTL_R	0xa4

/* Bits within Reported Temperature Control Register */
#define F10_TEMP_CURTEMP	(1 << 21)

/*
 * Revision Guide for AMD NPT Family 0Fh Processors,
 * Publication # 33610, Revision 3.30, February 2008
 */
#define K8_SOCKET_F	1	/* Server */
#define K8_SOCKET_AM2	2	/* Desktop */
#define K8_SOCKET_S1	3	/* Laptop */

static const struct {
	const char      rev[5];
	const struct {
		const pcireg_t  cpuid;
		const uint8_t   socket;
	} cpu[5];
} amdtemp_core[] = {
	{ "BH-F", { { 0x00040FB0, K8_SOCKET_AM2 },	/* F2 */
		  { 0x00040F80, K8_SOCKET_S1 },		/* F2 */
		  { 0, 0 }, { 0, 0 }, { 0, 0 } } },
	{ "DH-F", { { 0x00040FF0, K8_SOCKET_AM2 },	/* F2 */
		  { 0x00040FC0, K8_SOCKET_S1 },		/* F2 */
		  { 0x00050FF0, K8_SOCKET_AM2 },	/* F2, F3 */
		  { 0, 0 }, { 0, 0 } } },
	{ "JH-F", { { 0x00040F10, K8_SOCKET_F },	/* F2, F3 */
		  { 0x00040F30, K8_SOCKET_AM2 },	/* F2, F3 */
		  { 0x000C0F10, K8_SOCKET_F },		/* F3 */
		  { 0, 0 }, { 0, 0 } } },
	{ "BH-G", { { 0x00060FB0, K8_SOCKET_AM2 },	/* G1, G2 */
		  { 0x00060F80, K8_SOCKET_S1 },		/* G1, G2 */
		  { 0, 0 }, { 0, 0 }, { 0, 0 } } },
	{ "DH-G", { { 0x00060FF0, K8_SOCKET_AM2 },	/* G1, G2 */
		  { 0x00060FC0, K8_SOCKET_S1 },		/* G2 */
		  { 0x00070FF0, K8_SOCKET_AM2 },	/* G1, G2 */
		  { 0x00070FC0, K8_SOCKET_S1 },		/* G2 */
		  { 0, 0 } } }
};


struct amdtemp_softc {
        pci_chipset_tag_t sc_pc;
        pcitag_t sc_pcitag;

	struct sysmon_envsys *sc_sme;
	envsys_data_t *sc_sensor;
	size_t sc_sensor_len;

        char sc_rev;
        int8_t sc_numsensors;
	uint32_t sc_family;
	int32_t sc_adjustment;
};


static int  amdtemp_match(device_t, cfdata_t, void *);
static void amdtemp_attach(device_t, device_t, void *);
static int  amdtemp_detach(device_t, int);

static void amdtemp_k8_init(struct amdtemp_softc *, pcireg_t);
static void amdtemp_k8_setup_sensors(struct amdtemp_softc *, int);
static void amdtemp_k8_refresh(struct sysmon_envsys *, envsys_data_t *);

static void amdtemp_family10_init(struct amdtemp_softc *);
static void amdtemp_family10_setup_sensors(struct amdtemp_softc *, int);
static void amdtemp_family10_refresh(struct sysmon_envsys *, envsys_data_t *);

CFATTACH_DECL_NEW(amdtemp, sizeof(struct amdtemp_softc),
	amdtemp_match, amdtemp_attach, amdtemp_detach, NULL);

static int
amdtemp_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;
	pcireg_t cpu_signature;
	uint32_t family;

	KASSERT(PCI_VENDOR(pa->pa_id) == PCI_VENDOR_AMD);

	cpu_signature = pci_conf_read(pa->pa_pc,
	    pa->pa_tag, CPUID_FAMILY_MODEL_R);

	/* This CPUID northbridge register has been introduced
	 * in Revision F */
	if (cpu_signature == 0x0)
		return 0;

	family = CPUID_TO_FAMILY(cpu_signature);

	/* Errata #319: This has been fixed in Revision C2. */
	if (family == 0x10) {
		if (CPUID_TO_BASEMODEL(cpu_signature) < 4)
			return 0;
		if (CPUID_TO_BASEMODEL(cpu_signature) == 4
		    && CPUID_TO_STEPPING(cpu_signature) < 2)
			return 0;
	}


	/* Not yet supported CPUs */
	if (family > 0x15)
		return 0;

	return 1;
}

static void
amdtemp_attach(device_t parent, device_t self, void *aux)
{
	struct amdtemp_softc *sc = device_private(self);
	struct pci_attach_args *pa = aux;
	pcireg_t cpu_signature;
	int error;
	uint8_t i;

	aprint_naive("\n");
	aprint_normal(": AMD CPU Temperature Sensors");

	cpu_signature = pci_conf_read(pa->pa_pc,
	    pa->pa_tag, CPUID_FAMILY_MODEL_R);

	/* If we hit this, then match routine is wrong. */
	KASSERT(cpu_signature != 0x0);

	sc->sc_family = CPUID_TO_FAMILY(cpu_signature);

	KASSERT(sc->sc_family >= 0xf);

	sc->sc_sme = NULL;
	sc->sc_sensor = NULL;

	sc->sc_pc = pa->pa_pc;
	sc->sc_pcitag = pa->pa_tag;
	sc->sc_adjustment = 0;

	switch (sc->sc_family) {
	case 0xf:  /* AMD K8 NPT */
		amdtemp_k8_init(sc, cpu_signature);
		break;

	case 0x10: /* AMD Barcelona/Phenom */
	case 0x11: /* AMD Griffin */
	case 0x12: /* AMD Lynx/Sabine (Llano) */
	case 0x14: /* AMD Brazos (Ontario/Zacate/Desna) */
	case 0x15:
		amdtemp_family10_init(sc);
		break;

	default:
		aprint_normal(", family 0x%x not supported\n",
			     sc->sc_family);
		return;
	}

	aprint_normal("\n");

	if (sc->sc_adjustment != 0)
		aprint_debug_dev(self, "Workaround enabled\n");

	sc->sc_sme = sysmon_envsys_create();
	sc->sc_sensor_len = sizeof(envsys_data_t) * sc->sc_numsensors;
	sc->sc_sensor = kmem_zalloc(sc->sc_sensor_len, KM_SLEEP);

	if (sc->sc_sensor == NULL)
		goto bad;

	switch (sc->sc_family) {
	case 0xf:
		amdtemp_k8_setup_sensors(sc, device_unit(self));
		break;
	case 0x10:
	case 0x11:
	case 0x12:
	case 0x14:
	case 0x15:
		amdtemp_family10_setup_sensors(sc, device_unit(self));
		break;
	}

	/*
	 * Set properties in sensors.
	 */
	for (i = 0; i < sc->sc_numsensors; i++) {
		if (sysmon_envsys_sensor_attach(sc->sc_sme,
						&sc->sc_sensor[i]))
			goto bad;
	}

	/*
	 * Register the sysmon_envsys device.
	 */
	sc->sc_sme->sme_name = device_xname(self);
	sc->sc_sme->sme_cookie = sc;

	switch (sc->sc_family) {
	case 0xf:
		sc->sc_sme->sme_refresh = amdtemp_k8_refresh;
		break;
	case 0x10:
	case 0x11:
	case 0x12:
	case 0x14:
	case 0x15:
		sc->sc_sme->sme_refresh = amdtemp_family10_refresh;
		break;
	}

	error = sysmon_envsys_register(sc->sc_sme);
	if (error) {
		aprint_error_dev(self, "unable to register with sysmon "
			"(error=%d)\n", error);
		goto bad;
	}

	(void)pmf_device_register(self, NULL, NULL);

	return;

bad:
	if (sc->sc_sme != NULL) {
		sysmon_envsys_destroy(sc->sc_sme);
		sc->sc_sme = NULL;
	}

	if (sc->sc_sensor != NULL) {
		kmem_free(sc->sc_sensor, sc->sc_sensor_len);
		sc->sc_sensor = NULL;
	}
}

static int
amdtemp_detach(device_t self, int flags)
{
	struct amdtemp_softc *sc = device_private(self);

	pmf_device_deregister(self);
	if (sc->sc_sme != NULL)
		sysmon_envsys_unregister(sc->sc_sme);

	if (sc->sc_sensor != NULL)
		kmem_free(sc->sc_sensor, sc->sc_sensor_len);

	return 0;
}

static void
amdtemp_k8_init(struct amdtemp_softc *sc, pcireg_t cpu_signature)
{
	pcireg_t data;
	uint32_t cmpcap;
	uint8_t i, j;

	aprint_normal(" (K8");

	for (i = 0; i < __arraycount(amdtemp_core) && sc->sc_rev == '\0'; i++) {
		for (j = 0; amdtemp_core[i].cpu[j].cpuid != 0; j++) {
			if ((cpu_signature & ~0xf)
			    != amdtemp_core[i].cpu[j].cpuid)
				continue;

			sc->sc_rev = amdtemp_core[i].rev[3];
			aprint_normal(": core rev %.4s%.1x",
				amdtemp_core[i].rev,
				CPUID_TO_STEPPING(cpu_signature));

			switch (amdtemp_core[i].cpu[j].socket) {
			case K8_SOCKET_AM2:
				if (sc->sc_rev == 'G')
					sc->sc_adjustment = 21000000;
				aprint_normal(", socket AM2");
				break;
			case K8_SOCKET_S1:
				aprint_normal(", socket S1");
				break;
			case K8_SOCKET_F:
				aprint_normal(", socket F");
				break;
			}
		}
	}

	if (sc->sc_rev == '\0') {
		/* CPUID Family Model Register was introduced in
		 * Revision F */
		sc->sc_rev = 'G';       /* newer than E, assume G */
		aprint_normal(": cpuid 0x%x", cpu_signature);
	}

	aprint_normal(")");

	data = pci_conf_read(sc->sc_pc, sc->sc_pcitag, NORTHBRIDGE_CAP_R);
	cmpcap = (data >> 12) & 0x3;

	sc->sc_numsensors = cmpcap ? 4 : 2;
}


static void
amdtemp_k8_setup_sensors(struct amdtemp_softc *sc, int dv_unit)
{
	uint8_t i;

	/* There are two sensors per CPU core. So we use the
	 * device unit as socket counter to correctly enumerate
	 * the CPUs on multi-socket machines.
	 */
	dv_unit *= (sc->sc_numsensors / 2);
	for (i = 0; i < sc->sc_numsensors; i++) {
		sc->sc_sensor[i].units = ENVSYS_STEMP;
		sc->sc_sensor[i].state = ENVSYS_SVALID;
		sc->sc_sensor[i].flags = ENVSYS_FHAS_ENTROPY;

		snprintf(sc->sc_sensor[i].desc, sizeof(sc->sc_sensor[i].desc),
			"CPU%u Sensor%u", dv_unit + (i / 2), i % 2);
	}
}


static void
amdtemp_k8_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct amdtemp_softc *sc = sme->sme_cookie;
	pcireg_t status, match, tmp;
	uint32_t value;

	status = pci_conf_read(sc->sc_pc, sc->sc_pcitag, THERMTRIP_STAT_R);

	switch(edata->sensor) { /* sensor number */
	case 0: /* Core 0 Sensor 0 */
		K8_T_SEL_C0(status);
		K8_T_SEL_S0(status);
		break;
	case 1: /* Core 0 Sensor 1 */
		K8_T_SEL_C0(status);
		K8_T_SEL_S1(status);
		break;
	case 2: /* Core 1 Sensor 0 */
		K8_T_SEL_C1(status);
		K8_T_SEL_S0(status);
		break;
	case 3: /* Core 1 Sensor 1 */
		K8_T_SEL_C1(status);
		K8_T_SEL_S1(status);
		break;
	}

	match = status & (K8_THERM_SENSE_CORE_SEL | K8_THERM_SENSE_SEL);
	pci_conf_write(sc->sc_pc, sc->sc_pcitag, THERMTRIP_STAT_R, status);
	status = pci_conf_read(sc->sc_pc, sc->sc_pcitag, THERMTRIP_STAT_R);
	tmp = status & (K8_THERM_SENSE_CORE_SEL | K8_THERM_SENSE_SEL);

	value = 0x3ff & (status >> 14);
	if (sc->sc_rev != 'G')
		value &= ~0x3;

	edata->state = ENVSYS_SINVALID;
	if ((tmp == match) && ((value & ~0x3) != 0)) {
		edata->state = ENVSYS_SVALID;
		edata->value_cur = (value * 250000 - 49000000) + 273150000
			+ sc->sc_adjustment;
	}
}


static void
amdtemp_family10_init(struct amdtemp_softc *sc)
{
	aprint_normal(" (Family%02xh)", sc->sc_family);

	sc->sc_numsensors = 1;
}

static void
amdtemp_family10_setup_sensors(struct amdtemp_softc *sc, int dv_unit)
{
	/* sanity check for future enhancements */
	KASSERT(sc->sc_numsensors == 1);

	/* There's one sensor per memory controller (= socket)
	 * so we use the device unit as socket counter
	 * to correctly enumerate the CPUs
	 */
	sc->sc_sensor[0].units = ENVSYS_STEMP;
	sc->sc_sensor[0].state = ENVSYS_SVALID;
	sc->sc_sensor[0].flags = ENVSYS_FHAS_ENTROPY;

	snprintf(sc->sc_sensor[0].desc, sizeof(sc->sc_sensor[0].desc),
		"cpu%u temperature", dv_unit);
}


static void
amdtemp_family10_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct amdtemp_softc *sc = sme->sme_cookie;
	pcireg_t status;
	uint32_t value;

	status = pci_conf_read(sc->sc_pc,
	    sc->sc_pcitag, F10_TEMPERATURE_CTL_R);

	value = (status >> 21);

	edata->state = ENVSYS_SVALID;
	edata->value_cur = (value * 125000) + 273150000; /* From C to uK. */
}

MODULE(MODULE_CLASS_DRIVER, amdtemp, NULL);

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
amdtemp_modcmd(modcmd_t cmd, void *aux)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_amdtemp,
		    cfattach_ioconf_amdtemp, cfdata_ioconf_amdtemp);
#endif
		return error;
	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_amdtemp,
		    cfattach_ioconf_amdtemp, cfdata_ioconf_amdtemp);
#endif
		return error;
	default:
		return ENOTTY;
	}
}
