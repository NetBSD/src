/*	$Id: at91busvar.h,v 1.4.2.2 2013/01/16 05:32:45 yamt Exp $	*/
/*	$NetBSD: at91busvar.h,v 1.4.2.2 2013/01/16 05:32:45 yamt Exp $ */

/*
 * Copyright (c) 2007 Embedtronics Oy
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
 * THIS SOFTWARE IS PROVIDED BY ICHIRO FUKUHARA ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ICHIRO FUKUHARA OR THE VOICES IN HIS HEAD BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _AT91BUSVAR_H_
#define _AT91BUSVAR_H_

#include <sys/conf.h>
#include <sys/device.h>
#include <sys/queue.h>

#include <sys/bus.h>
#include <arm/at91/at91piovar.h>


/* clocks: */
struct at91bus_clocks {
	uint32_t		slow;		/* slow clock in Hz	*/
	uint32_t		main;		/* main clock in Hz	*/
	uint32_t		cpu;		/* processor clock in Hz */
	uint32_t		master;		/* master clock in Hz	*/
	uint32_t		plla;		/* PLLA clock */
	uint32_t		pllb;		/* PLLB clock */
};

extern struct at91bus_clocks at91bus_clocks;

#define	AT91_SCLK	at91bus_clocks.slow
#define	AT91_MCLK	at91bus_clocks.main
#define	AT91_PCLK	at91bus_clocks.cpu
#define	AT91_MSTCLK	at91bus_clocks.master
#define	AT91_PLLACLK	at91bus_clocks.plla
#define	AT91_PLLBCLK	at91bus_clocks.pllb


/* at91bus attach arguments: */
struct at91bus_attach_args {
	bus_space_tag_t		sa_iot;		/* bus tag		*/
	bus_dma_tag_t		sa_dmat;	/* DMA tag		*/
	bus_addr_t		sa_addr;	/* I/O base address	*/
	bus_size_t		sa_size;	/* I/O space size	*/
	int			sa_pid;		/* peripheral ID	*/
};


struct at91bus_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_dma_tag_t		sc_dmat;
};

struct trapframe;

struct at91bus_machdep {
	/* initialization: */
	void (*init)(struct at91bus_clocks *);
	void (*attach_cn)(bus_space_tag_t, int speed, int flags);
	const struct pmap_devmap *(*devmap)(void);

	/* clocking support: */
	void (*peripheral_clock)(int pid, int enable);

	/* PIO support: */
	at91pio_port (*pio_port)(int pid);
	uint32_t (*gpio_mask)(int pid);

	/* interrupt handling support: */
	void (*intr_init)(void);
	void *(*intr_establish)(int pid, int ipl, int type, int (*ih_func)(void *), void *arg);
	void (*intr_disestablish)(void *cookie);
	void (*intr_poll)(void *cookie, int flags);
	void (*intr_dispatch)(struct trapframe *);

	/* configuration */
	const char *(*peripheral_name)(int pid);
	void (*search_peripherals)(device_t self,
				   device_t (*found_func)(device_t, bus_addr_t, int pid));
};
typedef const struct at91bus_machdep * at91bus_tag_t;

#ifdef	AT91RM9200
extern const struct at91bus_machdep at91rm9200bus;
#endif

extern uint32_t at91_chip_id;
#define	AT91_CHIP_ID()	at91_chip_id
extern at91bus_tag_t at91bus_tag;
extern struct bus_space at91_bs_tag;
extern struct arm32_bus_dma_tag at91_bd_tag;


extern int at91bus_init(void);
struct _BootConfig;
extern u_int at91bus_setup(struct _BootConfig *);
extern bus_dma_tag_t at91_bus_dma_init(struct arm32_bus_dma_tag *);

static __inline const struct pmap_devmap *
at91_devmap(void)
{
	return (*at91bus_tag->devmap)();
}

static __inline void
at91_peripheral_clock(int pid, int enable)
{
	return (*at91bus_tag->peripheral_clock)(pid, enable);
}

static __inline const char *
at91_peripheral_name(int pid)
{
	return (*at91bus_tag->peripheral_name)(pid);
}

static __inline at91pio_port
at91_pio_port(int pid)
{
	return (*at91bus_tag->pio_port)(pid);
}

static __inline uint32_t
at91_gpio_mask(int pid)
{
	return (*at91bus_tag->gpio_mask)(pid);
}

static __inline void 
at91_intr_init(void)
{
	return (*at91bus_tag->intr_init)();
}

static __inline void *
at91_intr_establish(int pid, int ipl, int type,
		     int (*ih_func)(void *), void *ih_arg)
{
	return (*at91bus_tag->intr_establish)(pid, ipl, type, ih_func, ih_arg);
}

static __inline void
at91_intr_disestablish(void *cookie)
{
	return (*at91bus_tag->intr_disestablish)(cookie);
}

static __inline void
at91_intr_poll(void *cookie, int flags)
{
	return (*at91bus_tag->intr_poll)(cookie, flags);
}

static __inline void
at91_search_peripherals(device_t self,
			device_t (*found_func)(device_t, bus_addr_t, int pid))
{
	return (*at91bus_tag->search_peripherals)(self, found_func);
}


#endif /* _AT91BUSVAR_H_ */
