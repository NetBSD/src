/*	$NetBSD: adm1030.c,v 1.9.8.1 2008/01/09 01:52:39 matt Exp $	*/

/*-
 * Copyright (C) 2005 Michael Lorenz.
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

/* 
 * a driver fot the ADM1030 environmental controller found in some iBook G3 
 * and probably other Apple machines 
 */


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: adm1030.c,v 1.9.8.1 2008/01/09 01:52:39 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/sysctl.h>

#include <uvm/uvm_extern.h>

#include <dev/i2c/i2cvar.h>

#include <dev/sysmon/sysmonvar.h>
#include "sysmon_envsys.h"
#include <dev/i2c/adm1030var.h>

static void adm1030c_attach(struct device *, struct device *, void *);
static int adm1030c_match(struct device *, struct cfdata *, void *);

static uint8_t adm1030c_readreg(struct adm1030c_softc *, uint8_t);
static void adm1030c_writereg(struct adm1030c_softc *, uint8_t, uint8_t);
static int adm1030c_temp2muk(uint8_t);
static int adm1030c_reg2rpm(uint8_t);
static void adm1030c_refresh(struct sysmon_envsys *, envsys_data_t *);

CFATTACH_DECL(adm1030c, sizeof(struct adm1030c_softc),
    adm1030c_match, adm1030c_attach, NULL, NULL);

static int
adm1030c_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	/* no probing when we're attaching to iic */
	return 1;
}

static void
adm1030c_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct adm1030c_softc *sc = device_private(self);
	struct i2c_attach_args *args = aux;

	sc->parent = parent;
	sc->address = args->ia_addr;
	printf(" ADM1030 thermal monitor and fan controller\n");
	sc->sc_i2c = (struct i2c_controller *)args->ia_tag;
	adm1030c_setup(sc);
}

static uint8_t
adm1030c_readreg(struct adm1030c_softc *sc, uint8_t reg)
{
	uint8_t data = 0;
	
	iic_acquire_bus(sc->sc_i2c,0);
	iic_exec(sc->sc_i2c, I2C_OP_READ, sc->address, &reg, 1,
	    &data, 1, 0);
	iic_release_bus(sc->sc_i2c, 0);
	return data;
}

static void 
adm1030c_writereg(struct adm1030c_softc *sc, uint8_t reg, uint8_t data)
{
	uint8_t mdata[2]={reg, data};
	
	iic_acquire_bus(sc->sc_i2c, 0);
	iic_exec(sc->sc_i2c, I2C_OP_WRITE, sc->address, &mdata, 2, NULL, 0, 0);
	iic_release_bus(sc->sc_i2c, 0);
}

#if NSYSMON_ENVSYS > 0
/* convert temperature read from the chip to micro kelvin */
static inline int 
adm1030c_temp2muk(uint8_t t)
{
	int temp=t;
	
	return temp * 1000000 + 273150000U;
}

static inline int
adm1030c_reg2rpm(uint8_t r)
{
	if (r == 0xff)
		return 0;
	return (11250 * 60) / (2 * (int)r);
}

SYSCTL_SETUP(sysctl_adm1030c_setup, "sysctl ADM1030M subtree setup")
{
#ifdef ADM1030_DEBUG
	printf("node setup\n");
#endif
	sysctl_createv(NULL, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "machdep", NULL,
		       NULL, 0, NULL, 0,
		       CTL_MACHDEP, CTL_EOL);
}

static int
sysctl_adm1030c_temp(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct adm1030c_softc *sc=(struct adm1030c_softc *)node.sysctl_data;
	int reg = 12345, nd=0;
	const int *np = newp;
	uint8_t chipreg = (uint8_t)(node.sysctl_idata & 0xff);
	
	reg = (uint32_t)adm1030c_readreg(sc, chipreg);
	reg = (reg & 0xf8) >> 1;
	node.sysctl_idata = reg;
	if (np) {
		/* we're asked to write */	
		nd = *np;
		node.sysctl_data = &reg;
		if (sysctl_lookup(SYSCTLFN_CALL(&node)) == 0) {
			int8_t new_reg;
			
			new_reg = (int8_t)(max(30, min(85, node.sysctl_idata)));
			new_reg = ((new_reg & 0x7c) << 1);	/* 5C range */
			adm1030c_writereg(sc, chipreg, new_reg);
			return 0;
		}
		return EINVAL;
	} else {
		node.sysctl_size = 4;
		return (sysctl_lookup(SYSCTLFN_CALL(&node)));
	}
}

