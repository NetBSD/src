/*	$NetBSD: superhywayvar.h,v 1.1 2002/07/05 13:31:55 scw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SH5_SUPERHYWAYVAR_H
#define _SH5_SUPERHYWAYVAR_H

struct superhyway_attach_args {
	const char	*sa_name;
	bus_space_tag_t	sa_bust;
	bus_dma_tag_t	sa_dmat;
	u_int		sa_pport;
};

/* XXX: This will likely change when/if superhyway addresses ever go 64-bit */
#define	SUPERHYWAY_PPORT_TO_BUSADDR(p)	((bus_addr_t)((p) << 24))

#define	SUPERHYWAY_REG_VCR		0x00
#define	SUPERHYWAY_REG_SZ		0x08
#define	SUPERHYWAY_VCR_PERR_FLAGS_MASK	0xff
#define	SUPERHYWAY_VCR_PERR_FLAGS_SHIFT	0
#define	SUPERHYWAY_VCR_MERR_FLAGS_MASK	0xff
#define	SUPERHYWAY_VCR_MERR_FLAGS_SHIFT	8
#define	SUPERHYWAY_VCR_MOD_VERS_MASK	0xffff
#define	SUPERHYWAY_VCR_MOD_VERS_SHIFT	16
#define	SUPERHYWAY_VCR_MOD_ID_MASK	0xffff
#define	SUPERHYWAY_VCR_MOD_ID_SHIFT	32
#define	SUPERHYWAY_VCR_BOT_MB_MASK	0xff
#define	SUPERHYWAY_VCR_BOT_MB_SHIFT	48
#define	SUPERHYWAY_VCR_TOP_MB_MASK	0xff
#define	SUPERHYWAY_VCR_TOP_MB_SHIFT	56

#define	SUPERHYWAY_VCR_PERR_FLAGS(v)	\
	(((v) >> SUPERHYWAY_VCR_PERR_FLAGS_SHIFT) & \
	    SUPERHYWAY_VCR_PERR_FLAGS_MASK)
#define	SUPERHYWAY_VCR_MERR_FLAGS(v)	\
	(((v) >> SUPERHYWAY_VCR_MERR_FLAGS_SHIFT) & \
	    SUPERHYWAY_VCR_MERR_FLAGS_MASK)
#define	SUPERHYWAY_VCR_MOD_VERS(v)	\
	(((v) >> SUPERHYWAY_VCR_MOD_VERS_SHIFT) & SUPERHYWAY_VCR_MOD_VERS_MASK)
#define	SUPERHYWAY_VCR_MOD_ID(v)	\
	(((v) >> SUPERHYWAY_VCR_MOD_ID_SHIFT) & SUPERHYWAY_VCR_MOD_ID_MASK)
#define	SUPERHYWAY_VCR_BOT_MB(v)	\
	(((v) >> SUPERHYWAY_VCR_BOT_MB_SHIFT) & SUPERHYWAY_VCR_BOT_MB_MASK)
#define	SUPERHYWAY_VCR_TOP_MB(v)	\
	(((v) >> SUPERHYWAY_VCR_TOP_MB_SHIFT) & SUPERHYWAY_VCR_TOP_MB_MASK)

#endif /* _SH5_SUPERHYWAYVAR_H */
