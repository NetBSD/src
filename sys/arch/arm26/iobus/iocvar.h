/* $NetBSD: iocvar.h,v 1.1 2000/05/09 21:56:01 bjh21 Exp $ */
/*-
 * Copyright (c) 1998, 1999 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/* This file is part of NetBSD/arm26 -- a port of NetBSD to ARM2/3 machines. */
/*
 * iocvar.h - aspects of the IOC driver that are visible to the world.
 */

#ifndef _ARM26_IOCVAR_H
#define _ARM26_IOCVAR_H

#include <machine/irq.h>

/* Structure passed to children of an IOC */

struct ioc_attach_args {
	/* Means of accessing the device at various speeds */
	bus_space_tag_t		ioc_fast_t;
	bus_space_handle_t	ioc_fast_h;
	bus_space_tag_t		ioc_medium_t;
	bus_space_handle_t	ioc_medium_h;
	bus_space_tag_t		ioc_slow_t;
	bus_space_handle_t	ioc_slow_h;
	bus_space_tag_t		ioc_sync_t;
	bus_space_handle_t	ioc_sync_h;
	int			ioc_bank; /* only for ioc_print */
	int			ioc_offset;
};

/* Public IOC functions */

extern u_int ioc_ctl_read __P((struct device *self));
extern void ioc_ctl_write __P((struct device *self, u_int value, u_int mask));

extern struct irq_handler *ioc_irq_establish __P((struct device *self, int irq, int level, int handler __P((void *)), void *cookie));
extern int ioc_irq_status __P((struct device *self, int irq));
extern void ioc_irq_waitfor __P((struct device *self, int irq));
extern void ioc_irq_clear __P((struct device *self, int mask));
extern u_int32_t ioc_irq_status_full __P((struct device *self));
extern void ioc_irq_setmask __P((struct device *self, u_int32_t mask));

extern void ioc_counter_start __P((struct device *self, int counter, int value));

extern void ioc_initclocks __P((struct device *self));
extern void ioc_setstatclockrate __P((struct device *self, int hzrate));
extern void ioc_microtime __P((struct device *self, struct timeval *tv));

#endif
