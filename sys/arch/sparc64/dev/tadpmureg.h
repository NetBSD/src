/* $NetBSD: tadpmureg.h,v 1.2.2.2 2018/10/20 06:58:29 pgoyette Exp $ */

/*-
 * Copyright (c) 2018 Michael Lorenz <macallan@netbsd.org>
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

/* registers and commands for the PMU found in Tadpole Viper laptops */

#ifndef TADPMUREG_H
#define TADPMUREG_H

/* all values were found by looking at OF methods on the tadpmu node */

/* these registers live in the pckbc's address space */
#define TADPMU_CMD	0x6
#define TADPMU_STATUS	0x6
#define TADPMU_DATA	0x2

#define STATUS_HAVE_DATA	0x01	/* response from command */
#define STATUS_CMD_IN_PROGRESS	0x02
#define STATUS_INTR		0x04	/* interrupt happened, read data to ack */
#define STATUS_SEND_DATA	0x08	/* cmd waiting for data */

#define CMD_SET_OPMODE		0x41	/* not sure what exactly this does... */
#define		OPMODE_UNIX	0x75	/* other than toggling the UNIX mode  */
#define		OPMODE_OF	0x67	/* bit in the GENSTAT register        */
#define CMD_SET_BACKLIGHT	0x43	/* apparently 0 - 1f */
#define CMD_SET_CPUSPEED	0x44	/* in 10MHz, so 120 == 1.2GHz */
#define CMD_SET_FANSPEED	0x46	/* ??? */
#define CMD_SET_VOLUME		0x48	/* beeper volume */

#define CMD_READ_GENSTAT	0x10
#define CMD_READ_BACKLIGHT	0x11
#define CMD_READ_SYSTEMP	0x12	/* temperature */
#define CMD_READ_VOLUME		0x13
#define CMD_READ_VBATT		0x14
#define CMD_READ_VERSION	0x15
#define CMD_READ_CPUSPEED	0x16
/* 0x17 returns a byte, always 0 */
/* 0x18 returns a byte, always(?) 0x77 */ 
#define CMD_READ_GENSTAT2	0x19
#define CMD_READ_FANSPEED	0x50	/* takes a parameter, returns nothing? */

/* these are according to the ROM methods
   #define GENSTAT_DC_PRESENT		0x01
   #define GENSTAT_DC_ENABLE		0x02
   #define GENSTAT_BATTERY_PRESENT	0x04
   #define GENSTAT_BATTERY_CHARGING	0x08
   #define GENSTAT_LID_CLOSED		0x10
   #define GENSTAT_UNIX_MODE		0x20
   #define GENSTAT_SPREADSPECTRUM	0x40
*/

/* these are according to experiment */
#define GENSTAT_UNIX_MODE		0x01
   #define GENSTAT_DC_PRESENT		0x08	/* guess */
   #define GENSTAT_DC_ENABLE		0x18	/* guess */
#define GENSTAT_LID_CLOSED		0x80

#define GENSTAT2_MUTE		0x02

/* messages from interrupts */
#define TADPMU_LID		0x05
#define TADPMU_POWERBUTTON	0x06

#endif /* TADPMUREG_H */