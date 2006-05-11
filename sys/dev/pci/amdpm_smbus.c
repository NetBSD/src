/*	$NetBSD: amdpm_smbus.c,v 1.2.8.1 2006/05/11 23:28:47 elad Exp $ */

/*
 * Copyright (c) 2005 Anil Gopinath (anil_public@yahoo.com)
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* driver for SMBUS 1.0 host controller found in the
 * AMD-8111 HyperTransport I/O Hub
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: amdpm_smbus.c,v 1.2.8.1 2006/05/11 23:28:47 elad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/rnd.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/i2c_bitbang.h>

#include <dev/pci/amdpmreg.h>
#include <dev/pci/amdpmvar.h>

#include <dev/pci/amdpm_smbusreg.h>

static int       amdpm_smbus_acquire_bus(void *cookie, int flags);
static void      amdpm_smbus_release_bus(void *cookie, int flags);
static int       amdpm_smbus_exec(void *cookie, i2c_op_t op, i2c_addr_t addr,
				  const void *cmd, size_t cmdlen, void *vbuf,
				  size_t buflen, int flags);
static int       amdpm_smbus_check_done(struct amdpm_softc *sc);
static void      amdpm_smbus_clear_gsr(struct amdpm_softc *sc);
static u_int16_t amdpm_smbus_get_gsr(struct amdpm_softc *sc);
static int       amdpm_smbus_send_1(struct amdpm_softc *sc, u_int8_t val);
static int       amdpm_smbus_write_1(struct amdpm_softc *sc, u_int8_t cmd, u_int8_t data);
static int       amdpm_smbus_receive_1(struct amdpm_softc *sc);
static int       amdpm_smbus_read_1(struct amdpm_softc *sc, u_int8_t cmd);


void
amdpm_smbus_attach(struct amdpm_softc *sc)
{
        struct i2cbus_attach_args iba;
	
	// register with iic
	sc->sc_i2c.ic_cookie = sc; 
	sc->sc_i2c.ic_acquire_bus = amdpm_smbus_acquire_bus; 
	sc->sc_i2c.ic_release_bus = amdpm_smbus_release_bus;
	sc->sc_i2c.ic_send_start = NULL;
	sc->sc_i2c.ic_send_stop = NULL;
	sc->sc_i2c.ic_initiate_xfer = NULL;
	sc->sc_i2c.ic_read_byte = NULL;
	sc->sc_i2c.ic_write_byte = NULL;
	sc->sc_i2c.ic_exec = amdpm_smbus_exec;

	lockinit(&sc->sc_lock, PZERO, "amdpm_smbus", 0, 0);

	iba.iba_name = "iic";
	iba.iba_tag = &sc->sc_i2c;
	(void) config_found(&sc->sc_dev, &iba, iicbus_print);      
}

static int
amdpm_smbus_acquire_bus(void *cookie, int flags)
{
	struct amdpm_softc *sc = cookie;
	int err;

	err = lockmgr(&sc->sc_lock, LK_EXCLUSIVE, NULL);

	return err;
}

static void
amdpm_smbus_release_bus(void *cookie, int flags)
{
	struct amdpm_softc *sc = cookie;

	lockmgr(&sc->sc_lock, LK_RELEASE, NULL);

	return;
}

static int
amdpm_smbus_exec(void *cookie, i2c_op_t op, i2c_addr_t addr, const void *cmd,
    size_t cmdlen, void *vbuf, size_t buflen, int flags)
{
        struct amdpm_softc *sc  = (struct amdpm_softc *) cookie;
	sc->sc_smbus_slaveaddr  = addr;
	
	if (I2C_OP_READ_P(op) && (cmdlen == 0) && (buflen == 1)) {
	  return (amdpm_smbus_receive_1(sc));    
	}
	
	if ( (I2C_OP_READ_P(op)) && (cmdlen == 1) && (buflen == 1)) {
	  return (amdpm_smbus_read_1(sc, *(const uint8_t*)cmd));
	}
	
	if ( (I2C_OP_WRITE_P(op)) && (cmdlen == 0) && (buflen == 1)) {
	  return (amdpm_smbus_send_1(sc, *(uint8_t*)vbuf));
	}
	
	if ( (I2C_OP_WRITE_P(op)) && (cmdlen == 1) && (buflen == 1)) {
	  return (amdpm_smbus_write_1(sc,  *(const uint8_t*)cmd, *(uint8_t*)vbuf));
	}
	
	return (-1);  
}

static int 
amdpm_smbus_check_done(struct amdpm_softc *sc)
{  
        int i = 0;    
	for (i = 0; i < 1000; i++) {
	  /* check gsr and wait till cycle is done */
	  u_int16_t data = amdpm_smbus_get_gsr(sc);
	  if (data & AMDPM_8111_GSR_CYCLE_DONE) {
	    return (0);	
	  }
	  delay(1);      
	}
	return (-1);    
}