void
adm1030c_setup(struct adm1030c_softc *sc)
{
	int error;
	int ret;
	struct sysctlnode *me = NULL, *node = NULL;

	sc->sc_sme = sysmon_envsys_create();	
	sc->sc_sensor = malloc(sizeof(envsys_data_t) * 3,
	    M_DEVBUF, M_WAITOK | M_ZERO);		
	
	ret=sysctl_createv(NULL, 0, NULL, (const struct sysctlnode **)&me,
	       CTLFLAG_READWRITE,
	       CTLTYPE_NODE, sc->sc_dev.dv_xname, NULL,
	       NULL, 0, NULL, 0,
	       CTL_MACHDEP, CTL_CREATE, CTL_EOL);

	(void)strlcpy(sc->sc_sensor[0].desc, "case temperature",
	    sizeof(sc->sc_sensor[0].desc));
	sc->sc_sensor[0].units = ENVSYS_STEMP;
	if (sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor[0]))
		goto out;

	sc->regs[0] = 0x0a;	/* remote temperature register */
	ret=sysctl_createv(NULL, 0, NULL, (const struct sysctlnode **)&node,
		CTLFLAG_READWRITE | CTLFLAG_OWNDESC | CTLFLAG_IMMEDIATE,
		CTLTYPE_INT, "temp0", sc->sc_sensor[0].desc,
		sysctl_adm1030c_temp, 0x25, NULL, 0,
		CTL_MACHDEP, me->sysctl_num, CTL_CREATE, CTL_EOL);
		if (node != NULL) {
			node->sysctl_data = sc;
		}

	(void)strlcpy(sc->sc_sensor[1].desc, "CPU temperature",
	    sizeof(sc->sc_sensor[1].desc));
	sc->sc_sensor[1].units = ENVSYS_STEMP;
	if (sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor[1]))
		goto out;
		
	sc->regs[1] = 0x0b;	/* built-in temperature register */
	ret=sysctl_createv(NULL, 0, NULL, (const struct sysctlnode **)&node,
		CTLFLAG_READWRITE | CTLFLAG_OWNDESC | CTLFLAG_IMMEDIATE,
		CTLTYPE_INT, "temp1", sc->sc_sensor[1].desc,
		sysctl_adm1030c_temp,0x24, NULL, 0,
		CTL_MACHDEP, me->sysctl_num, CTL_CREATE, CTL_EOL);
		if(node!=NULL) {
			node->sysctl_data = sc;
		}

	(void)strlcpy(sc->sc_sensor[2].desc, "fan speed",
	    sizeof(sc->sc_sensor[2].desc));
	sc->sc_sensor[2].units = ENVSYS_SFANRPM;
	sc->regs[2] = 0x08;	/* fan rpm */
	if (sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor[2]))
		goto out;

	sc->sc_sme->sme_name = sc->sc_dev.dv_xname;
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = adm1030c_refresh;

	if ((error = sysmon_envsys_register(sc->sc_sme)) != 0) {
		aprint_error("%s: unable to register with sysmon (%d)\n",
		    sc->sc_dev.dv_xname, error);
		goto out;
	}
	return;

out:
	sysmon_envsys_destroy(sc->sc_sme);
}


static void
adm1030c_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct adm1030c_softc *sc = sme->sme_cookie;
	int i;
	uint8_t reg;
	
	i = edata->sensor;
	reg = sc->regs[i];
	switch (edata->units)
	{
		case ENVSYS_STEMP:
			edata->value_cur = 
			    adm1030c_temp2muk(adm1030c_readreg(sc, reg));
			break;
			
		case ENVSYS_SFANRPM:
			{
				uint8_t blah = adm1030c_readreg(sc,reg);
				edata->value_cur = adm1030c_reg2rpm(blah);
			}
			break;
	}
	edata->state = ENVSYS_SVALID;
}
#endif /* NSYSMON_ENVSYS > 0 */
