/*	$NetBSD: hbvar.h,v 1.1.2.1 1999/12/27 18:32:56 wrstuden Exp $	*/

/*-
 * Copyright (C) 1999 Izumi Tsutsui.  All rights reserved.
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Shorthand for locators. */
#include "locators.h"
#define cf_addr	cf_loc[HBCF_ADDR]
#define cf_ipl	cf_loc[HBCF_IPL]
#define cf_vect	cf_loc[HBCF_VECT]

/*
 * Structure used to attach hb devices.
 */
struct hb_attach_args {
	char	*ha_name;	/* name of device */
	u_long	ha_address;	/* device address */
	int	ha_ipl;		/* interrupt level */
	int	ha_vect;	/* interrupt vector */
};

void	hb_intr_establish __P((int, int (*)(void *), int, void *));
void	hb_intr_disestablish __P((int));

#ifdef news1700 /* XXX these should be defined dinamically? */
#define	CTRL_LED	0xe0dc0000
#define	 LED0		0x01
#define	 LED1		0x02

#define	CTRL_TIMER	0xe1000000

#define LANCE_PORT	0xe0f00000
#define	LANCE_MEMORY	0xe0e00000
#define	LANCE_ID	0xe1c00000

#define LANCE_PORT1	0xf0c30000
#define	LANCE_MEMORY1	0xf0c20000
#define	LANCE_ID1	0xf0c38000

#define	LANCE_PORT2	0xf0c70000
#define	LANCE_MEMORY2	0xf0c60000
#define	LANCE_ID2	0xf0c78000

#define	SCCPORT0A	0xe0d40002
#define	SCCPORT0B	0xe0d40000

#define	SCSI_BASE	0xe0cc0000
#define	DMAC_BASE	0xe0e80000
#endif
