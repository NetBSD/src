/*	$NetBSD: hbvar.h,v 1.5.104.1 2008/06/02 13:22:28 mjf Exp $	*/

/*-
 * Copyright (c) 1999 Izumi Tsutsui.  All rights reserved.
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <machine/bus.h>

/* Shorthand for locators. */
#include "locators.h"
#define cf_addr	cf_loc[HBCF_ADDR]
#define cf_ipl	cf_loc[HBCF_IPL]
#define cf_vect	cf_loc[HBCF_VECT]

/*
 * Structure used to attach hb devices.
 */
struct hb_attach_args {
	const char	*ha_name;	/* name of device */
	bus_space_tag_t	ha_bust;	/* bus space tag */
	bus_addr_t	ha_address;	/* device address */
	bus_size_t	ha_size;	/* device space */
	int		ha_ipl;		/* interrupt level */
	int		ha_vect;	/* interrupt vector */
};

void hb_intr_establish(int, int (*)(void *), int, void *);
void hb_intr_disestablish(int);
