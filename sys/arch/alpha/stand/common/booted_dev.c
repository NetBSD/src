/* $NetBSD: booted_dev.c,v 1.3 1999/11/13 21:38:20 thorpej Exp $ */

/*
 * Copyright (c) 1999 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include <machine/prom.h>
#include "common.h"

long	booted_dev_fd;
#if defined(PRIMARY_BOOTBLOCK) || defined(UNIFIED_BOOTBLOCK)
char	booted_dev_name[BOOTED_DEV_MAXNAMELEN];
#endif /* defined(PRIMARY_BOOTBLOCK) || defined(UNIFIED_BOOTBLOCK) */

#if defined(PRIMARY_BOOTBLOCK) || defined(UNIFIED_BOOTBLOCK)
int
booted_dev_open(void)
{
	prom_return_t ret;
	int devlen;

	/*
	 * XXX
	 * We don't know what device names look like yet,
	 * so we can't change them.
	 */
	ret.bits = prom_getenv(PROM_E_BOOTED_DEV, booted_dev_name,
	    sizeof(booted_dev_name));
	devlen = ret.u.retval;

	ret.bits = prom_open(booted_dev_name, devlen);

	if (ret.u.status)
		return 0;
	booted_dev_fd = ret.u.retval;

	return 1;
}
#endif
