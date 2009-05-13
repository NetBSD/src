/*	$NetBSD: db_user.h,v 1.1.6.2 2009/05/13 17:19:04 jym Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

#ifndef _DDB_DB_USER_H_
#define _DDB_DB_USER_H_
#ifndef _KERNEL

#include <sys/errno.h>
#include <sys/uio.h>

#include <machine/vmparam.h>

#include <uvm/uvm_extern.h>

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <setjmp.h>

#define	KASSERT		assert
#define	KDASSERT	assert
#define	longjmp(a)	longjmp((void *)(a), 1)
#define	setjmp(a)	setjmp((void *)(a))

typedef jmp_buf label_t;

#ifndef offsetof
#define	offsetof(type, member) \
     ((size_t)(unsigned long)(&(((type *)0)->member)))
#endif

int	cngetc(void);
void	cnputc(int);

#endif	/* !_KERNEL */
#endif	/* !_DDB_DB_USER_H_ */
