/* $NetBSD: platform.h,v 1.2 2011/07/10 06:26:02 matt Exp $ */
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

#ifndef	_MIPS_ATHEROS_PLATFORM_H_
#define	_MIPS_ATHEROS_PLATFORM_H_

#include <sys/param.h>
#include <sys/bus.h>

struct atheros_device {
	const char	*adv_name;
	bus_addr_t	adv_addr;
	bus_size_t	adv_size;
	u_int		adv_cirq;
	u_int		adv_mirq;
	uint32_t	adv_mask;
	uint32_t	adv_reset;
	uint32_t	adv_enable;
};

/*
 * Board specific things.
 */
struct atheros_boarddata;
struct atheros_config;

struct atheros_intrsw {
	void (*aisw_init)(void);
	void *(*aisw_cpu_establish)(int, int (*)(void *), void *);
	void (*aisw_cpu_disestablish)(void *);
	void *(*aisw_misc_establish)(int, int (*)(void *), void *);
	void (*aisw_misc_disestablish)(void *);
	void (*aisw_cpuintr)(int, vaddr_t, uint32_t);
	void (*aisw_iointr)(int, vaddr_t, uint32_t);
};

struct arfreqs {
	uint32_t freq_bus;
	uint32_t freq_cpu;
	uint32_t freq_mem;
	uint32_t freq_pll;
	uint32_t freq_ref;
	uint32_t freq_uart;
};

struct atheros_platformsw {
	const struct atheros_intrsw *apsw_intrsw;

	void (*apsw_intr_init)(void);
	const char * const * apsw_cpu_intrnames;
	const char * const * apsw_misc_intrnames;
	size_t apsw_cpu_nintrs;
	size_t apsw_misc_nintrs;
	u_int apsw_cpuirq_misc;

	bus_addr_t apsw_misc_intmask;
	bus_addr_t apsw_misc_intstat;

	const struct ipl_sr_map *apsw_ipl_sr_map;

	/*
	 * CPU specific routines.
	 */
	size_t (*apsw_get_memsize)(void);
	void (*apsw_wdog_reload)(uint32_t);
	void (*apsw_bus_init)(void);
	void (*apsw_get_freqs)(struct arfreqs *);
	void (*apsw_device_register)(device_t, void *);
	int (*apsw_enable_device)(const struct atheros_device *);
	void (*apsw_reset)(void);
	const struct atheros_device *apsw_devices;

	/*
	 * Early console support.
	 */
	bus_addr_t apsw_uart0_base;
	bus_addr_t apsw_revision_id_addr;
};

/*
 * Board specific data.
 */
struct ar531x_config;
struct ar531x_boarddata;
struct atheros_boardsw {
	const struct ar531x_boarddata *(*absw_get_board_info)(void);
	const void *(*absw_get_radio_info)(void);
};

#ifdef _KERNEL
void	atheros_consinit(void);
void	atheros_early_consinit(void);

void	atheros_set_platformsw(void);
const char *
	atheros_get_cpuname(void);
u_int	atheros_get_chipid(void);

uint32_t atheros_get_uart_freq(void);
uint32_t atheros_get_bus_freq(void);
uint32_t atheros_get_cpu_freq(void);
uint32_t atheros_get_mem_freq(void);

const struct ar531x_boarddata *
	atheros_get_board_info(void);
int 	atheros_get_board_config(struct ar531x_config *); 

extern const struct atheros_boardsw ar5312_boardsw;
extern const struct atheros_boardsw ar5315_boardsw;

extern const struct atheros_platformsw ar5312_platformsw;
extern const struct atheros_platformsw ar5315_platformsw;
extern const struct atheros_platformsw ar7100_platformsw;
extern const struct atheros_platformsw ar9344_platformsw;
extern const struct atheros_platformsw *platformsw;

extern const struct atheros_intrsw atheros_intrsw;

static inline uint32_t
atheros_get_memsize(void)
{
	return (*platformsw->apsw_get_memsize)();
}

static inline void
atheros_wdog_reload(uint32_t period)
{
	(*platformsw->apsw_wdog_reload)(period);
}

static inline void
atheros_bus_init(void)
{
	return (*platformsw->apsw_bus_init)();
}

static inline void
atheros_intr_init(void)
{
	(*platformsw->apsw_intrsw->aisw_init)();
}

static inline void *
atheros_cpu_intr_establish(int irq, int (*func)(void *), void *arg)
{
	return (*platformsw->apsw_intrsw->aisw_cpu_establish)(irq, func, arg);
}

static inline void
atheros_cpu_intr_disestablish(void *cookie)
{
	(*platformsw->apsw_intrsw->aisw_cpu_disestablish)(cookie);
}

static inline void *
atheros_misc_intr_establish(int irq, int (*func)(void *), void *arg)
{
	return (*platformsw->apsw_intrsw->aisw_misc_establish)(irq, func, arg);
}

static inline void
atheros_misc_intr_disestablish(void *cookie)
{
	(*platformsw->apsw_intrsw->aisw_misc_disestablish)(cookie);
}

static inline int
atheros_enable_device(const struct atheros_device *adv)
{
	return (*platformsw->apsw_enable_device)(adv);
}

static inline void
atheros_reset(void)
{
	return (*platformsw->apsw_reset)();
}

#endif /* _KERNEL */

#endif /* _MIPS_ATHEROS_PLATFORM_H_ */
