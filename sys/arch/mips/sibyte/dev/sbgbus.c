/* $NetBSD: sbgbus.c,v 1.10.96.1 2009/11/23 18:28:46 matt Exp $ */

/*
 * Copyright 2000, 2001
 * Broadcom Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and copied only
 * in accordance with the following terms and conditions.  Subject to these
 * conditions, you may download, copy, install, use, modify and distribute
 * modified or unmodified copies of this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce and
 *    retain this copyright notice and list of conditions as they appear in
 *    the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Broadcom Corporation.  The "Broadcom Corporation" name may not be
 *    used to endorse or promote products derived from this software
 *    without the prior written permission of Broadcom Corporation.
 *
 * 3) THIS SOFTWARE IS PROVIDED "AS-IS" AND ANY EXPRESS OR IMPLIED
 *    WARRANTIES, INCLUDING BUT NOT LIMITED TO, ANY IMPLIED WARRANTIES OF
 *    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
 *    NON-INFRINGEMENT ARE DISCLAIMED. IN NO EVENT SHALL BROADCOM BE LIABLE
 *    FOR ANY DAMAGES WHATSOEVER, AND IN PARTICULAR, BROADCOM SHALL NOT BE
 *    LIABLE FOR DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *    BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *    OR OTHERWISE), EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sbgbus.c,v 1.10.96.1 2009/11/23 18:28:46 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/locore.h>
#include <machine/sb1250/sb1250_regs.h>
#include <machine/sb1250/sb1250_genbus.h>
#include <sbmips/dev/sbobio/sbobiovar.h>
#include <sbmips/dev/sbgbus/sbgbusvar.h>

extern struct cfdriver sbgbus_cd;

static int	sbgbus_match(struct device *, struct cfdata *, void *);
static void	sbgbus_attach(struct device *, struct device *, void *);

CFATTACH_DECL(sbgbus, sizeof(struct device),
    sbgbus_match, sbgbus_attach, NULL, NULL);

static int	sbgbussearch(struct device *, struct cfdata *,
			     const int *, void *);
static int	sbgbusprint(void *, const char *);

static int
sbgbus_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct sbobio_attach_args *sap = aux;

	if (sap->sa_locs.sa_type != SBOBIO_DEVTYPE_GBUS)
		return (0);

	return 1;
}

static void
sbgbus_attach(struct device *parent, struct device *self, void *aux)
{

	/* Configure children using indirect configuration. */
	config_search_ia(sbgbussearch, self, "sbgbus", NULL);
}

static int
sbgbusprint(void *aux, const char *pnp)
{
	struct sbgbus_attach_args *sga = aux;

	if (sga->sga_chipsel != SBGBUS_CHIPSEL_NONE)
		aprint_normal(" chipsel %u", sga->sga_chipsel);
	if (sga->sga_offset != 0)
		aprint_normal(" offset 0x%x", sga->sga_offset);
	if (sga->sga_intr[0] != SBGBUS_INTR_NONE) {
		aprint_normal(" intr %u", sga->sga_intr[0]);
		if (sga->sga_intr[1] != SBGBUS_INTR_NONE) {
			aprint_normal(",%u", sga->sga_intr[1]);
		}
	}
	return (UNCONF);
}

static int
sbgbussearch(struct device *parent, struct cfdata *cf,
	     const int *ldesc, void *aux)
{
	struct sbgbus_attach_args sga;
	int tryagain;

	do {
		/* Fill in sga */
		sga.sga_chipsel = cf->cf_loc[SBGBUSCF_CHIPSEL];
		sga.sga_offset = cf->cf_loc[SBGBUSCF_OFFSET];
		sga.sga_intr[0] = cf->cf_loc[SBGBUSCF_INTR];
		sga.sga_intr[1] = cf->cf_loc[SBGBUSCF_INTR_1];

		/* Some quick sanity checks. */
		if (sga.sga_intr[0] == SBGBUS_INTR_NONE &&
		    sga.sga_intr[1] != SBGBUS_INTR_NONE) {
			/* XXX probably should print */
			return (0);
		}
		if (sga.sga_chipsel == SBGBUS_CHIPSEL_NONE &&
		    sga.sga_offset != 0) {
			/* XXX probably should print */
			return (0);
		}
		if (sga.sga_chipsel >= IO_EXT_CFG_COUNT) {
			/* XXX probably should print */
			return (0);
		}

		/* XXX check for PCMCIA. */

		if (sga.sga_chipsel == SBGBUS_CHIPSEL_NONE) {
			sga.sga_startphys = 0;
			sga.sga_size = 0;
		} else {
			uint64_t rv;

			rv = mips3_ld((volatile uint64_t *)MIPS_PHYS_TO_KSEG1(
			    A_IO_EXT_CS_BASE(sga.sga_chipsel) +
			    R_IO_EXT_START_ADDR));
			sga.sga_startphys = (rv & M_IO_START_ADDR) << S_IO_ADDRBASE;
			rv = mips3_ld((volatile uint64_t *)MIPS_PHYS_TO_KSEG1(
			    A_IO_EXT_CS_BASE(sga.sga_chipsel) +
			    R_IO_EXT_MULT_SIZE));
			sga.sga_size = (rv & M_IO_MULT_SIZE) << S_IO_REGSIZE;

			/* Make sure that region is enabled.  */
			if (sga.sga_size == 0 ||
			    sga.sga_startphys < A_PHYS_GENBUS ||
			    sga.sga_startphys >= A_PHYS_GENBUS_END)
				return (0);

			/* Report to child only the space they can access. */
			if (sga.sga_startphys + sga.sga_size > A_PHYS_GENBUS_END)
			    sga.sga_size = A_PHYS_GENBUS_END - sga.sga_startphys;
		}

		tryagain = 0;
		if (config_match(parent, cf, &sga) > 0) {
			config_attach(parent, cf, &sga, sbgbusprint);
			tryagain = (cf->cf_fstate == FSTATE_STAR);
		}
	} while (tryagain);

	return (0);
}
