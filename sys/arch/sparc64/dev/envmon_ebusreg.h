/*	$NetBSD: envmon_ebusreg.h,v 1.1 2026/09/07 07:35:14 jdc Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julian Coleman.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Register definitions for "env-monitor":
 *   "SUNW,ebus-pic16f747-env" on U45/U25
 *   "epic" on V245/V215
 * Registers are indirectly accessed through a address and data register.
 *
 * Register contents are mostly copies of registers from other chips on
 * the i2c bus.  On V245/V215, they appear to be only from the chip at
 * i2c addr 0x5a, and on the U45, from the adt7462 and lm95221.
 * However, some registers contain sensor data not available elsewhere,
 * e.g.  front panel temperature on U45.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: envmon_ebusreg.h,v 1.1 2026/09/07 07:35:14 jdc Exp $");

#define ENVMON_COMPAT_PIC	"SUNW,ebus-pic16f747-env"
#define ENVMON_COMPAT_EPIC	"epic"

/* env-monitor registers */
#define ENVMON_DATA		0x40	/* Read/write register data */
#define ENVMON_ADDR		0x41	/* Register address to access */
#define ENVMON_EPC_LED_MASK	0x80	/* LED write mask */

/* Indirect registers: U45/U25 */
#define ENVMON_PIC_FAN0		0x00	/* Fan 0 speed */
#define ENVMON_PIC_FAN1		0x01	/* Fan 1 speed */
#define ENVMON_PIC_FAN2		0x02	/* Fan 2 speed */
#define ENVMON_PIC_FAN3		0x03	/* Fan 3 speed */
#define ENVMON_PIC_FAN4		0x04	/* Fan 4 speed */

#define ENVMON_PIC_TEMP_ADT	0x06	/* ADT7462 temperature */
#define ENVMON_PIC_TEMP_CPU0	0x07	/* CPU 0 temperature */
#define ENVMON_PIC_TEMP_CPU1	0x08	/* CPU 1 temperature */
#define ENVMON_PIC_TEMP_MB	0x09	/* Motherboard temperature */
#define ENVMON_PIC_TEMP_LM	0x0a	/* LM95221 temperature */
#define ENVMON_PIC_TEMP_FIRE	0x0b	/* FireASIC temperature */
#define ENVMON_PIC_TEMP_LSI	0x0c	/* LSI1064 temperature */
#define ENVMON_PIC_TEMP_FP	0x0d	/* Front Panel temperature */

#define ENVMON_PIC_VOLT_15_1	0x0f	/* 1.5v #1 */
#define ENVMON_PIC_VOLT_15_2	0x10	/* 1.5v #2 */

#define ENVMON_PIC_TEMP_PSU	0x13	/* Power Supply temperature */

/* 0x0f/0x10: Voltage scale for 1.5v registers */
#define ENVMON_PIC_SCALE_1_5V	7800	

/* Indirect registers: V245/V215 */
#define ENVMON_EPC_KEYSW	0x04	/* Virtual keyswitch */
#define ENVMON_EPC_FW		0x05	/* Firmware version */
#define ENVMON_EPC_LED6		0x06	/* Locator / power LED's */
#define ENVMON_EPC_LED7		0x07	/* PSU / temperature LED's */
#define ENVMON_EPC_FAN_FT0_F0	0x0e	/* Fan tray 0 fan 0 */
#define ENVMON_EPC_FAN_FT1_F0	0x0f	/* Fan tray 1 fan 0 */
#define ENVMON_EPC_FAN_FT2_F0	0x10	/* Fan tray 2 fan 0 */
#define ENVMON_EPC_FAN_FT3_F0	0x11	/* Fan tray 3 fan 0 */
#define ENVMON_EPC_FAN_FT4_F0	0x12	/* Fan tray 4 fan 0 */
#define ENVMON_EPC_FAN_FT5_F0	0x13	/* Fan tray 5 fan 0 */
#define ENVMON_EPC_FAN_FT0_F1	0x14	/* Fan tray 0 fan 1 (V215 only) */
#define ENVMON_EPC_FAN_FT1_F1	0x15	/* Fan tray 1 fan 1 (V215 only) */
#define ENVMON_EPC_FAN_FT2_F1	0x16	/* Fan tray 2 fan 1 (V215 only) */
#define ENVMON_EPC_FAN_FT3_F1	0x17	/* Fan tray 3 fan 1 (V215 only) */
#define ENVMON_EPC_FAN_FT4_F1	0x18	/* Fan tray 4 fan 1 (V215 only) */
#define ENVMON_EPC_FAN_FT5_F1	0x19	/* Fan tray 5 fan 1 (V215 only) */
#define ENVMON_EPC_TEMP_FP	0x22	/* Front Panel temperature */
#define ENVMON_EPC_FAN_FLT	0x30	/* Fan fault LED's */

/* 0x04: Virtual keyswitch */
#define ENVMON_EPC_KEYSW_NORM		0x00	/* Normal or Diag */
#define ENVMON_EPC_KEYSW_OFF		0x02	/* Power off */
#define ENVMON_EPC_KEYSW_LOCK		0x04	/* Locked */

/* 0x06: Front panel LED's */
#define ENVMON_EPC_LED6_LOC		0x01	/* Locator */
#define ENVMON_EPC_LED6_LOC2		0x02	/* Locator too */
#define ENVMON_EPC_LED6_SRV		0x04	/* Service */
#define ENVMON_EPC_LED6_SRV_FSH		0x08	/* Service flashing */
#define ENVMON_EPC_LED6_PWR		0x10	/* Power */
#define ENVMON_EPC_LED6_TF_FLT		0x40	/* Top fan fault */
#define ENVMON_EPC_LED6_TF_FLT_FSH	0x80	/* Top fan fault flashing */

/* 0x07: Front panel LED's */
#define ENVMON_EPC_LED7_PSU		0x01	/* PSU fault */
#define ENVMON_EPC_LED7_PSU_FSH		0x02	/* PSU fault. flashing */
#define ENVMON_EPC_LED7_CTEMP		0x04	/* CPU Overtemperature */
#define ENVMON_EPC_LED7_CTEMP_FSH	0x08	/* CPU Overtemp. flashing */

/* 0x30: Fan fault */
#define ENVMON_EPC_NUM_FAN_FLTS		6
#define ENVMON_EPC_FAN_FLT_NONE		0x3f

/* For LED's with flash: both LED and LED_flashing set = LED fast flash. */

/* Front panel temperature base */
#define ENVMON_TEMP_BASE	209150000	/* -64'C */
