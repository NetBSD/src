/*	$NetBSD: adt7463.c,v 1.6.2.1 2007/06/15 10:37:18 liamjfoy Exp $ */

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

/*
 * Analog devices AD7463 remote thermal controller and voltage monitor
 * Data sheet at:
 * http://www.analog.com/UploadedFiles/Data_Sheets/272624927ADT7463_c.pdf
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: adt7463.c,v 1.6.2.1 2007/06/15 10:37:18 liamjfoy Exp $");

/* Fan speed control added by Hanns Hartman */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <dev/sysmon/sysmonvar.h>
#include <dev/i2c/i2cvar.h>

#include <dev/i2c/adt7463reg.h>


int adt7463c_gtredata(struct sysmon_envsys *, struct envsys_tre_data *);

static int adt7463c_send_1(struct adt7463c_softc *sc, u_int8_t val);
static int adt7463c_receive_1(struct adt7463c_softc *sc);
static int adt7463c_write_1(struct adt7463c_softc *sc,  u_int8_t cmd, u_int8_t val);

static void adt7463c_setup_volt(struct adt7463c_softc *sc, int start, int tot);
static void adt7463c_setup_temp(struct adt7463c_softc *sc,  int start, int tot);
static void adt7463c_setup_fan(struct adt7463c_softc *sc,  int start, int tot);
static void adt7463c_refresh_volt(struct adt7463c_softc *sc);
static void adt7463c_refresh_temp(struct adt7463c_softc *sc);
static void adt7463c_refresh_fan(struct adt7463c_softc *sc);
static int  adt7463c_verify(struct adt7463c_softc *sc);

static int  adt7463c_match(struct device *, struct cfdata *, void *);
static void adt7463c_attach(struct device *, struct device *, void *);


CFATTACH_DECL(adt7463c, sizeof(struct adt7463c_softc),
    adt7463c_match, adt7463c_attach, NULL, NULL);

static int
adt7463c_match(struct device *parent, struct cfdata *cf,
    void *aux)
{
        struct i2c_attach_args *ia = aux;
	struct adt7463c_softc sc;
	sc.sc_tag = ia->ia_tag;
        sc.sc_address = ia->ia_addr;

	if(adt7463c_verify(&sc))
	  return (1);
	
        return (0); 
}

