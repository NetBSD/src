/*	$NetBSD: vripvar.h,v 1.4.4.1 2001/10/01 12:39:26 fvdl Exp $	*/

/*-
 * Copyright (c) 1999
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
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

typedef void *vrip_chipset_tag_t;

#include <hpcmips/vr/cmuvar.h>
#include <dev/hpc/hpciovar.h>

/*
 * Vrip bus attach arguments
 */
struct vripbus_attach_args {
	char *vba_busname;
	bus_space_tag_t vba_iot;	/* vrip i/o space tag */
};

/* Vrip GPIO chips */
enum vrip_iochip {
	VRIP_IOCHIP_VRGIU = 0,
	VRIP_IOCHIP_VRC4172GPIO,
	VRIP_NIOCHIPS
};

struct vrip_softc {
	struct	device sc_dv;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	hpcio_chip_t sc_gpio_chips[VRIP_NIOCHIPS];
	vrcmu_chipset_tag_t sc_cc;
	vrcmu_function_tag_t sc_cf;
	int sc_pri; /* attaching device priority */
	u_int32_t sc_intrmask;
};

/* 
 * Vrip driver attach arguments. 
 */
struct vrip_attach_args {
	vrip_chipset_tag_t va_vc; /* Chipset tag */
	bus_space_tag_t va_iot;	/* i/o space tag */
	bus_addr_t	va_addr;	/* i/o address	*/
	bus_size_t	va_size;
	int		va_intr;	/* level 1 interrupt line */
	bus_addr_t	va_addr2;	/* i/o address 2	*/
	bus_size_t	va_size2;
	hpcio_chip_t*	va_gpio_chips;
	vrcmu_chipset_tag_t va_cc;
	vrcmu_function_tag_t va_cf;
#ifdef HPCMIPS_NOT_YET
	bus_dma_tag_t va_dmat;	/* DMA tag */
#endif
};

/*
 * Interrupt establishment/disestablishment functions
 */
void *vrip_intr_establish(vrip_chipset_tag_t, int, int, int(*)(void*), void*);
void vrip_intr_disestablish(vrip_chipset_tag_t, void*);
void vrip_intr_setmask1(vrip_chipset_tag_t, void*, int);
void vrip_intr_setmask2(vrip_chipset_tag_t, void*, u_int32_t, int);
void vrip_intr_get_status2(vrip_chipset_tag_t, void*, u_int32_t*);
void vrip_intr_suspend(void);
void vrip_intr_resume(void);
/*
 * CMU/GPIO interface.
 */
void vrip_cmu_function_register(vrip_chipset_tag_t, vrcmu_function_tag_t,
    vrcmu_chipset_tag_t);
void vrip_gpio_register(vrip_chipset_tag_t, hpcio_chip_t);
    
/*
 * Debuggin utility
 */    
void bitdisp16(u_int16_t);
void bitdisp32(u_int32_t);
void bitdisp64(u_int32_t[2]);
