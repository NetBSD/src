/*	$NetBSD: compat_crypto_50.c,v 1.2 2020/01/27 17:11:27 pgoyette Exp $ */

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Coyote Point Systems, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: compat_crypto_50.c,v 1.2 2020/01/27 17:11:27 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/compat_stub.h> 
#include <sys/module.h>

#include <opencrypto/cryptodev.h>
#include <opencrypto/ocryptodev.h>

/* Module glue for compat ioctl's */

static void
crypto_50_init(void)
{

	MODULE_HOOK_SET(ocryptof_50_hook, ocryptof_ioctl);
}

static void
crypto_50_fini(void)
{

	MODULE_HOOK_UNSET(ocryptof_50_hook);
}

MODULE(MODULE_CLASS_EXEC, compat_crypto_50, "crypto,compat_50");
 
static int
compat_crypto_50_modcmd(modcmd_t cmd, void *arg)
{
 
	switch (cmd) {
	case MODULE_CMD_INIT:
		crypto_50_init();
		return 0;
	case MODULE_CMD_FINI:
		crypto_50_fini();
		return 0;
	default: 
		return ENOTTY;
	}
}