static void
adt7463c_attach(struct device *parent, struct device *self, void *aux)
{
        struct adt7463c_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;
	int i = 0;

	printf("\n");

	sc->sc_tag = ia->ia_tag;
        sc->sc_address = ia->ia_addr;

	/* start ADT7463 */
	adt7463c_write_1(sc,  ADT7463_CONFIG_REG1, ADT7463_START);
	/* set config reg3 to enable fast TACH measurements */
	adt7463c_write_1(sc,  ADT7463_CONFIG_REG3, ADT7463_CONFIG_REG3_FAST);
	
	/* begin fan speed control addition */
	/* associate each fan with Temp zone 2 */
	adt7463c_write_1(sc,  FANZONEREG1, TEMPCHANNEL);
	adt7463c_write_1(sc,  FANZONEREG2, TEMPCHANNEL);
	adt7463c_write_1(sc,  FANZONEREG3, TEMPCHANNEL);
	
	/* set Tmin */
	adt7463c_write_1(sc,  TMINREG,     TMINTEMP);
	/* set fans to always on when below Tmin */
	adt7463c_write_1(sc,  FANONREG,    ALWAYSON);
	
	/* set min fan speed */
	adt7463c_write_1(sc,  FANMINREG1,  FANMINSPEED);
	adt7463c_write_1(sc,  FANMINREG2,  FANMINSPEED);
	adt7463c_write_1(sc,  FANMINREG3,  FANMINSPEED);
	
	/* set Trange */
	adt7463c_write_1(sc,  TRANGEREG,   TRANGEVAL);
	/* set Tterm */
	adt7463c_write_1(sc,  TTERMREG,    TTERMVAL);
	/* set operating point */
	adt7463c_write_1(sc,  OPPTREG,     OPPTTEMP);
	/* set Tlow */
	adt7463c_write_1(sc,  TLOWREG,     TLOW);
	/* set Thigh */
	adt7463c_write_1(sc,  THIGHREG,    THIGH);
	
	/* turn on dynamic control */
	adt7463c_write_1(sc,  ENABLEDYNAMICREG, REMOTE2);
	/* set a hyst value */
	adt7463c_write_1(sc,THYSTREG,      THYST);
	/* done with fan speed control additions */
	
	/* Initialize sensors */
	adt7463c_setup_volt(sc, 0, ADT7463_VOLT_SENSORS_COUNT);
	adt7463c_setup_temp(sc, ADT7463_VOLT_SENSORS_COUNT,
			    ADT7463_TEMP_SENSORS_COUNT);
	adt7463c_setup_fan(sc, ADT7463_VOLT_SENSORS_COUNT+ADT7463_TEMP_SENSORS_COUNT,
			   ADT7463_FAN_SENSORS_COUNT);
	
	for (i = 0; i < ADT7463_MAX_ENVSYS_RANGE; ++i) {
	  sc->sc_sensor[i].sensor = sc->sc_info[i].sensor = i;
	  sc->sc_sensor[i].validflags = (ENVSYS_FVALID|ENVSYS_FCURVALID);
	  sc->sc_info[i].validflags = ENVSYS_FVALID;
	  sc->sc_sensor[i].warnflags = ENVSYS_WARN_OK;
	}      
	
	/* Hook into the System Monitor. */
	sc->sc_sysmon.sme_ranges = adt7463c_ranges;
	sc->sc_sysmon.sme_sensor_info = sc->sc_info;
	sc->sc_sysmon.sme_sensor_data = sc->sc_sensor;
	sc->sc_sysmon.sme_cookie = sc;
	
	/* callback for envsys get data */
	sc->sc_sysmon.sme_gtredata = adt7463c_gtredata;
	
	sc->sc_sysmon.sme_nsensors = ADT7463_MAX_ENVSYS_RANGE;
	sc->sc_sysmon.sme_envsys_version = 1000;
	
	sc->sc_sysmon.sme_flags = 0;	
	
	if (sysmon_envsys_register(&sc->sc_sysmon))
	printf("adt7463: unable to register with sysmon\n");
}


static int
adt7463c_verify(struct adt7463c_softc *sc)
{
      /* verify this is an adt7463 */
      int c_id, d_id;
      adt7463c_send_1(sc,  ADT7463_COMPANYID_REG);
      c_id = adt7463c_receive_1(sc);
      adt7463c_send_1(sc,  ADT7463_DEVICEID_REG);
      d_id = adt7463c_receive_1(sc);
      
      if ( (c_id == ADT7463_COMPANYID) &&
	   (d_id == ADT7463_DEVICEID) ) {
	return (1);
      }
      return (0);      
}

static void
adt7463c_setup_volt(struct adt7463c_softc *sc, int start, int tot)
{  
        sc->sc_sensor[start+0].units = sc->sc_info[start+0].units = ENVSYS_SVOLTS_DC;
	snprintf(sc->sc_info[start+0].desc, sizeof(sc->sc_info[start+0].desc), "2.5V");
	sc->sc_info[start+0].rfact = 10000;
	sc->sc_sensor[start+1].units = sc->sc_info[start+1].units = ENVSYS_SVOLTS_DC;	
	snprintf(sc->sc_info[start+1].desc, sizeof(sc->sc_info[start+1].desc), "VCCP");
	sc->sc_info[start+1].rfact = 10000;
	sc->sc_sensor[start+2].units = sc->sc_info[start+2].units = ENVSYS_SVOLTS_DC;
	snprintf(sc->sc_info[start+2].desc, sizeof(sc->sc_info[start+2].desc), "VCC");
	sc->sc_info[start+2].rfact = 10000;
	sc->sc_sensor[start+3].units = sc->sc_info[start+3].units = ENVSYS_SVOLTS_DC;
	snprintf(sc->sc_info[start+3].desc, sizeof(sc->sc_info[start+3].desc), "5V");
	sc->sc_info[start+3].rfact = 10000;
	sc->sc_sensor[start+4].units = sc->sc_info[start+4].units = ENVSYS_SVOLTS_DC;
	snprintf(sc->sc_info[start+4].desc, sizeof(sc->sc_info[start+4].desc), "12V");
	sc->sc_info[start+4].rfact = 10000;
}


