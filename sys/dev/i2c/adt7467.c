/*	$NetBSD: adt7467.c,v 1.8.8.1 2008/01/09 01:52:40 matt Exp $	*/

/*-
 * Copyright (C) 2005 Michael Lorenz
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
 * a driver fot the ADT7467 environmental controller found in the iBook G4 
 * and probably other Apple machines 
 */

/* 
 * todo: get the parental interface right, this is an ugly hack. The driver
 * should work with ANY i2c bus as parent 
 */
 
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: adt7467.c,v 1.8.8.1 2008/01/09 01:52:40 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/sysctl.h>

#include <uvm/uvm_extern.h>

#include <dev/i2c/adt7467var.h>


static void adt7467c_attach(struct device *, struct device *, void *);
static int adt7467c_match(struct device *, struct cfdata *, void *);

static uint8_t adt7467c_readreg(struct adt7467c_softc *, uint8_t);
static void adt7467c_writereg(struct adt7467c_softc *, uint8_t, uint8_t);
int sensor_type(char *);
int temp2muk(uint8_t);
int reg2rpm(uint16_t);
void adt7467c_refresh(struct sysmon_envsys *, envsys_data_t *);

CFATTACH_DECL(adt7467c, sizeof(struct adt7467c_softc),
    adt7467c_match, adt7467c_attach, NULL, NULL);

int
adt7467c_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	/* no probing if we attach to iic */
	return 1;
}

void
adt7467c_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct adt7467c_softc *sc = device_private(self);
	struct i2c_attach_args *args = aux;

	sc->parent = parent;
	sc->address = args->ia_addr;
	printf(" ADT7467 thermal monitor and fan controller, addr: %02x\n", 
	    sc->address);
	sc->sc_i2c = (struct i2c_controller *)args->ia_tag;
	adt7467c_setup(sc);
}

uint8_t adt7467c_readreg(struct adt7467c_softc *sc, uint8_t reg)
{
	uint8_t data = 0;
	
	iic_acquire_bus(sc->sc_i2c, 0);
	iic_exec(sc->sc_i2c, I2C_OP_READ, sc->address, &reg, 1,
	    &data, 1, 0);
	iic_release_bus(sc->sc_i2c, 0);
	return data;
}

void adt7467c_writereg(struct adt7467c_softc *sc, uint8_t reg, uint8_t data)
{
	uint8_t mdata[2] = {reg, data};
	
	iic_acquire_bus(sc->sc_i2c, 0);
	iic_exec(sc->sc_i2c, I2C_OP_WRITE, sc->address, &mdata, 2, NULL, 0, 0);
	iic_release_bus(sc->sc_i2c, 0);
}

#if NSYSMON_ENVSYS > 0

int sensor_type(char *t)
{
	if (strcmp(t, "temperature") == 0)
		return ENVSYS_STEMP;
	if (strcmp(t, "voltage") == 0)
		return ENVSYS_SVOLTS_DC;
	if (strcmp(t, "fanspeed") == 0)
		return ENVSYS_SFANRPM;
	return -1;
}

int temp2muk(uint8_t t)
{
	int temp = (int8_t)t;
	
	return temp * 1000000 + 273150000U;
}

int reg2rpm(uint16_t r)
{
	if(r == 0xffff)
		return 0;
	return (90000 * 60) / (int)r;
}

SYSCTL_SETUP(sysctl_adt7467_setup, "sysctl ADT7467 subtree setup")
{
#ifdef ADT7467_DEBUG
	printf("node setup\n");
#endif
	sysctl_createv(NULL, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "machdep", NULL,
		       NULL, 0, NULL, 0,
		       CTL_MACHDEP, CTL_EOL);
}

static int
sysctl_adt7467_temp(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct adt7467c_softc *sc = (struct adt7467c_softc *)node.sysctl_data;
	int reg = 12345, nd = 0;
	const int *np = newp;
	uint8_t chipreg = (uint8_t)(node.sysctl_idata & 0xff);
	
	reg = (uint32_t)adt7467c_readreg(sc, chipreg);
	node.sysctl_idata = reg;
	if (np) {
		/* we're asked to write */	
		nd = *np;
		node.sysctl_data = &reg;
		if (sysctl_lookup(SYSCTLFN_CALL(&node)) == 0) {
			int8_t new_reg;
			
			new_reg = (int8_t)(max(30, min(85, node.sysctl_idata)));
			adt7467c_writereg(sc,chipreg, new_reg);
			return 0;
		}
		return EINVAL;
	} else {
		node.sysctl_size = 4;
		return(sysctl_lookup(SYSCTLFN_CALL(&node)));
	}
}

