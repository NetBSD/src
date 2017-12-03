/*	$NetBSD: if_module.h,v 1.1.18.2 2017/12/03 11:39:02 jdolecek Exp $	*/

/*-
 * Copyright (c) 2016 The NetBSD Foundation, Inc.
 * All rights reserved.
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

#ifdef _MODULE
# define IF_MODULE_CFDRIVER_DECL(name) CFDRIVER_DECL(name, DV_IFNET, NULL)
# define IF_MODULE_CFDRIVER_ATTACH(name)			\
    error = config_cfdriver_attach(& name ## _cd);		\
    if (error) {						\
	    aprint_error("%s: unable to register cfdriver for"	\
		"%s, error %d\n", __func__, name ## _cd.cd_name,\
		error);						\
	    break;						\
    }
# define IF_MODULE_CFDRIVER_DETACH(name)			\
    /* Remove device from autoconf database */			\
    error = config_cfdriver_detach(&name ## _cd);		\
    if (error) {						\
	    aprint_error("%s: failed to detach %s cfdriver, "	\
		"error %d\n", __func__, name ## _cd.cd_name,	\
		error);						\
	    break;						\
    }
#else
# define IF_MODULE_CFDRIVER_DECL(name)
# define IF_MODULE_CFDRIVER_ATTACH(name)
# define IF_MODULE_CFDRIVER_DETACH(name)
#endif

#define IF_MODULE(class, name, dep)				\
MODULE(class, if_ ## name, dep);				\
IF_MODULE_CFDRIVER_DECL(name);					\
static int							\
if_ ## name ## _modcmd(modcmd_t cmd, void *arg)			\
{								\
	int error = 0;						\
								\
	switch (cmd) {						\
	case MODULE_CMD_INIT:					\
		IF_MODULE_CFDRIVER_ATTACH(name)			\
		/* Init the unit list/line discipline stuff */	\
		name ## init();					\
		break;						\
								\
	case MODULE_CMD_FINI:					\
		/*						\
		 * Make sure it's ok to detach - no units left,	\
		 * and line discipline is removed		\
		 */						\
		error = name ## detach();			\
		if (error != 0)					\
			break;					\
		IF_MODULE_CFDRIVER_DETACH(name)			\
		break;						\
								\
	case MODULE_CMD_STAT:					\
		error = ENOTTY;					\
		break;						\
	default:						\
		error = ENOTTY;					\
		break;						\
	}							\
								\
	return error;						\
}
