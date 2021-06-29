/*	$NetBSD: dev_verbose.h,v 1.8 2021/06/29 21:03:36 pgoyette Exp $ */

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#ifndef _DEV_DEV_VERBOSE_H_
#define	_DEV_DEV_VERBOSE_H_

#ifdef _KERNEL
#include <sys/module_hook.h>
#endif

const char *dev_findvendor(char *, size_t, const char *, size_t, 
	const uint32_t *, size_t, uint32_t, const char *);
const char *dev_findproduct(char *, size_t, const char *, size_t, 
	const uint32_t *, size_t, uint32_t, uint32_t, const char *);

#define DEV_VERBOSE_COMMON_DEFINE(tag)					\
static const char *							\
tag ## _findvendor_real(char *buf, size_t len, uint32_t vendor)		\
{									\
	return dev_findvendor(buf, len, tag ## _words,			\
	    __arraycount(tag ## _words), tag ## _vendors,		\
	    __arraycount(tag ## _vendors), vendor, tag ## _id1_format);	\
}									\
									\
static const char *							\
tag ## _findproduct_real(char *buf, size_t len, uint32_t vendor,	\
    uint32_t product)							\
{									\
	return dev_findproduct(buf, len, tag ## _words,			\
	    __arraycount(tag ## _words), tag ## _products,		\
	    __arraycount(tag ## _products), vendor, product,		\
	    tag ## _id2_format);					\
}									\

#ifdef _KERNEL

#define DEV_VERBOSE_KERNEL_DECLARE(tag)					\
MODULE_HOOK(tag ## _findvendor_hook, const char *,			\
	(char *, size_t, uint32_t));					\
MODULE_HOOK(tag ## _findproduct_hook, const char *,			\
	(char *, size_t, uint32_t, uint32_t));				\
extern int tag ## verbose_loaded;

#define DEV_VERBOSE_MODULE_DEFINE(tag, deps)				\
DEV_VERBOSE_COMMON_DEFINE(tag)						\
DEV_VERBOSE_KERNEL_DECLARE(tag)						\
									\
static int								\
tag ## verbose_modcmd(modcmd_t cmd, void *arg)				\
{									\
									\
	switch (cmd) {							\
	case MODULE_CMD_INIT:						\
		MODULE_HOOK_SET(tag ## _findvendor_hook,		\
			tag ## _findvendor_real);			\
		MODULE_HOOK_SET(tag ## _findproduct_hook,		\
			tag ## _findproduct_real);			\
		tag ## verbose_loaded = 1;				\
		return 0;						\
	case MODULE_CMD_FINI:						\
		tag ## verbose_loaded = 0;				\
		MODULE_HOOK_UNSET(tag ## _findproduct_hook);		\
		MODULE_HOOK_UNSET(tag ## _findvendor_hook);		\
		return 0;						\
	default:							\
		return ENOTTY;						\
	}								\
}									\
MODULE(MODULE_CLASS_DRIVER, tag ## verbose, deps)

#endif /* KERNEL */

#define DEV_VERBOSE_DECLARE(tag)					\
extern const char * tag ## _findvendor(char *, size_t, uint32_t);	\
extern const char * tag ## _findproduct(char *, size_t, uint32_t, uint32_t)

#if defined(_KERNEL)

#define DEV_VERBOSE_DEFINE(tag)						\
DEV_VERBOSE_KERNEL_DECLARE(tag)						\
struct tag ## _findvendor_hook_t tag ## _findvendor_hook;		\
struct tag ## _findproduct_hook_t tag ## _findproduct_hook;		\
int tag ## verbose_loaded = 0;						\
									\
static void								\
tag ## _load_verbose(void)						\
{									\
									\
	if (tag ## verbose_loaded == 0)					\
		module_autoload(# tag "verbose", MODULE_CLASS_DRIVER);	\
}									\
									\
const char *								\
tag ## _findvendor(char *buf, size_t len, uint32_t vendor)		\
{									\
	const char *retval = NULL;					\
									\
	tag ## _load_verbose();						\
	MODULE_HOOK_CALL(tag ## _findvendor_hook, (buf, len, vendor),	\
		(snprintf(buf, len, tag ## _id1_format, vendor), NULL),	\
		retval);						\
	return retval;							\
}									\
									\
const char *								\
tag ## _findproduct(char *buf, size_t len, uint32_t vendor,		\
    uint32_t product)							\
{									\
	const char *retval = NULL;					\
									\
	tag ## _load_verbose();						\
	MODULE_HOOK_CALL(tag ## _findproduct_hook,			\
		(buf, len, vendor, product),				\
		(snprintf(buf, len, tag ## _id2_format, product), NULL), \
		retval);						\
	return retval;							\
}									\

#else	/* _KERNEL */

#define DEV_VERBOSE_DEFINE(tag)						\
DEV_VERBOSE_COMMON_DEFINE(tag)						\
const char *tag ## _findvendor(char *buf, size_t len, uint32_t vendor)	\
{									\
									\
	return tag ## _findvendor_real(buf, len, vendor);		\
}									\
									\
const char *tag ## _findproduct(char *buf, size_t len, uint32_t vendor,	\
		uint32_t product)					\
{									\
									\
	return tag ## _findproduct_real(buf, len, vendor, product);	\
}

#endif /* _KERNEL */

#endif /* _DEV_DEV_VERBOSE_H_ */
