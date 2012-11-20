/*	$OpenBSD: autoconf.h,v 1.8 2010/08/31 10:24:46 pirofti Exp $ */

/*
 * Copyright (c) 2001-2003 Opsycon AB  (www.opsycon.se / www.opsycon.com)
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*
 * Definitions used by autoconfiguration.
 */

#ifndef _MACHINE_AUTOCONF_H_
#define _MACHINE_AUTOCONF_H_

#include <sys/bus.h>
#include <sys/kcore.h>
#include <mips/bonito/bonitovar.h>

struct bonito_config;
struct mips_isa_chipset;
struct bonito_irqmap;

/*
 * List of legacy I/O ranges.
 */
struct legacy_io_range {
	bus_addr_t	start;
	bus_size_t	end;	/* inclusive */
};

/*
 * Per platform information.
 */

struct platform {
	const int			 system_type;
	const char			*vendor;
	const char			*product;

	const struct bonito_config	*bonito_config;
	struct mips_isa_chipset		*isa_chipset;
	const struct legacy_io_range	*legacy_io_ranges;
	int				bonito_mips_intr;
	int				isa_mips_intr;
	void				(*isa_intr)(int, vaddr_t, uint32_t);
	int				(*p_pci_intr_map)(int, int, int, pci_intr_handle_t *);
	const struct bonito_irqmap	*irq_map;

	void				(*setup)(void);
	void				(*device_register)(device_t , void *);

	void				(*powerdown)(void);
	void				(*reset)(void);
	int				(*suspend)(void);
	int				(*resume)(void);
};

#define LOONGSON_CLASS		0x0060	/* Loongson + PMON2000 class */	
#define LOONGSON_2E		0x0060	/* Generic Loongson 2E system */	
#define LOONGSON_YEELOONG	0x0061	/* Lemote Yeeloong */
#define LOONGSON_GDIUM		0x0062	/* EMTEC Gdium Liberty */
#define LOONGSON_FULOONG	0x0063	/* Lemote Fuloong */ 
#define LOONGSON_LYNLOONG	0x0064	/* Lemote Lynloong */ 

extern const struct platform *sys_platform;
extern uint loongson_ver;

struct mainbus_attach_args {
	const char	*maa_name;
};

extern char bootdev[];
extern enum devclass bootdev_class;

void	loongson2e_setup(paddr_t, paddr_t, vaddr_t, vaddr_t, bus_dma_tag_t);
void	loongson2f_setup(paddr_t, paddr_t, vaddr_t, vaddr_t, bus_dma_tag_t);

extern bus_space_tag_t comconsiot;
extern bus_addr_t comconsaddr;
extern int comconsrate;

extern phys_ram_seg_t mem_clusters[];
extern int mem_cluster_cnt;

int gdium_cnattach(bus_space_tag_t, bus_space_tag_t,
    pci_chipset_tag_t, pcitag_t, pcireg_t);

#endif /* _MACHINE_AUTOCONF_H_ */
