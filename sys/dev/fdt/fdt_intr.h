/*	$NetBSD: fdt_intr.h,v 1.1 2025/09/06 20:11:30 thorpej Exp $	*/

/*-
 * Copyright (c) 2015-2018 Jared McNeill <jmcneill@invisible.ca>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _DEV_FDT_FDT_INTR_H_
#define	_DEV_FDT_FDT_INTR_H_

#include <sys/device.h>

/* flags for fdtbus_intr_establish */
#define	FDT_INTR_MPSAFE			__BIT(0)

/* Interrupt trigger types defined by the FDT "interrupts" bindings. */
#define	FDT_INTR_TYPE_POS_EDGE		__BIT(0)
#define	FDT_INTR_TYPE_NEG_EDGE		__BIT(1)
#define	FDT_INTR_TYPE_DOUBLE_EDGE	(FDT_INTR_TYPE_POS_EDGE | \
					 FDT_INTR_TYPE_NEG_EDGE)
#define	FDT_INTR_TYPE_HIGH_LEVEL	__BIT(2)
#define	FDT_INTR_TYPE_LOW_LEVEL		__BIT(3)

struct fdtbus_interrupt_controller_func {
	void *	(*establish)(device_t, u_int *, int, int,
			     int (*)(void *), void *, const char *);
	void	(*disestablish)(device_t, void *);
	bool	(*intrstr)(device_t, u_int *, char *, size_t);
	void	(*mask)(device_t, void *);
	void	(*unmask)(device_t, void *);
};

int		fdtbus_register_interrupt_controller(device_t, int,
		    const struct fdtbus_interrupt_controller_func *);

void *		fdtbus_intr_establish(int, u_int, int, int,
		    int (*func)(void *), void *arg);
void *		fdtbus_intr_establish_xname(int, u_int, int, int,
		    int (*func)(void *), void *arg, const char *);
void *		fdtbus_intr_establish_byname(int, const char *, int, int,
		    int (*func)(void *), void *arg, const char *);
void *		fdtbus_intr_establish_raw(int, const u_int *, int, int,
		    int (*func)(void *), void *arg, const char *);
void		fdtbus_intr_mask(int, void *);
void		fdtbus_intr_unmask(int, void *);
void		fdtbus_intr_disestablish(int, void *);
bool		fdtbus_intr_str(int, u_int, char *, size_t);
bool		fdtbus_intr_str_raw(int, const u_int *, char *, size_t);
int		fdtbus_intr_parent(int);

#endif /* _DEV_FDT_FDT_INTR_H_ */
