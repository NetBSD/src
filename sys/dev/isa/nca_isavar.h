/*	$NetBSD: nca_isavar.h,v 1.2 2000/03/18 13:17:04 mycroft Exp $	*/

/*-
 * Copyright (c)  1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by John M. Ruschmeyer.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <machine/bus.h>

struct nca_isa_softc {
	struct ncr5380_softc	sc_ncr5380;	/* glue to MI code */

        int             sc_active;              /* Pseudo-DMA state vars */
        int             sc_tc;
        int             sc_datain;
        size_t          sc_dmasize;
        size_t          sc_dmatrans; 
        char            **sc_dmaaddr; 
        size_t          *sc_pdmalen;

        void *sc_ih;
        int sc_irq;
        int sc_drq;
	int sc_options;

#ifdef NCA_DEBUG
        int sc_debug;
#endif
};

struct nca_isa_probe_data {
	struct device sc_dev;
	int sc_reg_offset;
	int sc_host_type;
	int sc_irq;
	int sc_isncr;
	int sc_rev;
	int sc_isfast;
	int sc_msize;
	int sc_parity;
	int sc_sync;
	int sc_id;
};
