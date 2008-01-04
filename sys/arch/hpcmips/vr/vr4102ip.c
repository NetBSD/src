/*	$NetBSD: vr4102ip.c,v 1.8 2008/01/04 22:13:57 ad Exp $	*/

/*-
 * Copyright (c) 2002 TAKEMURA Shin
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
__KERNEL_RCSID(0, "$NetBSD: vr4102ip.c,v 1.8 2008/01/04 22:13:57 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/bus.h>

#include "opt_vr41xx.h"
#include <hpcmips/vr/vrcpudef.h>
#include <hpcmips/vr/vripunit.h>
#include <hpcmips/vr/vripreg.h>
#include <hpcmips/vr/vripvar.h>
#include <hpcmips/vr/icureg.h>
#include <hpcmips/vr/cmureg.h>

void	vr4102ipattach(struct device *, struct device *, void *);

CFATTACH_DECL(vr4102ip, sizeof(struct vrip_softc),
    vripmatch, vr4102ipattach, NULL, NULL);

static const struct vrip_unit vr4102ip_units[] = {
	[VRIP_UNIT_PMU] = { "pmu",
			    { VRIP_INTR_POWER,	VRIP_INTR_BAT,	},	},
	[VRIP_UNIT_RTC] = { "rtc",
			    { VRIP_INTR_RTCL1,	},		},
	[VRIP_UNIT_PIU] = { "piu",
			    { VRIP_INTR_PIU, },
			    VR4102_CMUMSKPIU,
			    ICUPIUINT_REG_W,	MPIUINT_REG_W	},
	[VRIP_UNIT_KIU] = { "kiu",
			    { VRIP_INTR_KIU,	},
			    VR4102_CMUMSKKIU,
			    VR4102_KIUINT_REG_W,VR4102_MKIUINT_REG_W	},
	[VRIP_UNIT_SIU] = { "siu",
			    { VRIP_INTR_SIU,	},		},
	[VRIP_UNIT_GIU] = { "giu",
			    { VRIP_INTR_GIU,	},
			    0,
			    VR4102_GIUINT_L_REG_W,VR4102_MGIUINT_L_REG_W,
			    VR4102_GIUINT_H_REG_W,VR4102_MGIUINT_H_REG_W},
	[VRIP_UNIT_LED] = { "led",
			    { VRIP_INTR_LED,	},		},
	[VRIP_UNIT_AIU] = { "aiu",
			    { VRIP_INTR_AIU,	},
			    VR4102_CMUMSKAIU,
			    VR4102_AIUINT_REG_W,VR4102_MAIUINT_REG_W	},
	[VRIP_UNIT_FIR] = { "fir",
			    { VRIP_INTR_FIR,	},
			    VR4102_CMUMSKFIR,
			    VR4102_FIRINT_REG_W,VR4102_MFIRINT_REG_W	},
	[VRIP_UNIT_DSIU]= { "dsiu",
			    { VRIP_INTR_DSIU,	},
			    VR4102_CMUMSKDSIU,
			    VR4102_DSIUINT_REG_W,VR4102_MDSIUINT_REG_W	},
	[VRIP_UNIT_BCU] = { "bcu",
			    { VRIP_INTR_BCU,	},
			    0,
			    VR4102_BCUINT_REG_W,VR4102_MBCUINT_REG_W	}
};

void
vr4102ipattach(struct device *parent, struct device *self, void *aux)
{
	struct vrip_softc *sc = (struct vrip_softc*)self;

	printf("\n");

	sc->sc_units = vr4102ip_units;
	sc->sc_nunits = sizeof(vr4102ip_units)/sizeof(struct vrip_unit);
	sc->sc_icu_addr = VR4102_ICU_ADDR;
	sc->sc_sysint2 = VR4102_SYSINT2_REG_W;
	sc->sc_msysint2 = VR4102_MSYSINT2_REG_W;

	vripattach_common(parent, self, aux);
}