void
adt7467c_setup(struct adt7467c_softc *sc)
{
	const struct sysctlnode *me=NULL;
	struct sysctlnode *node=NULL;
	int i, ret;
	int error;
	const char *sensor_desc[] = { "case temperature", "CPU temperature",
	    "GPU temperature" };
	char name[16];
	uint8_t stuff, sensortab[] = {0x26, 0x27, 0x25};
	
	sc->sc_sme = sysmon_envsys_create();
	    
	ret = sysctl_createv(NULL, 0, NULL, &me,
	       CTLFLAG_READWRITE,
	       CTLTYPE_NODE, sc->sc_dev.dv_xname, NULL,
	       NULL, 0, NULL, 0,
	       CTL_MACHDEP, CTL_CREATE, CTL_EOL);

	/* temperature sensors */
	for (i=0; i<3; i++) {
		snprintf(name, 16, "temp%d", i);
		strlcpy(sc->sc_sensor[i].desc, sensor_desc[i],
		    sizeof(sc->sc_sensor[i].desc));
		sc->sc_sensor[i].units = ENVSYS_STEMP;
		sc->regs[i] = sensortab[i];
		if (sysmon_envsys_sensor_attach(sc->sc_sme,
						&sc->sc_sensor[i]))
			goto out;

		ret = sysctl_createv(NULL, 0, NULL, 
		    (const struct sysctlnode **)&node, 
		    CTLFLAG_READWRITE | CTLFLAG_OWNDESC | CTLFLAG_IMMEDIATE,
		    CTLTYPE_INT, name, sc->sc_sensor[i].desc,
		    sysctl_adt7467_temp, 
		    sc->regs[i]+0x42 , NULL, 0, CTL_MACHDEP, me->sysctl_num, 
		    CTL_CREATE, CTL_EOL);
		if (node != NULL) {
			node->sysctl_data = sc;
		}
	}

	snprintf(name, 16, "voltage0");
	strlcpy(sc->sc_sensor[3].desc, name, sizeof(sc->sc_sensor[3].desc));
	sc->regs[3] = 0x21;
	sc->sc_sensor[3].units = ENVSYS_SVOLTS_DC;
	if (sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor[3]))
		goto out;

	snprintf(name, 16, "fan0");
	strlcpy(sc->sc_sensor[4].desc, name, sizeof(sc->sc_sensor[4].desc));
	sc->regs[4] = 0x28;
	sc->sc_sensor[4].units = ENVSYS_SFANRPM;
	if (sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor[4]))
		goto out;

	stuff = adt7467c_readreg(sc, 0x40);
	adt7467c_writereg(sc, 0x40, stuff);

	sc->sc_sme->sme_name = sc->sc_dev.dv_xname;
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = adt7467c_refresh;

	if ((error = sysmon_envsys_register(sc->sc_sme)) != 0) {
		aprint_error("%s: unable to register with sysmon (%d)\n",
		    sc->sc_dev.dv_xname, error);
		goto out;
	}
	
	return;

out:
	sysmon_envsys_destroy(sc->sc_sme);
}


void
adt7467c_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct adt7467c_softc *sc=sme->sme_cookie;
	int i;
	uint8_t reg;
	
	i = edata->sensor;
	reg = sc->regs[i];
	switch (edata->units)
	{
		case ENVSYS_STEMP:
			edata->value_cur = 
			    temp2muk(adt7467c_readreg(sc, reg));
			break;
			
		case ENVSYS_SVOLTS_DC:
			{
				uint32_t vr = adt7467c_readreg(sc, reg);
				edata->value_cur = 
				    (int)((vr * 2500000) / 0xc0);
			}
			break;
			
		case ENVSYS_SFANRPM:
			{
				uint16_t blah;
				blah = (((uint16_t)adt7467c_readreg(sc, reg)) | 
				    ((uint16_t)adt7467c_readreg(sc, reg + 1) <<
				    8));
				edata->value_cur = reg2rpm(blah);
			}
			break;
	}
	edata->state = ENVSYS_SVALID;
}
#endif /* NSYSMON_ENVSYS > 0 */