static void
amdpm_smbus_clear_gsr(struct amdpm_softc *sc)
{
        /* clear register */
        u_int16_t data = 0xFFFF;     
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, AMDPM_8111_SMBUS_STAT, data);
}

static u_int16_t
amdpm_smbus_get_gsr(struct amdpm_softc *sc)
{
        return (bus_space_read_2(sc->sc_iot, sc->sc_ioh, AMDPM_8111_SMBUS_STAT));  
}

static int
amdpm_smbus_send_1(struct amdpm_softc *sc,  u_int8_t val)
{
        /* first clear gsr */
        amdpm_smbus_clear_gsr(sc);

	/* write smbus slave address to register */
	u_int16_t data = 0;
	data = sc->sc_smbus_slaveaddr;
	data <<= 1;
	data |= AMDPM_8111_SMBUS_SEND;    
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, AMDPM_8111_SMBUS_HOSTADDR, data);
	
	data = val;    
	/* store data */
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, AMDPM_8111_SMBUS_HOSTDATA, data);
	/* host start */
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, AMDPM_8111_SMBUS_CTRL,
			  AMDPM_8111_SMBUS_GSR_SB);	
	return(amdpm_smbus_check_done(sc));    
}

  
static int
amdpm_smbus_write_1(struct amdpm_softc *sc, u_int8_t cmd, u_int8_t val)
{
        /* first clear gsr */
        amdpm_smbus_clear_gsr(sc);  
  
	u_int16_t data = 0;
	data = sc->sc_smbus_slaveaddr;
	data <<= 1;
	data |= AMDPM_8111_SMBUS_WRITE;    
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, AMDPM_8111_SMBUS_HOSTADDR, data);
	
	data = val;    
	/* store cmd */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, AMDPM_8111_SMBUS_HOSTCMD, cmd);
	/* store data */    
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, AMDPM_8111_SMBUS_HOSTDATA, data);    
	/* host start */
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, AMDPM_8111_SMBUS_CTRL, AMDPM_8111_SMBUS_GSR_WB);
	
	return (amdpm_smbus_check_done(sc));    
}

static int
amdpm_smbus_receive_1(struct amdpm_softc *sc)
{
        /* first clear gsr */
        amdpm_smbus_clear_gsr(sc);  

	/* write smbus slave address to register */
	u_int16_t data = 0;
	data = sc->sc_smbus_slaveaddr;
	data <<= 1;
	data |= AMDPM_8111_SMBUS_RX;    
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, AMDPM_8111_SMBUS_HOSTADDR, data);
	
	/* start smbus cycle */
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, AMDPM_8111_SMBUS_CTRL, AMDPM_8111_SMBUS_GSR_RXB);
	
	/* check for errors */
	if (amdpm_smbus_check_done(sc) < 0)
	  return (-1);
	
	/* read data */
	data = bus_space_read_2(sc->sc_iot, sc->sc_ioh, AMDPM_8111_SMBUS_HOSTDATA);    
	u_int8_t ret = (u_int8_t)(data & 0x00FF);
	return (ret);
}

static int
amdpm_smbus_read_1(struct amdpm_softc *sc, u_int8_t cmd)
{  
        /* first clear gsr */
        amdpm_smbus_clear_gsr(sc);  

	/* write smbus slave address to register */
	u_int16_t data = 0;
	data = sc->sc_smbus_slaveaddr;
	data <<= 1;
	data |= AMDPM_8111_SMBUS_READ;    
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, AMDPM_8111_SMBUS_HOSTADDR, data);
	
	/* store cmd */
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, AMDPM_8111_SMBUS_HOSTCMD, cmd);
	/* host start */
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, AMDPM_8111_SMBUS_CTRL, AMDPM_8111_SMBUS_GSR_RB);
	
	/* check for errors */
	if (amdpm_smbus_check_done(sc) < 0)
	  return (-1);
	
	/* store data */    
	data = bus_space_read_2(sc->sc_iot, sc->sc_ioh, AMDPM_8111_SMBUS_HOSTDATA);
	u_int8_t ret = (u_int8_t)(data & 0x00FF);
	return (ret);
}
