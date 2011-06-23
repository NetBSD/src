/*	$NetBSD: autoconf.h,v 1.15.50.1 2011/06/23 14:19:21 cherry Exp $	*/

/*-
 * Copyright (C) 1998	Internet Research Institute, Inc.
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
 *	This product includes software developed by
 *	Internet Research Institute, Inc.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _MACHINE_AUTOCONF_H_
#define _MACHINE_AUTOCONF_H_

#include <machine/bus.h>	/* for bus_space_tag_t */

struct confargs {
	const char *ca_name;
	u_int ca_node;
	int ca_nreg;
	u_int *ca_reg;
	int ca_nintr;
	int *ca_intr;

	bus_addr_t ca_baseaddr;
	bus_space_tag_t ca_tag;
};

/* there are in locore.S */
void ofbcopy(const void *, void *, size_t);
int badaddr(volatile void *, int);

/* these are in clock.c */
void calc_delayconst(void);
void decr_intr(struct clockframe *);

/* these are in cpu.c */
void identifycpu(char *);

/* these are in machdep.c */
void initppc(u_int, u_int, char *);
void model_init(void);
void *mapiodev(paddr_t, psize_t);
paddr_t kvtop(void *);
void dumpsys(void);
void copy_disp_props(device_t, int, prop_dictionary_t);

/* these are in extintr.c */
void init_interrupt(void);

/* these are in dev/akbd.c */
int kbd_intr(void *);
int akbd_cnattach(void);
int adbkbd_cnattach(void);

/* these are in dev/ofb.c */
int ofb_is_console(void);
int rascons_cnattach(void);

extern int console_node;
extern int console_instance;
extern char model_name[64];

#endif /* _MACHINE_AUTOCONF_H_ */
