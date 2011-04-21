/*	$NetBSD: rmixl_iobusvar.h,v 1.1.2.2 2011/04/21 01:41:13 rmind Exp $	*/
/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Cliff Neighbors
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

#ifndef _RMIXL_IOBUSVAR_H_
#define _RMIXL_IOBUSVAR_H_

struct rmixl_iobus_attach_args {
	bus_space_tag_t		ia_obio_bst;	/* for iobus controler access */
	bus_space_handle_t	ia_obio_bsh;	/*  "   "     "         "     */
	bus_space_tag_t		ia_iobus_bst;	/* for iobus access */
	bus_addr_t		ia_iobus_addr;	/* device iobus address */
	bus_size_t		ia_iobus_size;	/* device iobus size */
	uint32_t		ia_iobus_intr;	/* TBD */
	uint32_t		ia_dev_parm;	/* copy of chip select device parameters */
	int			ia_cs;		/* chip select index */
};

#endif	/* _RMIXL_IOBUSVAR_H_ */
