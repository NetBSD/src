/*	$NetBSD: irix_fcntl.c,v 1.2.4.3 2002/03/16 16:00:27 jdolecek Exp $ */

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: irix_fcntl.c,v 1.2.4.3 2002/03/16 16:00:27 jdolecek Exp $");

#include <sys/types.h>
#include <sys/signal.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/syscallargs.h>

#include <compat/irix/irix_types.h>
#include <compat/irix/irix_signal.h>
#include <compat/irix/irix_syscallargs.h>


int
irix_sys_lseek64(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	/* 
	 * Note: we have an alignement problem here. If pad2, pad3 and pad4 
	 * are removed, lseek64 will break, because whence will be wrong.
	 */
	struct irix_sys_lseek64_args /* {
		syscallarg(int) fd;
		syscallarg(int) pad1;
		syscallarg(irix_off64_t) offset;
		syscallarg(int) whence;
		syscallarg(int) pad2;
		syscallarg(int) pad3;
		syscallarg(int) pad4;
	} */ *uap = v;
	struct sys_lseek_args cup;

#ifdef DEBUG_IRIX
	printf("irix_sys_lseek64(): fd = %d, pad1 = 0x%08x, offset = 0x%llx\n",
	    SCARG(uap, fd), SCARG(uap, pad1), SCARG(uap, offset));
	printf("whence = 0x%08x, pad2 = 0x%08x, pad3 = 0x%08x, pad4 = 0x%08x\n",
	    SCARG(uap, whence), SCARG(uap, pad2), SCARG(uap, pad3), 
	    SCARG(uap, pad4));
#endif
	SCARG(&cup, fd) = SCARG(uap, fd);
	SCARG(&cup, pad) = 0;
	SCARG(&cup, offset) = SCARG(uap, offset);
	SCARG(&cup, whence) = SCARG(uap, whence);

	return sys_lseek(p, (void *)&cup, retval);
}
