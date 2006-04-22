/*	$NetBSD: adm1030.c,v 1.3.6.1 2006/04/22 11:38:52 simonb Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: adm1030.c,v 1.3.6.1 2006/04/22 11:38:52 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/sysctl.h>

#include <uvm/uvm_extern.h>

#include <dev/i2c/i2cvar.h>

#include <machine/autoconf.h>
#include <dev/sysmon/sysmonvar.h>
#include "sysmon_envsys.h"
#include <dev/i2c/adm1030var.h>

static void adm1030c_attach(struct device *, struct device *, void *);
static int adm1030c_match(struct device *, struct cfdata *, void *);

struct adm1030c_sysmon {
		struct sysmon_envsys sme;
		struct adm1030c_softc *sc;
		struct envsys_tre_data adm1030c_info[];
	};

static uint8_t adm1030c_readreg(struct adm1030c_softc *, uint8_t);
static void adm1030c_writereg(struct adm1030c_softc *, uint8_t, uint8_t);
static int adm1030c_temp2muk(uint8_t);
static int adm1030c_reg2rpm(uint8_t);
static int adm1030c_gtredata(struct sysmon_envsys *, struct envsys_tre_data *);
static int adm1030c_streinfo(struct sysmon_envsys *, struct envsys_basic_info *);

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

struct envsys_range *adm1030c_ranges;
struct envsys_basic_info *adm1030c_info;

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
	struct adm1030c_sysmon *datap;
	int error;
	struct envsys_range *cur_r;
	struct envsys_basic_info *cur_i;
	struct envsys_tre_data *cur_t;
	int ret;
	struct sysctlnode *me = NULL, *node = NULL;
	
	datap = malloc(sizeof(struct sysmon_envsys) + 3 * 
	    sizeof(struct envsys_tre_data) + sizeof(void *),
	    M_DEVBUF, M_WAITOK | M_ZERO);
	    
	adm1030c_ranges = malloc (sizeof(struct envsys_range) * 3,
	    M_DEVBUF, M_WAITOK | M_ZERO);
	    
	adm1030c_info = malloc (sizeof(struct envsys_basic_info) * 3,
	    M_DEVBUF, M_WAITOK | M_ZERO);
	
	ret=sysctl_createv(NULL, 0, NULL, (const struct sysctlnode **)&me,
	       CTLFLAG_READWRITE,
	       CTLTYPE_NODE, sc->sc_dev.dv_xname, NULL,
	       NULL, 0, NULL, 0,
	       CTL_MACHDEP, CTL_CREATE, CTL_EOL);

	cur_r = &adm1030c_ranges[0];
	cur_i = &adm1030c_info[0];
	cur_t = &datap->adm1030c_info[0];
	strcpy(cur_i->desc, "case temperature");
	cur_i->units = ENVSYS_STEMP;
	cur_i->sensor = 0;
	sc->regs[0] = 0x0a;	/* remote temperature register */
	cur_r->low = adm1030c_temp2muk(-127);
	cur_r->high = adm1030c_temp2muk(127);
	cur_r->units = ENVSYS_STEMP;
	ret=sysctl_createv(NULL, 0, NULL, (const struct sysctlnode **)&node,
		CTLFLAG_READWRITE | CTLFLAG_OWNDESC | CTLFLAG_IMMEDIATE,
		CTLTYPE_INT, "temp0", cur_i->desc,
		sysctl_adm1030c_temp, 0x25, NULL, 0,
		CTL_MACHDEP, me->sysctl_num, CTL_CREATE, CTL_EOL);
		if (node != NULL) {
			node->sysctl_data = sc;
		}
	cur_i->validflags = ENVSYS_FVALID | ENVSYS_FCURVALID;
	cur_t->sensor = 0;
	cur_t->warnflags = ENVSYS_WARN_OK;
	cur_t->validflags = ENVSYS_FVALID | ENVSYS_FCURVALID;
	cur_t->units = cur_i->units;

	cur_r = &adm1030c_ranges[1];
	cur_i = &adm1030c_info[1];
	cur_t = &datap->adm1030c_info[1];
	strcpy(cur_i->desc, "CPU temperature");
	
	cur_i->units = ENVSYS_STEMP;
	cur_i->sensor = 1;
	sc->regs[1] = 0x0b;	/* built-in temperature register */
	cur_r->low = adm1030c_temp2muk(-127);
	cur_r->high = adm1030c_temp2muk(127);
	cur_r->units = ENVSYS_STEMP;
	ret=sysctl_createv(NULL, 0, NULL, (const struct sysctlnode **)&node,
		CTLFLAG_READWRITE | CTLFLAG_OWNDESC | CTLFLAG_IMMEDIATE,
		CTLTYPE_INT, "temp1", cur_i->desc,
		sysctl_adm1030c_temp,0x24, NULL, 0,
		CTL_MACHDEP, me->sysctl_num, CTL_CREATE, CTL_EOL);
		if(node!=NULL) {
			node->sysctl_data = sc;
		}
	cur_i->validflags = ENVSYS_FVALID|ENVSYS_FCURVALID;
	cur_t->sensor = 1;
	cur_t->warnflags = ENVSYS_WARN_OK;
	cur_t->validflags = ENVSYS_FVALID|ENVSYS_FCURVALID;
	cur_t->units = cur_i->units;

	cur_r = &adm1030c_ranges[2];
	cur_i = &adm1030c_info[2];
	cur_t = &datap->adm1030c_info[2];
	strcpy(cur_i->desc, "fan speed");
	cur_i->units = ENVSYS_SFANRPM;
	cur_i->sensor = 2;
	sc->regs[2] = 0x08;	/* fan rpm */
	cur_r->low = 0;
	cur_r->high = adm1030c_reg2rpm(0xfe);
	cur_r->units = ENVSYS_SFANRPM;

	cur_i->validflags = ENVSYS_FVALID | ENVSYS_FCURVALID;
	cur_t->sensor = 2;
	cur_t->warnflags = ENVSYS_WARN_OK;
	cur_t->validflags = ENVSYS_FVALID|ENVSYS_FCURVALID;
	cur_t->units = cur_i->units;

	sc->sc_sysmon_cookie = &datap->sme;
	datap->sme.sme_nsensors = 3;
	datap->sme.sme_envsys_version = 1000;
	datap->sme.sme_ranges = adm1030c_ranges;
	datap->sme.sme_sensor_info = adm1030c_info;
	datap->sme.sme_sensor_data = datap->adm1030c_info;
	
	datap->sme.sme_cookie = sc;
	datap->sme.sme_gtredata = adm1030c_gtredata;
	datap->sme.sme_streinfo = adm1030c_streinfo;
	datap->sme.sme_flags = 0;

	if ((error = sysmon_envsys_register(&datap->sme)) != 0)
		aprint_error("%s: unable to register with sysmon (%d)\n",
		    sc->sc_dev.dv_xname, error);
}


