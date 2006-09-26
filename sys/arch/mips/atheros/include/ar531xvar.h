/* $Id: ar531xvar.h,v 1.5 2006/09/26 06:37:32 gdamore Exp $ */
/*
 * Copyright (c) 2006 Urbana-Champaign Independent Media Center.
 * Copyright (c) 2006 Garrett D'Amore.
 * All rights reserved.
 *
 * This code was written by Garrett D'Amore for the Champaign-Urbana
 * Community Wireless Network Project.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgements:
 *      This product includes software developed by the Urbana-Champaign
 *      Independent Media Center.
 *	This product includes software developed by Garrett D'Amore.
 * 4. Urbana-Champaign Independent Media Center's name and Garrett
 *    D'Amore's name may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER AND GARRETT D'AMORE ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER OR GARRETT D'AMORE BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	_MIPS_ATHEROS_AR531XVAR_H_
#define	_MIPS_ATHEROS_AR531XVAR_H_

#include <sys/param.h>
#include <machine/bus.h>

struct ar531x_device {
	const char	*name;
	bus_addr_t	addr;
	bus_size_t	size;
	int		cirq;
	int		mirq;
	uint32_t	mask;
	uint32_t	reset;
	uint32_t	enable;
};


void ar531x_intr_init(void);

void *ar531x_cpu_intr_establish(int, int (*)(void *), void *);
void ar531x_cpu_intr_disestablish(void *);

void *ar531x_misc_intr_establish(int, int (*)(void *), void *);
void ar531x_misc_intr_disestablish(void *);

void ar531x_cpuintr(uint32_t, uint32_t, uint32_t, uint32_t);


/*
 * CPU specific routines.
 */
size_t ar531x_memsize(void);
void ar531x_consinit(void);
void ar531x_wdog(uint32_t);
void ar531x_businit(void);
const char *ar531x_cpuname(void);
uint32_t ar531x_cpu_freq(void);
uint32_t ar531x_bus_freq(void);
void ar531x_device_register(struct device *, void *);
int ar531x_enable_device(const struct ar531x_device *);
const struct ar531x_device *ar531x_get_devices(void);
void ar531x_early_console(void);

/*
 * Board specific things.
 */
struct ar531x_boarddata;
const struct ar531x_boarddata *ar531x_board_info(void);
const void *ar531x_radio_info(void);
struct ar531x_config;
int ar531x_board_config(struct ar531x_config *);

#endif /* _MIPS_ATHEROS_AR531XVAR_H_ */
