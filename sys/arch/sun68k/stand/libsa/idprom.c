/*	$NetBSD: idprom.c,v 1.1 2001/06/14 12:57:14 fredette Exp $	*/

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
void idprom_init2 __P((void));
void idprom_init3 __P((void));
void idprom_init3x __P((void));

int
idprom_cksum(p)
	u_char *p;
{
	int len, x;

	len = IDPROM_CKSUM_SIZE;
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
	else if (_is2)
		idprom_init2();
	else
		idprom_init3();

	if (identity_prom.idp_format != 1)
		panic("idprom: bad version\n");
	cpu_machine_id = identity_prom.idp_machtype;
}

/*
 * Sun2 version:
 * Just copy it from control space.
 */
void
idprom_init2()
{

	/* Copy the IDPROM contents and do the checksum. */
	sun2_getidprom((u_char *) &identity_prom);
	if (idprom_cksum((u_char *) &identity_prom))
		printf("idprom: bad checksum\n");
}

/*
 * Sun3 version:
 * Just copy it from control space.
 */
void
idprom_init3()
{

	/* Copy the IDPROM contents and do the checksum. */
	sun3_getidprom((u_char *) &identity_prom);
	if (idprom_cksum((u_char *) &identity_prom))
		printf("idprom: bad checksum\n");
}

/*
 * Sun3X version:
 * Rather than do all the map-in/probe work to find the idprom,
 * we can cheat!  We _know_ the monitor already made a copy of
 * the IDPROM in its data page.  All we have to do is find it.
 *
 * Yeah, this is sorta gross...  Only used on old PROMs that
 * do not have a sif_macaddr function (rev < 3.0).  The area
 * to search was determined from some "insider" info. about
 * the layout of the PROM data area.
 */
void
idprom_init3x()
{
	u_char *p;

	printf("idprom: Sun3X search for soft copy...\n");

	for (p = (u_char *)(SUN3X_MONDATA + 0x0400);
	     p < (u_char *)(SUN3X_MONDATA + 0x1c00); p++)
	{
		/* first check for some constants */
		if (p[0] != 0x01) /* format */
			continue;
		if (p[2] != 0x08) /* ether[0] */
			continue;
		if (p[3] != 0x00) /* ether[1] */
			continue;
		if (p[4] != 0x20) /* ether[2] */
			continue;
		if ((p[1] & 0xfc) != IDM_ARCH_SUN3X)
			continue;
		/* Looks plausible.  Try the checksum. */
		if (idprom_cksum(p) == 0)
			goto found;
	}
	panic("idprom: not found in monitor data\n");

found:
	printf("idprom: copy found at 0x%x\n", (int)p);
	bcopy(p, &identity_prom, sizeof(struct idprom));
}
