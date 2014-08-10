/*	$NetBSD: ralink_var.h,v 1.5.20.1 2014/08/10 06:54:02 tls Exp $	*/
/*-
 * Copyright (c) 2011 CradlePoint Technology, Inc.
 * All rights reserved.
 *
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
 * THIS SOFTWARE IS PROVIDED BY CRADLEPOINT TECHNOLOGY, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _RALINK_VAR_H_
#define _RALINK_VAR_H_

#include <sys/bus.h>

extern void ralink_com_early(int);

extern void *ra_intr_establish(int, int (*)(void *), void *, int);
extern void ra_intr_disestablish(void *);

extern void ra_bus_init(void);
extern int  ra_spiflash_read(void *, vaddr_t, vsize_t, char *);

extern void ra_gpio_toggle_LED(void *);

extern struct mips_bus_space	ra_bus_memt;
extern struct mips_bus_dma_tag	ra_bus_dmat;
extern const bus_space_handle_t	ra_sysctl_bsh;

struct mainbus_attach_args {
	const char     *ma_name;
	bus_space_tag_t ma_memt;
	bus_dma_tag_t   ma_dmat;
};

#define SERIAL_CONSOLE 1
#define NO_SECURITY    2
extern int ra_check_memo_reg(int);

/* helper defines */
#define MS_TO_HZ(ms) ((ms) * hz / 1000)

#ifdef RALINK_CONSOLE_EARLY
extern void ralink_console_early(void);
#endif

#endif	/* _RALINK_VAR_H_ */
