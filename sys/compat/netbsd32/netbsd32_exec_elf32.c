/*	$NetBSD: netbsd32_exec_elf32.c,v 1.45 2019/06/07 23:35:53 christos Exp $	*/
/*	from: NetBSD: exec_aout.c,v 1.15 1996/09/26 23:34:46 cgd Exp */

/*
 * Copyright (c) 1998, 2001 Matthew R. Green.
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

/*
 * Copyright (c) 1993, 1994 Christopher G. Demetriou
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christopher G. Demetriou.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netbsd32_exec_elf32.c,v 1.45 2019/06/07 23:35:53 christos Exp $");

#define	ELFSIZE		32

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>
#include <sys/resourcevar.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/kauth.h>
#include <sys/namei.h>
#include <sys/compat_stub.h>

#include <compat/common/compat_util.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_exec.h>

#include <machine/netbsd32_machdep.h>

int ELFNAME2(netbsd32,probe_noteless)(struct lwp *, struct exec_package *epp,
				      void *eh, char *itp, vaddr_t *pos);
extern int ELFNAME2(netbsd,signature)(struct lwp *, struct exec_package *,
				      Elf_Ehdr *);

int
ELFNAME2(netbsd32,probe)(struct lwp *l, struct exec_package *epp,
			 void *eh, char *itp, vaddr_t *pos)
{
	int error;

	if ((error = ELFNAME2(netbsd,signature)(l, epp, eh)) != 0)
		return error;

#ifdef ELF_MD_PROBE_FUNC
	if ((error = ELF_MD_PROBE_FUNC(l, epp, eh, itp, pos)) != 0)
		return error;
#elif defined(ELF_INTERP_NON_RELOCATABLE)
	*pos = ELF_LINK_ADDR;
#endif

	return ELFNAME2(netbsd32,probe_noteless)(l, epp, eh, itp, pos);
}

int
ELFNAME2(netbsd32,probe_noteless)(struct lwp *l, struct exec_package *epp,
				  void *eh, char *itp, vaddr_t *pos)
{
	const char *m;

 	if (itp && epp->ep_interp == NULL) {
		MODULE_HOOK_CALL(netbsd32_machine32_hook, (), machine, m);
		(void)compat_elf_check_interp(epp, itp, m);
	}
#ifdef _LP64
	epp->ep_flags |= EXEC_32 | EXEC_FORCEAUX;
#endif
	epp->ep_vm_minaddr = exec_vm_minaddr(VM_MIN_ADDRESS);
	epp->ep_vm_maxaddr = USRSTACK32;
#ifdef ELF_INTERP_NON_RELOCATABLE
	*pos = ELF_LINK_ADDR;
#endif
	return 0;
}

/*
 * Copy arguments onto the stack in the normal way, but add some
 * extra information in case of dynamic binding.
 */
int
netbsd32_elf32_copyargs(struct lwp *l, struct exec_package *pack,
    struct ps_strings *arginfo, char **stackp, void *argp)
{
	int error;

	if ((error = netbsd32_copyargs(l, pack, arginfo, stackp, argp)) != 0)
		return error;

	return elf32_populate_auxv(l, pack, stackp);
}
