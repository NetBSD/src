/* $NetBSD: vr4181ip.c,v 1.2 2003/07/15 02:29:35 lukem Exp $ */

/*-
 * Copyright (c) 1999, 2002
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
 * 3. Neither the name of the project nor the names of its contributors
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vr4181ip.c,v 1.2 2003/07/15 02:29:35 lukem Exp $");

#include "opt_vr41xx.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/bus.h>

#include <hpcmips/vr/vrcpudef.h>
#include <hpcmips/vr/vripunit.h>
#include <hpcmips/vr/vripreg.h>
#include <hpcmips/vr/vripvar.h>
#include <hpcmips/vr/vr4181ipvar.h>
#include <hpcmips/vr/icureg.h>
#include <hpcmips/vr/cmureg.h>

static void vr4181ipattach(struct device *, struct device *, void *);

CFATTACH_DECL(vr4181ip, sizeof(struct vrip_softc),
	      vripmatch, vr4181ipattach, NULL, NULL);

static const struct vrip_unit vr4181ip_units[] = {
	[VRIP_UNIT_RTC] = { "rtc",
			    { VRIP_INTR_RTCL1,	},		},
	[VRIP_UNIT_SIU] = { "siu",
			    { VRIP_INTR_SIU,	},		},
	[VRIP_UNIT_GIU] = { "giu",
			    { VRIP_INTR_GIU,	},		},
	[VRIP_UNIT_ECU] = { "ecu",
			    { VRIP_INTR_ECU,	},		},
	[VRIP_UNIT_AIU] = { "aiu",
			    { VRIP_INTR_DCU81,	},		},
#if 0
	[VRIP_UNIT_PMU] = { "pmu",
			    { VRIP_INTR_POWER,	VRIP_INTR_BAT,	},	},
	[VRIP_UNIT_PIU] = { "piu",
			    { VRIP_INTR_PIU, },
			    CMUMASK_PIU,
			    ICUPIUINT_REG_W,	MPIUINT_REG_W	},
	[VRIP_UNIT_KIU] = { "kiu",
			    { VRIP_INTR_KIU,	},
			    CMUMASK_KIU,
			    KIUINT_REG_W,	MKIUINT_REG_W	},
	[VRIP_UNIT_LED] = { "led",
			    { VRIP_INTR_LED,	},		},
	[VRIP_UNIT_AIU] = { "aiu",
			    { VRIP_INTR_AIU,	},
			    CMUMASK_AIU,
			    AIUINT_REG_W,	MAIUINT_REG_W	},
	[VRIP_UNIT_FIR] = { "fir",
			    { VRIP_INTR_FIR,	},
			    CMUMASK_FIR,
			    FIRINT_REG_W,	MFIRINT_REG_W	},
	[VRIP_UNIT_DSIU]= { "dsiu",
			    { VRIP_INTR_DSIU,	},
			    CMUMASK_DSIU,
			    DSIUINT_REG_W,	MDSIUINT_REG_W	},
	[VRIP_UNIT_PCIU]= { "pciu",
			    { VRIP_INTR_PCI,	},
			    CMUMASK_PCIU,
			    PCIINT_REG_W,	MPCIINT_REG_W	},
	[VRIP_UNIT_SCU] = { "scu",
			    { VRIP_INTR_SCU,	},
			    0,
			    SCUINT_REG_W,	MSCUINT_REG_W	},
	[VRIP_UNIT_CSI] = { "csi",
			    { VRIP_INTR_CSI,	},
			    CMUMASK_CSI,
			    CSIINT_REG_W,	MCSIINT_REG_W	},
	[VRIP_UNIT_BCU] = { "bcu",
			    { VRIP_INTR_BCU,	},
			    0,
			    BCUINT_REG_W,	MBCUINT_REG_W	},
#endif
};

#define MAXCOMINTR	2

struct vr4181ip_intrhand {
	int	(*ih_fun)(void *);
	void	*ih_arg;
};

static struct vr4181ip_intrhand intrhands[MAXCOMINTR];
static int			registered = 0;


static void
vr4181ipattach(struct device *parent, struct device *self, void *aux)
{
	struct vrip_softc *sc = (struct vrip_softc*) self;

	printf("\n");

	sc->sc_units = vr4181ip_units;
	sc->sc_nunits = sizeof(vr4181ip_units)/sizeof(struct vrip_unit);
	sc->sc_icu_addr = VR4181_ICU_ADDR;
	sc->sc_sysint2 = VR4181_SYSINT2_REG_W;
	sc->sc_msysint2 = VR4181_MSYSINT2_REG_W;

	vripattach_common(parent, self, aux);
}

static int
vr4181ip_comintr(void *arg)
{
	struct vr4181ip_intrhand	*ih = arg;
	int				i;

	for (i = 0; i < MAXCOMINTR; i++) {
		if (ih[i].ih_fun)
			ih[i].ih_fun(ih[i].ih_arg);
	}
	return 1;
}

void
vr4181ip_comintr_establish(vrip_chipset_tag_t vc, int unit, int line,
			   int level, int (*ih_fun)(void *), void *ih_arg)
{
	int	i;
	
	if (!registered) {
		if (!vrip_intr_establish(vc, unit, 0, IPL_TTY,
					 vr4181ip_comintr, intrhands)) {
			panic("vr4181ip_comintr_establish: "
			      "can't map interrupt line.");
		}
		registered = 1;
	}

	for (i = 0; i < MAXCOMINTR; i++) {
		if (intrhands[i].ih_fun == NULL) {
			intrhands[i].ih_fun = ih_fun;
			intrhands[i].ih_arg = ih_arg;
			break;
		}
	}
	if (i >= MAXCOMINTR)
		panic("too many vr4181 SIU");
}
