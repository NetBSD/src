/*	$NetBSD: linux_exec_alpha.c,v 1.2 2001/01/18 17:48:04 tv Exp $	*/

#define ELFSIZE 64

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>

#include <compat/linux/common/linux_exec.h>

/*
 * Alpha specific linux copyargs function.
 *
 * XXX Figure out if this is common to more than one linux
 * XXX port.  If so, move it to common/linux_exec_elf32.c
 * XXX included based on some define.
 */
void *
ELFNAME2(linux,copyargs)(struct exec_package *pack, struct ps_strings *arginfo,	
			 void *stack, void *argp)
{
	size_t len;
	LinuxAuxInfo ai[LINUX_ELF_AUX_ENTRIES], *a;
	struct elf_args *ap;

	stack = copyargs(pack, arginfo, stack, argp);
	if (!stack)
		return(NULL);

	memset(ai, 0, sizeof(LinuxAuxInfo) * LINUX_ELF_AUX_ENTRIES);

	a = ai;

	/*
	 * Push extra arguments on the stack needed by dynamically
	 * linked binaries.
	 */
	if ((ap = (struct elf_args *)pack->ep_emul_arg)) {

		a->a_type = AT_PHDR;
		a->a_v = ap->arg_phaddr;
		a++;

		a->a_type = AT_PHENT;
		a->a_v = ap->arg_phentsize;
		a++;

		a->a_type = AT_PHNUM;
		a->a_v = ap->arg_phnum;
		a++;

		a->a_type = AT_PAGESZ;
		a->a_v = NBPG;
		a++;

		a->a_type = AT_BASE;
		a->a_v = ap->arg_interp;
		a++;

		a->a_type = AT_FLAGS;
		a->a_v = 0;
		a++;

		a->a_type = AT_ENTRY;
		a->a_v = ap->arg_entry;
		a++;

		a->a_type = AT_BASE;
		a->a_v = ap->arg_interp;
		a++;

#if 0
/*
 * The exec_package doesn't have a proc pointer and it's not
 * exactly trivial to add one since the credentials are changing.
 */
		a->a_type = LINUX_AT_UID;
		a->a_v = pack->p->p_cred->p_ruid;
		a++;

		a->a_type = LINUX_AT_EUID;
		a->a_v = pack->p->p_ucred->cr_uid;
		a++;

		a->a_type = LINUX_AT_GID;
		a->a_v = pack->p->p_cred->p_rgid;
		a++;

		a->a_type = LINUX_AT_EGID;
		a->a_v = pack->p->p_ucred->cr_gid;
		a++;
#endif

		free((char *)ap, M_TEMP);
		pack->ep_emul_arg = NULL;
	}

	a->a_type = AT_NULL;
	a->a_v = 0;
	a++;

	len = (a - ai) * sizeof(LinuxAuxInfo);
	if (copyout(ai, stack, len))
		return NULL;
	stack = (caddr_t)stack + len;

	return(stack);
}
