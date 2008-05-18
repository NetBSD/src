/*	$NetBSD: diovar.h,v 1.12.76.1 2008/05/18 12:31:55 yamt Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998 The NetBSD Foundation, Inc.
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

/*
 * Autoconfiguration definitions and prototypes for the DIO bus.
 */

#include <machine/bus.h>

/*
 * Arguments used to attach a device to the DIO bus.
 */
struct dio_attach_args {
	bus_space_tag_t da_bst;		/* bus space tag */
	int	da_scode;		/* select code */
	int	da_addr;		/* device address */
	int	da_size;		/* size of address space */
	int	da_ipl;			/* interrupt priority level */
	uint8_t da_id;			/* primary device id */
	uint8_t da_secid;		/* secondary device id */
};

/*
 * This structure is used by the autoconfiguration code to lookup
 * the size of a DIO device (not all use one select code).
 */
struct dio_devdata {
	uint8_t dd_id;			/* primary device id */
	uint8_t dd_secid;		/* secondary device id */
	int	dd_nscode;		/* number of select codes */
};

/*
 * This structure is used by the autoconfiguration code to print
 * a textual description of a device.
 */
struct dio_devdesc {
	uint8_t dd_id;			/* primary device id */
	uint8_t dd_secid;		/* secondary device id */
	const char *dd_desc;		/* description */
};

#ifdef _KERNEL
void	*dio_scodetopa(int);
void	*dio_intr_establish(int (*)(void *), void *, int, int);
void	dio_intr_disestablish(void *);
void	dio_set_bus_space_oddbyte(bus_space_tag_t);
#endif /* _KERNEL */
