/*	$NetBSD: rump_dev.c,v 1.9 2009/12/04 22:13:59 haad Exp $	*/

/*
 * Copyright (c) 2009 Antti Kantee.  All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rump_dev.c,v 1.9 2009/12/04 22:13:59 haad Exp $");

#include <sys/param.h>
#include <sys/device.h>

#include "rump_dev_private.h"

void nocomponent(void);
void nocomponent() {}
__weak_alias(rump_dev_cgd_init,nocomponent);
__weak_alias(rump_dev_dm_init,nocomponent);
__weak_alias(rump_dev_raidframe_init,nocomponent);
__weak_alias(rump_dev_netsmb_init,nocomponent);
__weak_alias(rump_dev_rnd_init,nocomponent);
__weak_alias(rump_dev_rumpusbhc_init,nocomponent);

__weak_alias(rump_device_configuration,nocomponent);

const char *rootspec = "rump0a"; /* usually comes from config */

void
rump_dev_init(void)
{
	extern int cold;

	KERNEL_LOCK(1, curlwp);

	config_init_mi();

	rump_dev_cgd_init();
	rump_dev_dm_init();
	rump_dev_raidframe_init();
	rump_dev_netsmb_init();
	rump_dev_rnd_init();
	rump_dev_rumpusbhc_init();

	rump_pdev_finalize();

	rump_device_configuration();

	cold = 0;
	if (config_rootfound("mainbus", NULL) == NULL)
		panic("no mainbus");

	config_finalize();

	KERNEL_UNLOCK_LAST(curlwp);
}

void
cpu_rootconf(void)
{

	panic("%s: unimplemented", __func__);
}

#ifdef __HAVE_DEVICE_REGISTER
void
device_register(struct device *dev, void *v)
{

	/* nada */
}
#endif