static void
adt7463c_setup_temp(struct adt7463c_softc *sc,  int start, int tot)
{
       sc->sc_sensor[start+0].units = sc->sc_info[start+0].units = ENVSYS_STEMP;
       snprintf(sc->sc_info[start + 0].desc,
		sizeof(sc->sc_info[start + 0].desc), "Temp-1");

       sc->sc_sensor[start+1].units = sc->sc_info[start+1].units = ENVSYS_STEMP;
       snprintf(sc->sc_info[start + 1].desc,
		sizeof(sc->sc_info[start + 1].desc), "Temp-2");       

       sc->sc_sensor[start+2].units = sc->sc_info[start+2].units = ENVSYS_STEMP;
       snprintf(sc->sc_info[start + 2].desc,
		sizeof(sc->sc_info[start + 2].desc), "Temp-3");

}

static void
adt7463c_setup_fan(struct adt7463c_softc *sc,  int start, int tot)
{
        sc->sc_sensor[start + 0].units = ENVSYS_SFANRPM;
	sc->sc_info[start + 0].units = ENVSYS_SFANRPM;
	snprintf(sc->sc_info[start + 0].desc,
		 sizeof(sc->sc_info[start + 0].desc), "Fan-1");

	sc->sc_sensor[start + 1].units = ENVSYS_SFANRPM;
	sc->sc_info[start + 1].units = ENVSYS_SFANRPM;
	snprintf(sc->sc_info[start + 1].desc,
		 sizeof(sc->sc_info[start + 1].desc), "Fan-2");
	
	sc->sc_sensor[start + 2].units = ENVSYS_SFANRPM;
	sc->sc_info[start + 2].units = ENVSYS_SFANRPM;
	snprintf(sc->sc_info[start + 2].desc,
		 sizeof(sc->sc_info[start + 2].desc), "Fan-3");

	sc->sc_sensor[start + 3].units = ENVSYS_SFANRPM;
	sc->sc_info[start + 3].units = ENVSYS_SFANRPM;
	snprintf(sc->sc_info[start + 3].desc,
		 sizeof(sc->sc_info[start + 3].desc), "Fan-4");	
	
}

int
adt7463c_gtredata(sme, tred)
	 struct sysmon_envsys *sme;
	 struct envsys_tre_data *tred;
{  
        struct adt7463c_softc *sc = sme->sme_cookie;

	adt7463c_refresh_volt(sc);
	adt7463c_refresh_temp(sc);
	adt7463c_refresh_fan(sc);
	
	*tred = sc->sc_sensor[tred->sensor];
	return (0);
  
}

void
adt7463c_refresh_volt(struct adt7463c_softc *sc)
{
        size_t i;
        u_int8_t reg;
        int data;
	static const uint32_t mult[] = {
		ADT7463_2_5V_CONST,
		ADT7463_VCC_CONST,
		ADT7463_3_3V_CONST,
		ADT7463_5V_CONST,
		ADT7463_12V_CONST
	};

        reg = ADT7463_VOLT_REG_START;
        for (i = 0; i < ADT7463_VOLT_SENSORS_COUNT; i++) {
		adt7463c_send_1(sc,  reg++);
		data = adt7463c_receive_1(sc);

		/* envstat assumes that voltage is in uVDC */
		if (data > 0)
			sc->sc_sensor[i].cur.data_us = data * 100 * mult[i];
		else
			sc->sc_sensor[i].cur.data_us = 0;
	}
}

