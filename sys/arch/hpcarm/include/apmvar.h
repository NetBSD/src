/*	$NetBSD: apmvar.h,v 1.1 2002/09/16 19:52:55 manu Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

struct apm_attach_args {
	int aaa_magic;
};
#define APM_ATTACH_ARGS_MAGIC 0x0061706d

/* 
 * All definitions from here are duplicated in i386 and macppc
 * versions. The definitions are the same, it is probably worth
 * moving them in a MI place.
 */

#define	 APM_AC_OFF		0x00
#define	 APM_AC_ON		0x01
#define	 APM_AC_BACKUP		0x02
#define	 APM_AC_UNKNOWN		0xff
#define	 APM_BATT_HIGH		0x00
#define	 APM_BATT_LOW		0x01
#define	 APM_BATT_CRITICAL	0x02
#define	 APM_BATT_CHARGING	0x03
#define	 APM_BATT_ABSENT	0x04
#define	 APM_BATT_UNKNOWN	0xff
#define	 APM_BATT_LIFE_UNKNOWN	0xff

struct apm_power_info {
	u_char battery_state;
	u_char ac_state;
	u_char battery_life;
	u_char spare1;
	u_int minutes_left;	     /* estimate */
	u_int nbattery;
	u_int batteryid;
	u_int spare2[4];
}; 

/* ioctl definitions */
#define APM_IOC_STANDBY _IO('A', 1)
#define APM_IOC_SUSPEND _IO('A', 2)
#define APM_IOC_GETPOWER _IOR('A', 3, struct apm_power_info) 
