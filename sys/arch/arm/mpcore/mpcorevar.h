/*
 * Copyright (c) 2010, 2011 Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
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
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARM_MPCORE_MPCOREVAR_H
#define _ARM_MPCORE_MPCOREVAR_H


#include "bus_space_a2x.h"
#include "bus_space_a4x.h"


extern struct bus_space mpcore_bs_tag;
extern struct bus_space mpcore_a2x_bs_tag;
extern struct arm32_bus_dma_tag mpcore_bus_dma_tag;

struct axi_attach_args {
	const char	*aa_name;
	bus_space_tag_t	aa_iot;
	bus_space_tag_t	aa_iot_a2x;
	bus_space_tag_t	aa_iot_a4x;
	bus_dma_tag_t	aa_dmat;
	bus_addr_t	aa_addr;
	bus_size_t	aa_size;
	int		aa_irq;
	int		aa_irqbase;
};


struct pmr_attach_args {
	const char *pa_name;
	bus_space_tag_t pa_iot;
	bus_space_handle_t pa_ioh;
	int pa_irq;
};


#endif	/* _ARM_MPCORE_MPCOREVAR_H */
