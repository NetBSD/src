/*	$NetBSD: apbusvar.h,v 1.2 1999/12/23 06:52:30 tsubai Exp $	*/

/*-
 * Copyright (C) 1999 SHIMIZU Ryo.  All rights reserved.
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

#include <machine/apbus.h>

/*
 * Arguments used to attach devices to an ibus
 */
struct apbus_attach_args {
	char	*apa_name;	/* device name (ex. "sonic", "esccf") */
	int	apa_ctlnum;	/* my unit number (ex. 0, 1, 2, ..) */
	int	apa_slotno;	/* which slot in */

	u_long	apa_hwbase;	/* hardware I/O address */
};

void *apbus_device_to_hwaddr __P((struct apbus_dev *));
struct apbus_dev *apbus_lookupdev __P((char *));
void apdevice_dump __P((struct apbus_dev *));
void apbus_intr_init __P((void));
int apbus_intr_call __P((int, int));
void *apbus_intr_establish __P((int, int, int, void (*)(void *), void *,
				char *, int));

#define	SLOTTOMASK(slot)	((slot) ? (0x0100 << ((slot) - 1)) : 0)
