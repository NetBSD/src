/* $NetBSD: podulebus_machdep.h,v 1.3.2.2 2001/03/27 15:30:23 bouyer Exp $ */

/*
 * Copyright (c) 1995 Mark Brinicombe.
 * Copyright (c) 1995 Brini.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * podulebus.h
 *
 * Podule bus header file
 *
 * Created      : 26/04/95
 */

#include <sys/param.h>
#include <machine/bus.h>

struct podulebus_chunk {
	int	pc_type;
	int	pc_length;
	int	pc_offset;
	int	pc_useloader;
};

typedef int podulebus_intr_handle_t;

struct podulebus_attach_args {
	/* bus_space references for the (many) podule spaces */
	bus_space_tag_t		pa_easi_t;
	bus_space_handle_t	pa_easi_h;
	bus_space_tag_t		pa_mod_t;
	bus_space_handle_t	pa_mod_h;
	bus_space_tag_t		pa_fast_t;
	bus_space_handle_t	pa_fast_h;
	bus_space_tag_t		pa_medium_t;
	bus_space_handle_t	pa_medium_h;
	bus_space_tag_t		pa_slow_t;
	bus_space_handle_t	pa_slow_h;
	bus_space_tag_t		pa_sync_t;
	bus_space_handle_t	pa_sync_h;

	/* Same things as raw addresses (arm32 compat) */
	bus_addr_t	pa_easi_base;
	bus_addr_t	pa_mod_base;
	bus_addr_t	pa_fast_base;
	bus_addr_t	pa_medium_base;
	bus_addr_t	pa_slow_base;
	bus_addr_t	pa_sync_base;

	podulebus_intr_handle_t	pa_ih;

	int	pa_slotnum;
	int	pa_ecid;
	int	pa_flags1;
	int	pa_manufacturer;
	int	pa_product;
	struct podulebus_chunk *pa_chunks;
	int	pa_nchunks;
	char	*pa_descr;
	void	*pa_loader;
	int	pa_slotflags;
#define PA_SLOT_EASI	0x01	/* EASI space available */
};

#define IS_PODULE(pa, man, prod)	\
	((pa)->pa_manufacturer == (man) && (pa)->pa_product == (prod))

/* Set the address-bus shift for a bus_space tag. */

#define podulebus_shift_tag(tag, shift, tagp)	(*(tagp) = (shift))

#ifdef _KERNEL

struct evcnt;

extern void *podulebus_irq_establish(podulebus_intr_handle_t, int,
    int (*)(void *), void *, struct evcnt *);
extern int podulebus_initloader(struct podulebus_attach_args *);
extern int podloader_readbyte(struct podulebus_attach_args *, u_int);
extern void podloader_writebyte(struct podulebus_attach_args *, u_int, int);
void podloader_reset(struct podulebus_attach_args *);
int podloader_callloader(struct podulebus_attach_args *, u_int, u_int);

#endif

/* arm32 compatibility */

#define WriteByte(a, v)		bus_space_write_1(2, (a), 0, (v))
#define WriteShort(a, v)	bus_space_write_2(2, (a), 0, (v))
#define ReadByte(a)		bus_space_read_1(2, (a), 0)
#define ReadShort(a)		bus_space_read_2(2, (a), 0)

/* XXX The arm32 version ignores "slot" too. */
#define matchpodule(pa, man, prod, slot) IS_PODULE(pa, man, prod)

/* end arm32 compatibility */

/* End of podulebus.h */
