/*	$NetBSD: idprom.c,v 1.2 1998/02/05 04:57:11 gwr Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Glass and Gordon W. Ross.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * Machine ID PROM - system type and serial number
 */

#include <sys/types.h>
#include <machine/control.h>
#include <machine/idprom.h>
#include <machine/mon.h>

#include "libsa.h"

/*
 * This driver provides a soft copy of the IDPROM.
 * It is copied from the device early in startup.
 * Allow these to be patched (helps with poor old
 * Sun3/80 boxes with dead NVRAM).
 */
u_char cpu_machine_id = 0;
struct idprom identity_prom = { 0 };

int idprom_cksum __P((u_char *));
void idprom_init3 __P((void));
void idprom_init3x __P((void));

int
idprom_cksum(p)
	u_char *p;
{
	int len, x;

	len = sizeof(struct idprom);
	x = 0;	/* xor of data */
	do x ^= *p++;
	while (--len > 0);
	return (x);
}

/* Copy the ethernet address into the passed space. */
void
idprom_etheraddr(eaddrp)
	u_char *eaddrp;
{

	idprom_init();
	bcopy(identity_prom.idp_etheraddr, eaddrp, 6);
}

/* Fetch a copy of the idprom. */
void
idprom_init()
{

	if (identity_prom.idp_format == 1)
		return;

	/* Copy the IDPROM contents and do the checksum. */
	if (_is3x)
		idprom_init3x();
	else
		idprom_init3();

	if (identity_prom.idp_format != 1)
		panic("idprom: bad version\n");
	cpu_machine_id = identity_prom.idp_machtype;
}

/*
 * Sun3 version:
 * Just copy it from control space.
 */
void
idprom_init3()
{
	u_char *dst;
	vm_offset_t src;	/* control space address */
	int len, x, xorsum;

	/* Copy the IDPROM contents and do the checksum. */
	dst = (u_char *) &identity_prom;
	src = 0;	/* XXX */
	len = sizeof(struct idprom);
	do {
		x = get_control_byte(src++);
		*dst++ = x;
	} while (--len > 0);

	x = idprom_cksum((char *) &identity_prom);
	if (x != 0)
		mon_printf("idprom: bad checksum\n");
}



/*
 * Sun3X version:
 * Rather than do all the map-in/probe work to find the idprom,
 * we can cheat!  We _know_ the monitor already made of copy of
 * the IDPROM in its data page.  All we have to do is find it.
 *
 * Yeah, this is sorta gross...  This is not needed any longer
 * because netif_sun.c now can use the "advertised" function
 * in 3.x PROMs to get the ethernet address.  Let's keep this
 * bit of trickery around for a while anyway, just in case...
 */
void
idprom_init3x()
{
	u_short *p;
	int x;

	/* Search for it.  Range determined empirically. */
	for (p = (u_short *)(SUN3X_MONDATA + 0x0400);
	     p < (u_short *)(SUN3X_MONDATA + 0x1c00); p++)
	{
		/* first some quick checks */
		if ((p[0] & 0xfffc) != 0x140)
			continue;
		if (p[1] != 0x0800)
			continue;
		/* Looks plausible.  Try the checksum. */
		x = idprom_cksum((u_char *) p);
		if (x == 0)
			goto found;
	}
	panic("idprom: not found in monitor data\n");

found:
#ifdef	DEBUG
	printf("idprom: copy found at 0x%x\n", (int)p);
#endif
	bcopy(p, &identity_prom, sizeof(struct idprom));
}
