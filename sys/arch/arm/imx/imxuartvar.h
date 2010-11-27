/* $NetBSD: imxuartvar.h,v 1.4 2010/11/27 13:37:27 bsh Exp $ */
/*
 * driver include for Freescale i.MX31 and i.MX31L UARTs
 */
/*
 * Copyright (c) 2009, 2010  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
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
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ifndef	_IMXUARTVAR_H
#define	_IMXUARTVAR_H


#include  <sys/cdefs.h>
#include  <sys/termios.h>	/* for tcflag_t */


void imxuart_attach_common(device_t parent, device_t self,
    bus_space_tag_t, paddr_t, size_t, int, int);

int imxuart_kgdb_attach(bus_space_tag_t, paddr_t, u_int, tcflag_t);
int imxuart_cons_attach(bus_space_tag_t, paddr_t, u_int, tcflag_t);

int imxuart_is_console(bus_space_tag_t, bus_addr_t, bus_space_handle_t *);

/*
 * Set platform dependent values
 */
void imxuart_set_frequency(u_int, u_int);

/*
 * defined in imx51uart.c and imx31uart.c
 */
int imxuart_match(struct device *, struct cfdata *, void *);
void imxuart_attach(struct device *, struct device *, void *);

#endif	/* _IMXUARTVAR_H */
