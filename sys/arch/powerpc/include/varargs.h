/*	$NetBSD: varargs.h,v 1.6.2.1 2002/06/23 17:39:43 jdolecek Exp $	*/

/*-
 * Copyright (c) 2000 Tsubai Masanari.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _POWERPC_VARARGS_H_
#define _POWERPC_VARARGS_H_

#include <machine/stdarg.h>

#define va_alist	__builtin_va_alist
#define va_dcl		int va_alist; ...

#undef va_start

#ifdef __lint__
#define va_start(ap)	((ap) = *(va_list *)0)
#elif __GNUC_PREREQ__(3, 0)
#define va_start(ap)	__builtin_varargs_start((ap).__va)
#elif __GNUC_PREREQ__(2, 95)
#define va_start(ap)	((ap) = *(va_list *)__builtin_saveregs())
#undef va_alist
#define va_alist __va_1st_arg
#else
#define	va_start(ap)							\
	((ap).__stack = __va_stack_args,				\
	 (ap).__base = __va_reg_args,					\
	 (ap).__gpr = __va_first_gpr,					\
	 (ap).__fpr = __va_first_fpr)
#endif

#endif /* _POWERPC_VARARGS_H_ */
