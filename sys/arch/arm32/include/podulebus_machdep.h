/* $NetBSD: podulebus_machdep.h,v 1.1 2001/03/20 22:59:41 bjh21 Exp $ */

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
#include <machine/io.h>

/* Define the structures used to describe the "known" podules */

struct podule_description {
	int product_id;
	char *description;
};

struct podule_list {
	int manufacturer_id;
	char *description;
	struct podule_description *products;
};

/* Define the structure used to describe a podule */

#define PODULE_DESCRIPTION_LENGTH	63

typedef struct {
	/* The podule header, read from the on board ROM */

	u_char flags0;
	u_char flags1;
	u_char reserved;
	u_short product;
	u_short manufacturer;
	u_char country;
	u_int irq_addr;
	u_int irq_mask;
	u_int fiq_addr;
	u_int fiq_mask;

	/* The base addresses for this podule */

	u_int fast_base;
	u_int medium_base;
	u_int slow_base;
	u_int sync_base;
	u_int mod_base;
	u_int easi_base;

	/* Flags */

	int podulenum; 
	int slottype;
	int attached;

	/* Other info */

	char description[PODULE_DESCRIPTION_LENGTH + 1];

	/* podule specific information provided by podulebus */

	int interrupt;

	int dma_channel;
	int dma_interrupt;
} podule_t;

#define PODULE_FLAGS_CD	0x01
#define PODULE_FLAGS_IS	0x02

#define SLOT_NONE 0x00
#define SLOT_POD  0x01
#define SLOT_NET  0x02

typedef	int	podulebus_intr_handle_t;

#define podulebus_attach_args podule_attach_args

struct podule_attach_args {
	podule_t *pa_podule;		/* podule descriptor */
	int pa_podule_number;		/* podule number */
	int pa_slottype;		/* podule slot type */
	bus_space_tag_t pa_iot;		/* bus space tag */

#define pa_easi_t	pa_iot
#define pa_mod_t	pa_iot
#define pa_fast_t	pa_iot
#define pa_medium_t	pa_iot
#define pa_slow_t	pa_iot
#define pa_sync_t	pa_iot

#define pa_easi_base	pa_podule->easi_base
#define pa_mod_base	pa_podule->mod_base
#define pa_fast_base	pa_podule->fast_base
#define pa_medium_base	pa_podule->medium_base
#define pa_slow_base	pa_podule->slow_base
#define pa_sync_base	pa_podule->sync_base

	podulebus_intr_handle_t pa_ih;	/* interrupt handle */

#define pa_manufacturer	pa_podule->manufacturer
#define pa_product	pa_podule->product
#define pa_descr	pa_podule->description
};

/* Useful macros */

/* EASI space cycle control */


#define IS_PODULE(pa, man, prod)	\
	(pa->pa_manufacturer == man && pa->pa_product == prod)



#define EASI_CYCLE_TYPE_A	0x00
#define EASI_CYCLE_TYPE_C	0x01
#define set_easi_cycle_type(podule, type) \
	IOMD_WRITE_BYTE(IOMD_ECTCR, (IOMD_READ_BYTE(IOMD_ECTCR) & ~(1 << podule)) | (1 << type))
  
#ifdef _KERNEL

/* Array of podule structures, one per possible podule */

extern podule_t podules[MAX_PODULES + MAX_NETSLOTS];

u_int poduleread __P((u_int address, int offset, int slottype));

int matchpodule __P((struct podule_attach_args *pa,
    int manufacturer, int product, int required_slot));

void netslot_ea	__P((u_int8_t *buffer));

extern void *podulebus_irq_establish __P((podulebus_intr_handle_t, int,
    int (*)(void *), void *, struct evcnt *));

#endif

/* End of podulebus.h */