void
adt7463c_refresh_temp(struct adt7463c_softc *sc)
{
        int i = 0;
	u_int8_t reg;
	int data;    
	
	reg = ADT7463_TEMP_REG_START;
	for (i = 0; i < ADT7463_TEMP_SENSORS_COUNT; i++) {      
	  adt7463c_send_1(sc,  reg++);
	  data = adt7463c_receive_1(sc);
	  
	  /* envstat assumes temperature is in micro kelvin */
	  if (data > 0)
	    sc->sc_sensor[i + ADT7463_VOLT_SENSORS_COUNT].cur.data_us
	      = (data * 100 + ADT7463_CEL_TO_KELVIN) * 10000;
	  else
	    sc->sc_sensor[i + ADT7463_VOLT_SENSORS_COUNT].cur.data_us = 0;      
	}    
}


void
adt7463c_refresh_fan(struct adt7463c_softc *sc)
{
        int i, j;
	u_int8_t reg;
	int data = 0;
	u_int16_t val = 0;    
	u_char buf[2];    
	
	reg = ADT7463_FAN_REG_START;
	for (i = 0; i < ADT7463_FAN_SENSORS_COUNT; i++) {
	  
	  /* read LSB and then MSB */
	  for (j = 0; j < 2; j++) {	
	    adt7463c_send_1(sc,  reg++);
	    data = adt7463c_receive_1(sc);
	    if (data > 0)
	      buf[j] = data;
	    else
	      buf[j] = 0;	
	  }
	  
	  val = le16dec(buf);
	  
#if _BYTE_ORDER == _BIG_ENDIAN
	  val = LE16TOH(val);    
#endif      
	  
	  /* calculate RPM */
	  if (val > 0)
	    sc->sc_sensor[i + ADT7463_VOLT_SENSORS_COUNT +
			  ADT7463_TEMP_SENSORS_COUNT].cur.data_us
	      = (ADT7463_RPM_CONST)/val;
	  else
	    sc->sc_sensor[i + ADT7463_VOLT_SENSORS_COUNT +
			  ADT7463_TEMP_SENSORS_COUNT].cur.data_us = 0;
	}
}

int
adt7463c_receive_1(struct adt7463c_softc *sc)
{
        u_int8_t val = 0;
	int error = 0;
	if ((error = iic_acquire_bus(sc->sc_tag, 0)) != 0)
	  return (error);

	if ((error = iic_exec(sc->sc_tag, I2C_OP_READ,
			      sc->sc_address, NULL, 0, &val, 1, 0)) != 0) {
		iic_release_bus(sc->sc_tag, 0);
		return (error);
	}
	
	iic_release_bus(sc->sc_tag, 0);
	return (val);
}

int
adt7463c_send_1(struct adt7463c_softc *sc, u_int8_t val)
{
        int error = 0;
	if ((error = iic_acquire_bus(sc->sc_tag, 0)) != 0)
	  return (error);

	if ((error = iic_exec(sc->sc_tag, I2C_OP_WRITE,
			      sc->sc_address, NULL, 0, &val, 1, 0)) != 0) {
		iic_release_bus(sc->sc_tag, 0);
	  	return (error);
	}
	
	iic_release_bus(sc->sc_tag, 0);
	return (0);
}

int
adt7463c_write_1(struct adt7463c_softc *sc,  u_int8_t cmd, u_int8_t val)
{
        int error = 0;
	if ((error = iic_acquire_bus(sc->sc_tag, 0)) != 0)
	  return (error);

	if ((error = iic_exec(sc->sc_tag, I2C_OP_WRITE,
			      sc->sc_address, &cmd, 1, &val, 1, 0)) != 0) {
		iic_release_bus(sc->sc_tag, 0);
	  	return (error);
	}
	
	iic_release_bus(sc->sc_tag, 0);
	return (0);
}
