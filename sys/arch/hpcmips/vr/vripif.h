/*	$NetBSD: vripif.h,v 1.2.4.3 2002/04/01 07:40:30 nathanw Exp $	*/

/*-
 * Copyright (c) 1999, 2002
 *         Shin Takemura and PocketBSD Project. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef _VRIPIF_H_
#define _VRIPIF_H_

#include <hpcmips/vr/cmuvar.h>
#include <hpcmips/vr/vrdmaauvar.h>
#include <hpcmips/vr/vrdcuvar.h>
#include <dev/hpc/hpciovar.h>

/* Vrip GPIO chip IDs */
enum vrip_iochip {
	VRIP_IOCHIP_VRGIU = 0,
	VRIP_IOCHIP_VRC4172GPIO,
	VRIP_NIOCHIPS
};

/* 
 * Vrip chipset
 */
typedef struct vrip_chipset_tag *vrip_chipset_tag_t;
typedef void *vrip_intr_handle_t;
struct vrip_chipset_tag {
	void *vc_sc;
	vrcmu_chipset_tag_t vc_cc;
	vrdmaau_chipset_tag_t vc_ac;
	vrdcu_chipset_tag_t vc_dc;
	int (*vc_power)(vrip_chipset_tag_t, int, int);
	vrip_intr_handle_t (*vc_intr_establish)(vrip_chipset_tag_t, int, int, 
	    int, int(*)(void*), void*);
	void (*vc_intr_disestablish)(vrip_chipset_tag_t, vrip_intr_handle_t);
	void (*vc_intr_setmask1)(vrip_chipset_tag_t, vrip_intr_handle_t, int);
	void (*vc_intr_setmask2)(vrip_chipset_tag_t, vrip_intr_handle_t,
	    u_int32_t, int);
	void (*vc_intr_getstatus2)(vrip_chipset_tag_t, vrip_intr_handle_t,
	    u_int32_t*);
	void (*vc_register_cmu)(vrip_chipset_tag_t, vrcmu_chipset_tag_t);
	void (*vc_register_gpio)(vrip_chipset_tag_t, hpcio_chip_t);
	void (*vc_register_dmaau)(vrip_chipset_tag_t, vrdmaau_chipset_tag_t);
	void (*vc_register_dcu)(vrip_chipset_tag_t, vrdcu_chipset_tag_t);
};

/* 
 * Vrip driver attach arguments. 
 */
struct vrip_attach_args {
	vrip_chipset_tag_t	va_vc;		/* Chipset tag		*/
	int			va_unit;	/* unit id		*/
	bus_space_tag_t		va_iot;		/* i/o space tag	*/
	bus_space_handle_t	va_parent_ioh;	/* parent i/o space	*/
	bus_addr_t		va_addr;	/* i/o address		*/
	bus_size_t		va_size;
	bus_addr_t		va_addr2;	/* i/o address 2	*/
	bus_size_t		va_size2;
	hpcio_chip_t*		va_gpio_chips;
	vrcmu_chipset_tag_t	va_cc;
	vrdmaau_chipset_tag_t	va_ac;
	vrdcu_chipset_tag_t	va_dc;
#ifdef HPCMIPS_NOT_YET
	bus_dma_tag_t va_dmat;	/* DMA tag */
#endif
};

/* 
 * Vrip methods
 */
#define vrip_power(vc, unit, onoff)					\
		((*(vc)->vc_power)((vc), (unit), (onoff)))
#define vrip_intr_establish(vc, u, l, lv, fn, arg)			\
		((*(vc)->vc_intr_establish)((vc), (u), (l), (lv), (fn), (arg)))
#define vrip_intr_disestablish(vc, handle)				\
		((*(vc)->vc_intr_disestablish)((vc), (handle)))
#define vrip_intr_setmask1(vc, handle, enable)				\
		((*(vc)->vc_intr_setmask1)((vc), (handle), (enable)))
#define vrip_intr_setmask2(vc, handle, mask, onoff)			\
		((*(vc)->vc_intr_setmask2)((vc), (handle), (mask), (onoff)))
#define vrip_intr_getstatus2(vc, handle, status)			\
		((*(vc)->vc_intr_getstatus2)((vc), (handle), (status)))
#define vrip_register_cmu(vc, cmu)					\
		((*(vc)->vc_register_cmu)((vc), (cmu)))
#define vrip_register_gpio(vc, gpio)				\
		((*(vc)->vc_register_gpio)((vc), (gpio)))
#define vrip_register_dmaau(vc, dmaau)					\
		((*(vc)->vc_register_dmaau)((vc), (dmaau)))
#define vrip_register_dcu(vc, dcu)					\
		((*(vc)->vc_register_dcu)((vc), (dcu)))

#endif /* !_VRIPIF_H_ */