static int
adm1030c_gtredata(struct sysmon_envsys *sme, struct envsys_tre_data *tred)
{
	struct adm1030c_softc *sc = sme->sme_cookie;
	struct envsys_tre_data *cur_tre;
	struct envsys_basic_info *cur_i;
	int i;
	uint8_t reg;
	
	i = tred->sensor;
	cur_tre = &sme->sme_sensor_data[i];
	cur_i = &sme->sme_sensor_info[i];
	reg = sc->regs[i];
	switch (cur_tre->units)
	{
		case ENVSYS_STEMP:
			cur_tre->cur.data_s = 
			    adm1030c_temp2muk(adm1030c_readreg(sc, reg));
			break;
			
		case ENVSYS_SFANRPM:
			{
				uint8_t blah = adm1030c_readreg(sc,reg);
				cur_tre->cur.data_us = adm1030c_reg2rpm(blah);
			}
			break;
	}
	cur_tre->validflags |= ENVSYS_FCURVALID | ENVSYS_FVALID;
	*tred = sme->sme_sensor_data[i];
	return 0;
}

static int
adm1030c_streinfo(struct sysmon_envsys *sme, struct envsys_basic_info *binfo)
{

	/* There is nothing to set here. */
	return (EINVAL);
}
#endif /* NSYSMON_ENVSYS > 0 */
