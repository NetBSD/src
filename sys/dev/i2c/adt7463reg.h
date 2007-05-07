/*	$NetBSD: adt7463reg.h,v 1.2.30.1 2007/05/07 10:55:25 yamt Exp $ */

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

/* Fan speed control added by Hanns Hartman */

#ifndef INCLUDE_ADT7463REG_H
#define INCLUDE_ADT7463REG_H

#define ADT7463_VOLT_SENSORS_COUNT 5
#define ADT7463_TEMP_SENSORS_COUNT 3
#define ADT7463_FAN_SENSORS_COUNT 4
#define ADT7463_MAX_ENVSYS_RANGE 12 /* sum of the above */

/* I2C/SMBUS address */
#define ADT7463_ADDR1                0x2C
#define ADT7463_ADDR2                0x2D
#define ADT7463_ADDR3                0x2E

#define ADT7463_CONFIG_REG1         0x40
#define ADT7463_CONFIG_REG3         0x78
#define ADT7463_START               0x01
#define ADT7463_COMPANYID_REG       0x3E
#define ADT7463_COMPANYID           0x41
#define ADT7463_DEVICEID_REG        0x3D
#define ADT7463_DEVICEID            0x27
  
#define ADT7463_VOLT_REG_START      0x20
#define ADT7463_TEMP_REG_START      0x25
#define ADT7463_FAN_REG_START       0x28

#define ADT7463_CONFIG_REG3_FAST    0x08

/* currently we use only 8 bits and hence the multiplier */
#define ADT7463_12V_CONST           (0.0625)
#define ADT7463_5V_CONST            (0.0260)
#define ADT7463_3_3V_CONST          (0.0171)
#define ADT7463_2_5V_CONST          (0.0130)
#define ADT7463_VCC_CONST           (0.0117)

#define ADT7463_CEL_TO_KELVIN       273.15
#define ADT7463_RPM_CONST           (90000 * 60)

const struct envsys_range adt7463c_ranges[] = {
  
	{ 5, 7,       ENVSYS_STEMP   },
	{ 8, 11,      ENVSYS_SFANRPM },
	{ 1, 0,       ENVSYS_SVOLTS_AC },	/* None */
	{ 0, 4,       ENVSYS_SVOLTS_DC },
	{ 1, 0,       ENVSYS_SOHMS },	/* None */
	{ 1, 0,       ENVSYS_SWATTS },	/* None */
	{ 1, 0,       ENVSYS_SAMPS }	/* None */
};  

struct adt7463c_softc {
	struct device sc_dev;		/* generic device structures */
	i2c_tag_t sc_tag;
	i2c_addr_t sc_address;

	struct envsys_tre_data sc_sensor[ADT7463_MAX_ENVSYS_RANGE];
	struct envsys_basic_info sc_info[ADT7463_MAX_ENVSYS_RANGE];

	struct sysmon_envsys sc_sysmon;  
  
};

/* Fan speed control define(s) 
 * All below references to page numbers refer to the Automatic Fan
 * Speed Control App Note
 */
 
/* step two setting temperature zone 2
 * page 5 gives specific information about how to program the temperature 
 * channel.  Also note that the low order byte of 2 should not be changed.
 */
#define FANZONEREG1 0x5C
#define FANZONEREG2 0x5D
#define FANZONEREG3 0x5E
#define TEMPCHANNEL 0x42

/* Minimum temperature remote zone 2 (page 7) */   
#define TMINREG 0x69
#define TMINTEMP 0x2C

/* keep the fans always on
 * please see page 7 for which  bit to set to enable a 
 * pwm to be left always on.
 */
#define FANONREG 0x62
#define ALWAYSON 0xE0

/* minimum fan speed
 * computing the number for FANMINSPEED is done by converting
 * percent fan speed to a pwm number using the equation on page 8
 */
#define FANMINREG1 0x64
#define FANMINREG2 0x65
#define FANMINREG3 0x66
#define FANMINSPEED 0x45

/* give a Trange this is the slope at which the fan speed will 
 * increase based on temperature
 * please make sure not to change the low order byte of 4 if adjusting
 * this value.  In order to calculate Trange use the equation on page 9
 * note that this is the best value given the current bios situation
 */
#define TRANGEREG 0x61
#define TRANGEVAL 0x94

/* This is the hyst value.  once the operating temperature-hyst 
 * is broken the fan speed will start to increase
 * consult page 12 for what to put in what register
 */
#define THYSTREG 0x6E
#define THYST 0x20

/* this is the value when reach will cause the fans to drive at full speed
 * see page 12
 */
#define TTERMREG 0x6C
#define TTERMVAL 0x39

/* This is the desired operating temperature for the cpu
 * see page 15-16 for more detail
 */
#define OPPTREG 0x35
#define OPPTTEMP 0x34

/* Once the temperature falls below this point the 
 * fans speed will start to decrease
 * see page 17 for more detail
 */
#define TLOWREG 0x52
#define TLOW 0x2D

/* Once the temperature rises above this point the fan speed 
 * will be increased at a more rapid rate
 * see page 17 for more detail
 */
#define THIGHREG 0x53
#define THIGH 0x36

/* Enable dynamic control on remote2 given a polling interval
 * please see page 18-22 in setting values for register 0x36
 */
#define ENABLEDYNAMICREG 0x36 
#define REMOTE2 0x80

/* done with fan speed control additions */

#endif
