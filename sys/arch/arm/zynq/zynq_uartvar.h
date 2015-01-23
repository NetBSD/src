/*	$NetBSD: zynq_uartvar.h,v 1.1 2015/01/23 12:34:09 hkenken Exp $	*/
/*-
 * Copyright (c) 2015  Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec Corporation.
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

#ifndef	_ARM_ZYNQ_ZYNQ_UARTVAR_H
#define	_ARM_ZYNQ_ZYNQ_UARTVAR_H

#include  <sys/cdefs.h>
#include  <sys/termios.h>	/* for tcflag_t */

#define ERROR_BITS __BITS(31, 16)

void zynquart_attach_common(device_t parent, device_t self,
    bus_space_tag_t, paddr_t, size_t, int, int);

int zynquart_kgdb_attach(bus_space_tag_t, paddr_t, u_int, tcflag_t);
int zynquart_cons_attach(bus_space_tag_t, paddr_t, u_int, tcflag_t);

int zynquart_is_console(bus_space_tag_t, bus_addr_t, bus_space_handle_t *);

/*
 * Set platform dependent values
 */
void zynquart_set_frequency(u_int, u_int);

/*
 * defined in zynq7000_uart.c
 */
int zynquart_match(device_t, cfdata_t, void *);
void zynquart_attach(device_t, device_t, void *);

#endif /* _ARM_ZYNQ_ZYNQ_UARTVAR_H */
