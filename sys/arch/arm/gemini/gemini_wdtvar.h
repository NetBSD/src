/*	$NetBSD: gemini_wdtvar.h,v 1.1 2008/10/24 04:23:18 matt Exp $	*/

/*
 * adapted/extracted from omap_wdt.c
 *
 * Copyright (c) 2007 Microsoft
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
 *	This product includes software developed by Microsoft
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTERS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef  _ARM_GEMINI_WDTVAR_H 
#define  _ARM_GEMINI_WDTVAR_H

#include <sys/device.h>
#include <machine/bus.h>
#include <dev/sysmon/sysmonvar.h>
 
typedef struct geminiwdt_softc {
	struct device sc_dev;
	bus_addr_t sc_addr;
	bus_size_t sc_size;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	struct sysmon_wdog sc_smw;
	int sc_armed;
} geminiwdt_softc_t;

extern struct geminiwdt_softc *geminiwdt_sc;
extern int omapwdt_sysconfig;

int	geminiwdt_setmode(struct sysmon_wdog *);
int	geminiwdt_tickle(struct sysmon_wdog *);

void	geminiwdt_set_timeout(unsigned int period);
int	geminiwdt_enable(int enable);
void	geminiwdt_reboot(void);

#endif  /* _ARM_GEMINI_WDTVAR_H */
