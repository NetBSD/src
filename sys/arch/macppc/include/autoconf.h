/*	$NetBSD: autoconf.h,v 1.4 2001/06/08 00:32:03 matt Exp $	*/

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

struct confargs {
	char *ca_name;
	u_int ca_node;
	int ca_nreg;
	u_int *ca_reg;
	int ca_nintr;
	int *ca_intr;

	u_int ca_baseaddr;
	/* bus_space_tag_t ca_tag; */
};

/* there are in locore.S */
void ofbcopy __P((void *, void *, size_t));
int badaddr __P((void *, int));

/* these are in autoconf.c */
int getnodebyname __P((int, const char *));
int OF_interpret __P((char *cmd, int nreturns, ...));

/* these are in clock.c */
void calc_delayconst __P((void));
void decr_intr __P((struct clockframe *));

/* these are in cpu.c */
void identifycpu __P((char *));

/* these are in machdep.c */
void initppc __P((u_int, u_int, char *));
void install_extint __P((void (*) __P((void)))); 
void *mapiodev __P((paddr_t, psize_t));
int kvtop __P((caddr_t));

/* these are in extintr.c */
void ext_intr __P((void));
void init_interrupt __P((void));
void *intr_establish __P((int, int, int, int (*)(void *), void *));
void intr_disestablish __P((void *));
char *intr_typename __P((int));

/* these are in dev/akbd.c */
int kbd_intr __P((void *));
int akbd_cnattach __P((void));

/* these are in dev/ofb.c */
int ofb_is_console __P((void));
int ofb_cnattach __P((void));

/* these are in dev/zs.c */
int zssoft __P((void *));

/* these are in ../../dev/ic/com.c */
void comsoft __P((void));

#endif /* _MACHINE_AUTOCONF_H_ */
