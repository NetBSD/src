/*	$NetBSD: gfttyvar.h,v 1.1 2024/01/02 07:29:39 thorpej Exp $	*/

/*-     
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
 * All rights reserved.
 *              
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */     

#ifndef _DEV_GOLDFISH_GFTTYVAR_H_
#define	_DEV_GOLDFISH_GFTTYVAR_H_

struct gftty_config {
	bus_space_tag_t		c_bst;
	bus_space_handle_t	c_bsh;
	uint32_t		c_version;
};

struct gftty_softc {
	device_t		sc_dev;
	struct gftty_config	*sc_config;
};

void	gftty_attach(struct gftty_softc *sc);
bool	gftty_is_console(struct gftty_softc *sc);
void	gftty_alloc_config(struct gftty_softc *sc, bus_space_tag_t,
	    bus_space_handle_t);
void	gftty_cnattach(bus_space_tag_t, bus_space_handle_t);

#endif /* _DEV_GOLDFISH_GFTTYVAR_H_ */
