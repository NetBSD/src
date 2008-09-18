/* $NetBSD: at91spivar.h,v 1.2.4.2 2008/09/18 04:33:20 wrstuden Exp $ */

/*-
 * Copyright (c) 2007 Embedtronics Oy.
 * All rights reserved.
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

#ifndef	_AT91SPIVAR_H_
#define	_AT91SPIVAR_H_

#include <dev/spi/spivar.h>

struct at91spi_machdep {
	int		(*select_slave)(void *self, int);
};

typedef	const struct at91spi_machdep *at91spi_machdep_tag_t;

struct at91spi_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;

	bus_dma_tag_t		sc_dmat;

	int			sc_pid;		/* peripheral identifier */
	struct spi_controller	sc_spi;		/* SPI implementation ops */
	at91spi_machdep_tag_t	sc_md;		/* board-specific support */
	struct at91spi_job	*sc_job;	/* current job */
	struct spi_chunk	*sc_wchunk;
	struct spi_chunk	*sc_rchunk;
	void			*sc_ih;		/* interrupt handler (what?) */

	void			*sc_dmapage;
	bus_addr_t		sc_dmaaddr;
	bus_dmamap_t		sc_dmamap;
	int			sc_dmaoffs;	/* current dma offset	*/

	struct spi_transfer	*sc_transfer;
	bool			sc_running;	/* is it processing stuff? */

	SIMPLEQ_HEAD(,spi_transfer)	sc_q;
};

void at91spi_attach_common(device_t parent, device_t self, void *aux,
			    at91spi_machdep_tag_t md);

#endif	/* _AT91SPIVAR_H_ */
