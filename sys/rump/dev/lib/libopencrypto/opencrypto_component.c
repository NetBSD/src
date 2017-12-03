/*	$NetBSD: opencrypto_component.c,v 1.1.10.3 2017/12/03 11:39:07 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: opencrypto_component.c,v 1.1.10.3 2017/12/03 11:39:07 jdolecek Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/stat.h>
#include <sys/module.h>

#include <rump-sys/kern.h>
#include <rump-sys/dev.h>
#include <rump-sys/vfs.h>

#include "ioconf.h"

void crypto_init(void);

RUMP_COMPONENT(RUMP_COMPONENT_DEV)
{
	extern const struct cdevsw crypto_cdevsw;
	devmajor_t bmaj, cmaj;
	int error;

	/* go, mydevfs */
	bmaj = cmaj = -1;

	if ((error = devsw_attach("crypto", NULL, &bmaj,
	    &crypto_cdevsw, &cmaj)) != 0)
		panic("cannot attach crypto: %d", error);

	if ((error = rump_vfs_makeonedevnode(S_IFCHR, "/dev/crypto",
	    cmaj, 0)) != 0)
		panic("cannot create /dev/crypto: %d", error);

	/*
	 * Initialize OpenCrypto and its pseudo-devices here
	 */
	crypto_init();
	rump_pdev_add(cryptoattach, 1);
#if 0
	/*
	 * rump defines "_MODULE" in spite of using built-in module
	 * initialization(module_init_class()). So, swcryptoattach_internal()
	 * is called by two functions, one is swcryptoattach() and the other is
	 * swcrypto_modcmd().
	 * That causes "builtin module `swcrypto' failed to init" WARNING message.
	 * To suppress this warning, we let rump use swcrypto_modcmd() call-path
	 * only.
	 *
	 * TODO:
	 * There is still "crypto: unable to register devsw" message. it should
	 * be suppressed.
	 */
	rump_pdev_add(swcryptoattach, 0);
#endif
}
