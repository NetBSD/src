/*	$NetBSD: itvar.h,v 1.4 2007/07/01 07:37:20 xtraeme Exp $	*/
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

#define IT_NUM_SENSORS	9

/* Identification chip registers */
#define IT_VENDORID	0x58
#define IT_REV_8705	0x90
#define IT_COREID	0x5b
#define IT_REV_8712	0x12

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

#define IT_VOLTENABLE 	0x50
#define IT_TEMPENABLE 	0x51

#define IT_FANMINBASE	0x10
#define IT_FANENABLE	0x13

#define IT_SENSORFANBASE	0x0d	/* Fan from 0x0d to 0x0f */
#define IT_SENSORVOLTBASE	0x20	/* Fan from 0x20 to 0x28 */
#define IT_SENSORTEMPBASE	0x29	/* Fan from 0x29 to 0x2b */

#define IT_SENSORVCORE0		0x20
#define IT_SENSORV33		0x22
#define IT_SENSORV5		0x23
#define IT_SENSORV12		0x24
#define IT_SENSORVBAT		0x28

#define IT_VOLTMAXBASE		0x30
#define IT_VOLTMINBASE		0x31

#define IT_TEMPMAXBASE		0x40
#define IT_TEMPMINBASE		0x41

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
};

#endif /* _DEV_ISA_ITVAR_H_ */
