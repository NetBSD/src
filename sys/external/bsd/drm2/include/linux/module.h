/*	$NetBSD: module.h,v 1.7 2018/08/27 15:22:54 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#ifndef _LINUX_MODULE_H_
#define _LINUX_MODULE_H_

/* XXX Get this first so we don't nuke the module_init declaration.  */
#include <sys/module.h>

#include <linux/export.h>
#include <linux/moduleparam.h>

#define	module_init(INIT)
#define	module_exit(EXIT)

#define	MODULE_AUTHOR(AUTHOR)
#define	MODULE_DESCRIPTION(DESCRIPTION)
#define	MODULE_DEVICE_TABLE(DESCRIPTION, IDLIST)
#define	MODULE_FIRMWARE(FIRMWARE)
#define	MODULE_LICENSE(LICENSE)
struct linux_module_param_desc {
	const char *name;
	const char *description;
};
#define	MODULE_PARM_DESC(PARAMETER, DESCRIPTION) \
static __attribute__((__used__)) \
const struct linux_module_param_desc PARAMETER ## _desc = { \
    .name = # PARAMETER, \
    .description = DESCRIPTION, \
}; \
__link_set_add_rodata(linux_module_param_desc, PARAMETER ## _desc)

#define	THIS_MODULE	0
#define	KBUILD_MODNAME	__file__

#endif  /* _LINUX_MODULE_H_ */
