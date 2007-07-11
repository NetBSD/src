/*	$NetBSD: linux32_utsname.c,v 1.2.8.1 2007/07/11 20:04:25 mjf Exp $ */

/*-
 * Copyright (c) 2006 Emmanuel Dreyfus, all rights reserved.
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
 *	This product includes software developed by Emmanuel Dreyfus
 * 4. The name of the author may not be used to endorse or promote 
 *    products derived from this software without specific prior written 
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE THE AUTHOR AND CONTRIBUTORS ``AS IS'' 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS 
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: linux32_utsname.c,v 1.2.8.1 2007/07/11 20:04:25 mjf Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/fstypes.h>
#include <sys/signal.h>
#include <sys/dirent.h>
#include <sys/kernel.h>
#include <sys/fcntl.h>
#include <sys/select.h>
#include <sys/proc.h>
#include <sys/ucred.h>
#include <sys/swap.h>

#include <machine/types.h>

#include <sys/syscallargs.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_conv.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_machdep.h>
#include <compat/linux/common/linux_misc.h>
#include <compat/linux/common/linux_oldolduname.h>
#include <compat/linux/linux_syscallargs.h>

#include <compat/linux32/common/linux32_types.h>
#include <compat/linux32/common/linux32_signal.h>
#include <compat/linux32/common/linux32_machdep.h>
#include <compat/linux32/common/linux32_sysctl.h>
#include <compat/linux32/common/linux32_socketcall.h>
#include <compat/linux32/linux32_syscallargs.h>

int
linux32_sys_uname(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_uname_args /* {
		syscallarg(linux32_utsnamep) up;
	} */ *uap = v;
	struct linux_utsname luts;
	struct linux_utsname *lp;

	strncpy(luts.l_sysname, linux32_sysname, sizeof(luts.l_sysname));
	strncpy(luts.l_nodename, hostname, sizeof(luts.l_nodename));
	strncpy(luts.l_release, linux32_release, sizeof(luts.l_release));
	strncpy(luts.l_version, linux32_version, sizeof(luts.l_version));
#ifdef LINUX_UNAME_ARCH
	strncpy(luts.l_machine, LINUX_UNAME_ARCH, sizeof(luts.l_machine));
#else  
	strncpy(luts.l_machine, machine, sizeof(luts.l_machine));
#endif 
	strncpy(luts.l_domainname, domainname, sizeof(luts.l_domainname));
       
	lp = SCARG_P32(uap, up);

        return copyout(&luts, lp, sizeof(luts));
}

int   
linux32_sys_oldolduname(l, v, retval)
        struct lwp *l;
        void *v;
        register_t *retval;
{
        struct linux32_sys_uname_args /* {
                syscallarg(linux32_oldoldutsnamep_t) up;
        } */ *uap = v; 
        struct linux_oldoldutsname luts;
 
        strncpy(luts.l_sysname, linux32_sysname, sizeof(luts.l_sysname));
        strncpy(luts.l_nodename, hostname, sizeof(luts.l_nodename));
        strncpy(luts.l_release, linux32_release, sizeof(luts.l_release));
        strncpy(luts.l_version, linux32_version, sizeof(luts.l_version));
        strncpy(luts.l_machine, machine, sizeof(luts.l_machine));
 
        return copyout(&luts, SCARG_P32(uap, up), sizeof(luts));
}
