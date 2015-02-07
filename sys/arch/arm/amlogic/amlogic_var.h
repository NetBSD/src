/* $NetBSD: amlogic_var.h,v 1.1 2015/02/07 17:20:17 jmcneill Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _ARM_AMLOGIC_VAR_H
#define _ARM_AMLOGIC_VAR_H

#include <sys/types.h>
#include <sys/bus.h>

struct amlogic_locators {
	const char *loc_name;
	bus_addr_t loc_offset;
	bus_size_t loc_size;
	int loc_port;
	int loc_intr;
#define AMLOGICIO_INTR_DEFAULT	0
};

struct amlogicio_attach_args {
	struct amlogic_locators aio_loc;
	bus_space_tag_t aio_core_bst;
	bus_space_tag_t aio_core_a4x_bst;
	bus_space_handle_t aio_bsh;
	bus_dma_tag_t aio_dmat;
};

extern struct bus_space amlogic_bs_tag;
extern struct bus_space amlogic_a4x_bs_tag;
extern bus_space_handle_t amlogic_core_bsh;
extern struct arm32_bus_dma_tag amlogic_dma_tag;

void	amlogic_bootstrap(void);

#endif /* _ARM_AMLOGIC_VAR_H */
