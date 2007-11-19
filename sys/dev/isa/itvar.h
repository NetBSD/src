/*	$NetBSD: itvar.h,v 1.5 2007/07/05 15:20:30 xtraeme Exp $	*/
/*	$OpenBSD: itvar.h,v 1.2 2003/11/05 20:57:10 grange Exp $	*/

/*
 * Copyright (c) 2006 Juan Romero Pardines <juan@xtrarom.org>
 * Copyright (c) 2003 Julien Bordet <zejames@greyhats.org>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITD TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITD TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _DEV_ISA_ITVAR_H
#define _DEV_ISA_ITVAR_H

#define IT_NUM_SENSORS	15

/* Identification chip registers */
#define IT_VENDORID	0x58
#define IT_REV_8705	0x90
#define IT_COREID	0x5b
#define	IT_REV_8712	0x12

/* Control registers */
#define IT_ADDR 	0x05
#define IT_DATA 	0x06

/* Data registers */
#define IT_CONFIG	0x00
#define IT_ISR1 	0x01
#define IT_ISR2 	0x02
#define IT_ISR3 	0x03
#define IT_SMI1 	0x04
#define IT_SMI2 	0x05
#define IT_SMI3 	0x06
#define IT_IMR1 	0x07
#define IT_IMR2 	0x08
#define IT_IMR3 	0x09
#define IT_VID 		0x0a
#define IT_FAN 		0x0b
#define IT_FAN16 	0x0c

#define IT_VOLTENABLE 	0x50
#define IT_TEMPENABLE 	0x51

#define IT_FANMINBASE	0x10
#define IT_FANENABLE	0x13

#define IT_SENSORFANBASE	0x0d	/* Fan from 0x0d to 0x0f */
#define IT_SENSORFANEXTBASE 	0x18 	/* Fan (MSB) from 0x18 to 0x1A */
#define IT_SENSORVOLTBASE 	0x20 	/* Voltage from 0x20 to 0x28 */
#define IT_SENSORTEMPBASE 	0x29 	/* Temperature from 0x29 to 0x2b */

#define IT_VIN0 		0x20
#define IT_VIN1 		0x21
#define IT_VIN2 		0x22
#define IT_VIN3 		0x23
#define IT_VIN4 		0x24
#define IT_VIN5 		0x25
#define IT_VIN6 		0x26
#define IT_VIN7 		0x27
#define IT_VBAT 		0x28

#define IT_UPDATEVBAT		0x40 	/* Update VBAT voltage reading */
#define IT_BEEPEER		0x5c	/* Beep Event Enable Register */

/* High and Low limits for voltages */
#define IT_VIN0_HIGH_LIMIT	0x30
#define IT_VIN0_LOW_LIMIT	0x31
#define IT_VIN1_HIGH_LIMIT	0x32
#define IT_VIN1_LOW_LIMIT	0x33
#define IT_VIN2_HIGH_LIMIT	0x34
#define IT_VIN2_LOW_LIMIT	0x35
#define IT_VIN3_HIGH_LIMIT	0x36
#define IT_VIN3_LOW_LIMIT	0x37
#define IT_VIN4_HIGH_LIMIT 	0x38
#define IT_VIN4_LOW_LIMIT	0x39
#define IT_VIN5_HIGH_LIMIT	0x3a
#define IT_VIN5_LOW_LIMIT	0x3b
#define IT_VIN6_HIGH_LIMIT	0x3c
#define IT_VIN6_LOW_LIMIT	0x3d
#define IT_VIN7_HIGH_LIMIT	0x3e
#define IT_VIN7_LOW_LIMIT	0x3f

/* High and Low limits for temperatures */
#define IT_TEMP0_HIGH_LIMIT	0x40
#define IT_TEMP0_LOW_LIMIT	0x41
#define IT_TEMP1_HIGH_LIMIT	0x42
#define IT_TEMP1_LOW_LIMIT	0x43
#define IT_TEMP2_HIGH_LIMIT	0x44
#define IT_TEMP2_LOW_LIMIT	0x45

/* Reserved registers, used for probing the chip */
#define IT_RES48		0x48
#define IT_RES48_DEFAULT	0x2d
#define IT_RES52		0x52
#define IT_RES52_DEFAULT	0x7f

#define IT_VREF	(4096) /* Vref = 4.096 V */

struct it_softc {
	struct device sc_dev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;

	struct sysmon_envsys sc_sysmon;
	envsys_data_t sc_data[IT_NUM_SENSORS];

	uint8_t sc_idr;
};

#endif /* _DEV_ISA_ITVAR_H_ */
