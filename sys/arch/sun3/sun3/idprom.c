/*	$NetBSD: idprom.c,v 1.29.44.1 2014/08/20 00:03:26 tls Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: idprom.c,v 1.29.44.1 2014/08/20 00:03:26 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <machine/autoconf.h>
#include <machine/idprom.h>

#include <sun3/sun3/machdep.h>
#ifdef _SUN3_
#include <sun3/sun3/control.h>
#elif _SUN3X_
#include <sun3/sun3x/obio.h>
#endif

/*
 * This structure is what this driver is all about.
 * It is copied from the device early in startup.
 */
struct idprom identity_prom;

static int idprom_cksum(u_char *);
static void idprom_get(u_char *);
static int idprom_hostid(void);

/*
 * Copy the IDPROM contents,
 * verify the checksum,
 * set the hostid...
 */
void
idprom_init(void)
{

	idprom_get((u_char *)&identity_prom);
	if (idprom_cksum((u_char *) &identity_prom))
		printf("idprom: bad checksum\n");
	if (identity_prom.idp_format < 1)
		printf("idprom: bad version\n");

	cpu_machine_id = identity_prom.idp_machtype;
	hostid = idprom_hostid();
}

static int
idprom_cksum(u_char *p)
{
	int len, x;

	len = IDPROM_CKSUM_SIZE;
	x = 0;	/* xor of data */
	do x ^= *p++;
	while (--len > 0);
	return (x);
}

static int
idprom_hostid(void)
{
	struct idprom *idp;
	union {
		long l;
		char c[4];
	} hid;

	/*
	 * Construct the hostid from the idprom contents.
	 * This appears to be the way SunOS does it.
	 */
	idp = &identity_prom;
	hid.c[0] = idp->idp_machtype;
	hid.c[1] = idp->idp_serialnum[0];
	hid.c[2] = idp->idp_serialnum[1];
	hid.c[3] = idp->idp_serialnum[2];
	return (hid.l);
}

void
idprom_etheraddr(u_char *eaddrp)
{

	memcpy(eaddrp, identity_prom.idp_etheraddr, 6);
}

/*
 * Machine specific stuff follows.
 */

#ifdef _SUN3_
/*
 * Copy the IDPROM to memory.
 *
 * On the Sun3, this is called very early,
 * because we need the cputype.
 */
static void
idprom_get(u_char *dst)
{
	vaddr_t src;	/* control space address */
	int len, x;

	src = IDPROM_BASE;
	len = IDPROM_SIZE;
	do {
		x = get_control_byte(src++);
		*dst++ = x;
	} while (--len > 0);
}

#endif /* SUN3 */
#ifdef _SUN3X_
#error "not yet merged"
#endif /* SUN3X */
