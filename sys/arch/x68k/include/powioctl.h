/*	$NetBSD: powioctl.h,v 1.2 2004/05/08 08:40:08 minoura Exp $	*/

/*
 * Copyright (c) 1995 MINOURA Makoto.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Minoura Makoto.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Power switch device driver. */

#ifndef _X68K_POWIOCTL_H_
#define _X68K_POWIOCTL_H_

#include <sys/time.h>

enum x68k_powerswitch {
	POW_ALARMSW = 1,
	POW_EXTERNALSW = 2,
	POW_FRONTSW = 4,
};

struct x68k_powerinfo {
	int pow_switch_boottime;
	int pow_switch_current;
	time_t pow_boottime;
	unsigned int pow_bootcount;
	time_t pow_usedtotal;
};

struct x68k_alarminfo {
	int al_enable;
	unsigned int al_ontime;
	int al_dowhat;
	time_t al_offtime;
};

#define POWIOCGPOWERINFO	_IOR('p', 0, struct x68k_powerinfo)
#define POWIOCGALARMINFO	_IOR('p', 1, struct x68k_alarminfo)
#define POWIOCSALARMINFO	_IOW('p', 2, struct x68k_alarminfo)
#define POWIOCSSIGNAL		_IOW('p', 3, int)

#endif
