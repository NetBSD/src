/*	$NetBSD: linux_exec_elf32.c,v 1.7 1995/06/11 15:15:09 fvdl Exp $	*/

/*
 * Copyright (c) 1995 Frank van der Linden
 * Copyright (c) 1994 Christos Zoulas
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
 * 3. The name of the author may not be used to endorse or promote products
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
 *
 * based on exec_aout.c, sunos_exec.c and svr4_exec.c
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/resourcevar.h>
#include <sys/wait.h>

#include <sys/mman.h>
#include <vm/vm.h>
#include <vm/vm_param.h>
#include <vm/vm_map.h>
#include <vm/vm_kern.h>
#include <vm/vm_pager.h>

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/exec.h>
#include <machine/linux_machdep.h>

#include <compat/linux/linux_types.h>
#include <compat/linux/linux_syscall.h>
#include <compat/linux/linux_syscallargs.h>
#include <compat/linux/linux_util.h>
#include <compat/linux/linux_exec.h>

struct elf_args {
	u_long  arg_entry;	/* progran entry point */
	u_long  arg_interp;	/* Interpreter load address */
	u_long  arg_phaddr;	/* program header address */
	u_long  arg_phentsize;	/* Size of program header */
	u_long  arg_phnum;	/* Number of program headers */
};

static void *linux_aout_copyargs __P((struct exec_package *,
	struct ps_strings *, void *, void *));
static void *linux_elf_copyargs __P((struct exec_package *, struct ps_strings *,
	void *, void *));
static int linux_elf_check_header __P((Elf32_Ehdr *, int));
static void linux_elf_load_psection __P((struct exec_vmcmd_set *,
	struct vnode *, Elf32_Phdr *, u_long *, u_long *, int *));
static int linux_elf_set_segment __P((struct exec_package *, u_long, u_long,
	int));
static int linux_elf_read_from __P((struct vnode *, u_long, struct proc *,
	caddr_t, int));
static int linux_elf_load_file __P((struct proc *, char *,
	struct exec_vmcmd_set *, u_long *, struct elf_args *, u_long *));

#ifdef DEBUG_EXEC_LINUX_ELF
#define DPRINTF(x) printf x
#else
#define DPRINTF(x)
#endif

#define LINUX_ELF_ALIGN(a, b) ((a) & ~((b) - 1))
#define LINUX_ELF_AUX_ARGSIZ (sizeof(AuxInfo) * 8 / sizeof(char *))
#define	LINUX_AOUT_AUX_ARGSIZ	2

extern int linux_error[];
extern struct sysent linux_sysent[];
extern char *linux_syscallnames[];

struct emul emul_linux_aout = {
	"linux",
	linux_error,
	linux_sendsig,
	LINUX_SYS_syscall,
	LINUX_SYS_MAXSYSCALL,
	linux_sysent,
	linux_syscallnames,
	LINUX_AOUT_AUX_ARGSIZ,
	linux_aout_copyargs,
	setregs,
	linux_sigcode,
	linux_esigcode,
};

struct emul emul_linux_elf = {
	"linux",
	linux_error,
	linux_sendsig,
	LINUX_SYS_syscall,
	LINUX_SYS_MAXSYSCALL,
	linux_sysent,
	linux_syscallnames,
	LINUX_ELF_AUX_ARGSIZ,
	linux_elf_copyargs,
	setregs,
	linux_sigcode,
	linux_esigcode,
};


static void *
linux_aout_copyargs(pack, arginfo, stack, argp)
	struct exec_package *pack;
	struct ps_strings *arginfo;
	void *stack;
	void *argp;
{
	char **cpp = stack;
	char **stk = stack;
	char *dp, *sp;
	size_t len;
	void *nullp = NULL;
	int argc = arginfo->ps_nargvstr;
	int envc = arginfo->ps_nenvstr;

	if (copyout(&argc, cpp++, sizeof(argc)))
		return NULL;

	/* leave room for envp and argv */
	cpp += 2;
	if (copyout(&cpp, &stk[1], sizeof (cpp)))
		return NULL;

	dp = (char *) (cpp + argc + envc + 2);
	sp = argp;

	/* XXX don't copy them out, remap them! */
	arginfo->ps_argvstr = cpp; /* remember location of argv for later */

	for (; --argc >= 0; sp += len, dp += len)
		if (copyout(&dp, cpp++, sizeof(dp)) ||
		    copyoutstr(sp, dp, ARG_MAX, &len))
			return NULL;

	if (copyout(&nullp, cpp++, sizeof(nullp)))
		return NULL;

	if (copyout(&cpp, &stk[2], sizeof (cpp)))
		return NULL;

	arginfo->ps_envstr = cpp; /* remember location of envp for later */

	for (; --envc >= 0; sp += len, dp += len)
		if (copyout(&dp, cpp++, sizeof(dp)) ||
		    copyoutstr(sp, dp, ARG_MAX, &len))
			return NULL;

	if (copyout(&nullp, cpp++, sizeof(nullp)))
		return NULL;

	return cpp;
}

static void *
linux_elf_copyargs(pack, arginfo, stack, argp)
	struct exec_package *pack;
	struct ps_strings *arginfo;
	void *stack;
	void *argp;
{
	char **cpp = stack;
	char *dp, *sp;
	size_t len;
	void *nullp = NULL;
	int argc = arginfo->ps_nargvstr;
	int envc = arginfo->ps_nenvstr;
	AuxInfo *a;
	struct elf_args *ap;

	if (copyout(&argc, cpp++, sizeof(argc)))
		return NULL;

	dp = (char *) (cpp + argc + envc + 2 + pack->ep_emul->e_arglen);
	sp = argp;

	/* XXX don't copy them out, remap them! */
	arginfo->ps_argvstr = cpp; /* remember location of argv for later */

	for (; --argc >= 0; sp += len, dp += len)
		if (copyout(&dp, cpp++, sizeof(dp)) ||
		    copyoutstr(sp, dp, ARG_MAX, &len))
			return NULL;

	if (copyout(&nullp, cpp++, sizeof(nullp)))
		return NULL;

	arginfo->ps_envstr = cpp; /* remember location of envp for later */

	for (; --envc >= 0; sp += len, dp += len)
		if (copyout(&dp, cpp++, sizeof(dp)) ||
		    copyoutstr(sp, dp, ARG_MAX, &len))
			return NULL;

	if (copyout(&nullp, cpp++, sizeof(nullp)))
		return NULL;

	/*
	 * Push extra arguments on the stack needed by dynamically
	 * linked binaries
	 */
	a = (AuxInfo *) cpp;
	if ((ap = (struct elf_args *) pack->ep_emul_arg)) {

		DPRINTF(("phaddr=0x%x, phsize=%d, phnum=%d, interp=0x%x, ",
			 ap->arg_phaddr, ap->arg_phentsize, ap->arg_phnum,
			 ap->arg_interp));
		DPRINTF((" entry=0x%x\n", ap->arg_entry));

		a->au_id = AUX_phdr;
		a->au_v = ap->arg_phaddr;
		a++;

		a->au_id = AUX_phent;
		a->au_v = ap->arg_phentsize;
		a++;

		a->au_id = AUX_phnum;
		a->au_v = ap->arg_phnum;
		a++;

		a->au_id = AUX_pagesz;
		a->au_v = NBPG;
		a++;

		a->au_id = AUX_base;
		a->au_v = ap->arg_interp;
		a++;

		a->au_id = AUX_flags;
		a->au_v = 0;
		a++;

		a->au_id = AUX_entry;
		a->au_v = ap->arg_entry;
		a++;

		a->au_id = AUX_null;
		a->au_v = 0;
		a++;

		free((char *) ap, M_TEMP);
	}
	return a;
}

#ifdef DEBUG_EXEC_LINUX_ELF
static void
print_Ehdr(e)
	Elf32_Ehdr     *e;
{
	printf("e_ident %s, ", e->e_ident);
	printf("e_type %d, ", e->e_type);
	printf("e_machine %d, ", e->e_machine);
	printf("e_version %ld, ", e->e_version);
	printf("e_entry %lx, ", e->e_entry);
	printf("e_phoff %lx, ", e->e_phoff);
	printf("e_shoff %lx, ", e->e_shoff);
	printf("e_flags %lx, ", e->e_flags);
	printf("e_ehsize %d, ", e->e_ehsize);
	printf("e_phentsize %d, ", e->e_phentsize);
	printf("e_phnum %d, ", e->e_phnum);
	printf("e_shentsize %d, ", e->e_shentsize);
	printf("e_shnum %d, ", e->e_shnum);
	printf("e_shstrndx %d\n", e->e_shstrndx);
}


static void
print_Phdr(p)
	Elf32_Phdr     *p;
{
	static char    *types[] =
	{
		"null", "load", "dynamic", "interp",
		"note", "shlib", "phdr", "entry7"
	};

	printf("p_type %ld [%s], ", p->p_type, types[p->p_type & 7]);
	printf("p_offset %lx, ", p->p_offset);
	printf("p_vaddr %lx, ", p->p_vaddr);
	printf("p_paddr %lx, ", p->p_paddr);
	printf("p_filesz %ld, ", p->p_filesz);
	printf("p_memsz %ld, ", p->p_memsz);
	printf("p_flags %lx, ", p->p_flags);
	printf("p_align %ld\n", p->p_align);
}
#endif

int
exec_linux_aout_makecmds(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	struct exec *linux_ep = epp->ep_hdr;
	int machtype, magic;
	int error = ENOEXEC;

	magic = LINUX_N_MAGIC(linux_ep);
	machtype = LINUX_N_MACHTYPE(linux_ep);


	if (machtype != LINUX_MID_MACHINE)
		return (ENOEXEC);

	switch (magic) {
	case QMAGIC:
		error = exec_linux_aout_prep_qmagic(p, epp);
		break;
	case ZMAGIC:
		error = exec_linux_aout_prep_zmagic(p, epp);
		break;
	case NMAGIC:
		error = exec_linux_aout_prep_nmagic(p, epp);
		break;
	case OMAGIC:
		error = exec_linux_aout_prep_omagic(p, epp);
		break;
	}
	if (error == 0)
		epp->ep_emul = &emul_linux_aout;
	return error;
}

/*
 * Since text starts at 0x400 in Linux ZMAGIC executables, and 0x400
 * is very likely not page aligned on most architectures, it is treated
 * as an NMAGIC here. XXX
 */

int
exec_linux_aout_prep_zmagic(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	struct exec *execp = epp->ep_hdr;

	epp->ep_taddr = LINUX_N_TXTADDR(*execp, ZMAGIC);
	epp->ep_tsize = execp->a_text;
	epp->ep_daddr = LINUX_N_DATADDR(*execp, ZMAGIC);
	epp->ep_dsize = execp->a_data + execp->a_bss;
	epp->ep_entry = execp->a_entry;

	/* set up command for text segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn, execp->a_text,
	    epp->ep_taddr, epp->ep_vp, LINUX_N_TXTOFF(*execp, ZMAGIC),
	    VM_PROT_READ|VM_PROT_EXECUTE);

	/* set up command for data segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn, execp->a_data,
	    epp->ep_daddr, epp->ep_vp, LINUX_N_DATOFF(*execp, ZMAGIC),
	    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	/* set up command for bss segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, execp->a_bss,
	    epp->ep_daddr + execp->a_data, NULLVP, 0,
	    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	return exec_aout_setup_stack(p, epp);
}

/*
 * exec_aout_prep_nmagic(): Prepare Linux NMAGIC package.
 * Not different from the normal stuff.
 */

int
exec_linux_aout_prep_nmagic(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	struct exec *execp = epp->ep_hdr;
	long bsize, baddr;

	epp->ep_taddr = LINUX_N_TXTADDR(*execp, NMAGIC);
	epp->ep_tsize = execp->a_text;
	epp->ep_daddr = LINUX_N_DATADDR(*execp, NMAGIC);
	epp->ep_dsize = execp->a_data + execp->a_bss;
	epp->ep_entry = execp->a_entry;

	/* set up command for text segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn, execp->a_text,
	    epp->ep_taddr, epp->ep_vp, LINUX_N_TXTOFF(*execp, NMAGIC),
	    VM_PROT_READ|VM_PROT_EXECUTE);

	/* set up command for data segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn, execp->a_data,
	    epp->ep_daddr, epp->ep_vp, LINUX_N_DATOFF(*execp, NMAGIC),
	    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	/* set up command for bss segment */
	baddr = roundup(epp->ep_daddr + execp->a_data, NBPG);
	bsize = epp->ep_daddr + epp->ep_dsize - baddr;
	if (bsize > 0)
		NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, bsize, baddr,
		    NULLVP, 0, VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	return exec_aout_setup_stack(p, epp);
}

/*
 * exec_aout_prep_omagic(): Prepare Linux OMAGIC package.
 * Business as usual.
 */

int
exec_linux_aout_prep_omagic(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	struct exec *execp = epp->ep_hdr;
	long dsize, bsize, baddr;

	epp->ep_taddr = LINUX_N_TXTADDR(*execp, OMAGIC);
	epp->ep_tsize = execp->a_text;
	epp->ep_daddr = LINUX_N_DATADDR(*execp, OMAGIC);
	epp->ep_dsize = execp->a_data + execp->a_bss;
	epp->ep_entry = execp->a_entry;

	/* set up command for text and data segments */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn,
	    execp->a_text + execp->a_data, epp->ep_taddr, epp->ep_vp,
	    LINUX_N_TXTOFF(*execp, OMAGIC), VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	/* set up command for bss segment */
	baddr = roundup(epp->ep_daddr + execp->a_data, NBPG);
	bsize = epp->ep_daddr + epp->ep_dsize - baddr;
	if (bsize > 0)
		NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, bsize, baddr,
		    NULLVP, 0, VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	/*
	 * Make sure (# of pages) mapped above equals (vm_tsize + vm_dsize);
	 * obreak(2) relies on this fact. Both `vm_tsize' and `vm_dsize' are
	 * computed (in execve(2)) by rounding *up* `ep_tsize' and `ep_dsize'
	 * respectively to page boundaries.
	 * Compensate `ep_dsize' for the amount of data covered by the last
	 * text page. 
	 */
	dsize = epp->ep_dsize + execp->a_text - roundup(execp->a_text, NBPG);
	epp->ep_dsize = (dsize > 0) ? dsize : 0;
	return exec_aout_setup_stack(p, epp);
}

int
exec_linux_aout_prep_qmagic(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	struct exec *execp = epp->ep_hdr;

	epp->ep_taddr = LINUX_N_TXTADDR(*execp, QMAGIC);
	epp->ep_tsize = execp->a_text;
	epp->ep_daddr = LINUX_N_DATADDR(*execp, QMAGIC);
	epp->ep_dsize = execp->a_data + execp->a_bss;
	epp->ep_entry = execp->a_entry;

	/*
	 * check if vnode is in open for writing, because we want to
	 * demand-page out of it.  if it is, don't do it, for various
	 * reasons
	 */
	if ((execp->a_text != 0 || execp->a_data != 0) &&
	    epp->ep_vp->v_writecount != 0) {
#ifdef DIAGNOSTIC
		if (epp->ep_vp->v_flag & VTEXT)
			panic("exec: a VTEXT vnode has writecount != 0\n");
#endif
		return ETXTBSY;
	}
	epp->ep_vp->v_flag |= VTEXT;

	/* set up command for text segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_pagedvn, execp->a_text,
	    epp->ep_taddr, epp->ep_vp, LINUX_N_TXTOFF(*execp, QMAGIC),
	    VM_PROT_READ|VM_PROT_EXECUTE);

	/* set up command for data segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_pagedvn, execp->a_data,
	    epp->ep_daddr, epp->ep_vp, LINUX_N_DATOFF(*execp, QMAGIC),
	    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	/* set up command for bss segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, execp->a_bss,
	    epp->ep_daddr + execp->a_data, NULLVP, 0,
	    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	return exec_aout_setup_stack(p, epp);
}

/*
 * linux_elf_check_header():
 *
 * Check header for validity; return 0 of ok ENOEXEC if error
 */
static int
linux_elf_check_header(eh, type)
	Elf32_Ehdr     *eh;
	int             type;
{
#ifdef sparc
  /* #$%@#$%@#$%! */
# define memcmp bcmp
#endif
	if (memcmp(eh->e_ident, Elf32_e_ident, Elf32_e_siz) != 0) {
		DPRINTF(("Not an elf file\n"));
		return ENOEXEC;
	}

	switch (eh->e_machine) {
#ifdef i386
	case Elf32_em_386:
	case Elf32_em_486:
#endif
#ifdef sparc
	case Elf32_em_sparc:
#endif
		break;

	default:
		DPRINTF(("Unsupported elf machine type %d\n", eh->e_machine));
		return ENOEXEC;
	}

	if (eh->e_type != type) {
		DPRINTF(("Not an elf executable\n"));
		return ENOEXEC;
	}

	return 0;
}


/*
 * linux_elf_load_psection():
 * 
 * Load a psection at the appropriate address
 */
static void
linux_elf_load_psection(vcset, vp, ph, addr, size, prot)
	struct exec_vmcmd_set   *vcset;
	struct vnode		*vp;
	Elf32_Phdr		*ph;
	u_long			*addr;
	u_long			*size;
	int			*prot;
{
	u_long	uaddr;
	long	diff;
	long	offset;
	u_long	msize;

	/*
         * If the user specified an address, then we load there.
         */
	if (*addr != ~0) {
		uaddr = *addr + ph->p_align;
		*addr = LINUX_ELF_ALIGN(uaddr, ph->p_align);
		uaddr = LINUX_ELF_ALIGN(ph->p_vaddr, ph->p_align);
		diff = ph->p_vaddr - uaddr;
	} else {
		uaddr = ph->p_vaddr;
		*addr = LINUX_ELF_ALIGN(uaddr, ph->p_align);
		diff = uaddr - *addr;
	}

	*prot |= (ph->p_flags & Elf32_pf_r) ? VM_PROT_READ : 0;
	*prot |= (ph->p_flags & Elf32_pf_w) ? VM_PROT_WRITE : 0;
	*prot |= (ph->p_flags & Elf32_pf_x) ? VM_PROT_EXECUTE : 0;

	offset = ph->p_offset - diff;
	*size = ph->p_filesz + diff;
	msize = ph->p_memsz + diff;

	DPRINTF(("Elf Seg@ 0x%x/0x%x sz %d/%d off 0x%x/0x%x[%d] algn 0x%x\n",
		 ph->p_vaddr, *addr, *size, msize, ph->p_offset, offset,
		 diff, ph->p_align));

	NEW_VMCMD(vcset, vmcmd_map_readvn, *size,
		  *addr, vp, offset, *prot);

	/*
         * Check if we need to extend the size of the segment
         */
	{
		u_long	rm = round_page(*addr + msize);
		u_long	rf = round_page(*addr + *size);
		if (rm != rf) {
			DPRINTF(("zeropad 0x%x-0x%x\n", rf, rm));
			NEW_VMCMD(vcset, vmcmd_map_zero, rm - rf,
				  rf, NULLVP, 0, *prot);
			*size = msize;
		}
	}
}


/*
 * linux_elf_set_segment():
 *
 * Decide if the segment is text or data, depending on the protection
 * and set it appropriately
 */
static int
linux_elf_set_segment(epp, vaddr, size, prot)
	struct exec_package	*epp;
	u_long			 vaddr;
	u_long			 size;
	int			 prot;
{
	/*
         * Kludge: Unfortunately the current implementation of
         * exec package assumes a single text and data segment.
         * In Elf we can have more, but here we limit ourselves
         * to two and hope :-(
         * We also assume that the text is r-x, and data is rwx.
         */
	switch (prot) {
	case (VM_PROT_READ | VM_PROT_EXECUTE):
		if (epp->ep_tsize != ~0) {
			DPRINTF(("More than one text segment\n"));
			return ENOEXEC;
		}
		epp->ep_taddr = vaddr;
		epp->ep_tsize = size;
		DPRINTF(("Elf Text@ 0x%x, size %d\n", vaddr, size));
		break;

	case (VM_PROT_READ | VM_PROT_WRITE):
	case (VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE):
		if (epp->ep_dsize != ~0) {
			DPRINTF(("More than one data segment\n"));
			return ENOEXEC;
		}
		epp->ep_daddr = vaddr;
		epp->ep_dsize = size;

		DPRINTF(("Elf Data@ 0x%x, size %d\n", vaddr, size));
		break;

	default:
		DPRINTF(("Bad protection 0%o\n", prot));
		return ENOEXEC;
	}
	return 0;
}


/*
 * linux_elf_read_from():
 *
 *	Read from vnode into buffer at offset.
 */
static int
linux_elf_read_from(vp, off, p, buf, size)
	struct vnode	*vp;
	u_long		 off;
	struct proc	*p;
	caddr_t		 buf;
	int		 size;
{
	int	error;
	int	resid;

	DPRINTF(("read from 0x%x to 0x%x size %d\n",
		 off, buf, size));
	if ((error = vn_rdwr(UIO_READ, vp, buf, size,
			     off, UIO_SYSSPACE, IO_NODELOCKED, p->p_ucred,
			     &resid, p)) != 0) {
		DPRINTF(("Bad read error %d\n", error));
		return error;
	}
	/*
         * See if we got all of it
         */
	if (resid != 0) {
		DPRINTF(("Incomplete read for header ask=%d, rem=%d\n",
			 size, resid));
		return error;
	}
	return 0;
}


/*
 * linux_elf_load_file():
 *
 * Load a file (interpreter/library) pointed to by path
 * [stolen from coff_load_shlib()]. Made slightly more generic than
 * the svr4 version, for possible later use in linux_uselib().
 */
static int
linux_elf_load_file(p, path, vcset, entry, ap, last)
	struct proc		*p;
	char			*path;
	struct exec_vmcmd_set   *vcset;
	u_long			*entry;
	struct elf_args		*ap;
	u_long			*last;
{
	int			 error, i;
	struct nameidata	 nd;
	Elf32_Ehdr		 eh;
	Elf32_Phdr		*ph = NULL;
	u_long			 phsize;
	char			*bp = NULL;
	u_long			 addr = *last;

	DPRINTF(("Loading file %s @ %x\n", path, addr));

	if ((error = linux_emul_find(p, NULL, linux_emul_path, path, &bp, 0)) != 0)
		bp = NULL;
	else
		path = bp;
	/*
         * 1. open file
         * 2. read filehdr
         * 3. map text, data, and bss out of it using VM_*
         */
	NDINIT(&nd, LOOKUP, FOLLOW, UIO_SYSSPACE, path, p);
	/* first get the vnode */
	if ((error = namei(&nd)) != 0) {
		if (bp != NULL)
			free((char *) bp, M_TEMP);
		return error;
	}
	if ((error = linux_elf_read_from(nd.ni_vp, 0, p, (caddr_t) &eh,
				    sizeof(eh))) != 0)
		goto bad;

#ifdef DEBUG_EXEC_LINUX_ELF
	print_Ehdr(&eh);
#endif

	if ((error = linux_elf_check_header(&eh, Elf32_et_dyn)) != 0)
		goto bad;

	phsize = eh.e_phnum * sizeof(Elf32_Phdr);
	ph = (Elf32_Phdr *) malloc(phsize, M_TEMP, M_WAITOK);

	if ((error = linux_elf_read_from(nd.ni_vp, eh.e_phoff, p,
				    (caddr_t) ph, phsize)) != 0)
		goto bad;

	/*
         * Load all the necessary sections
         */
	for (i = 0; i < eh.e_phnum; i++) {
		u_long	size = 0;
		int	prot = 0;
#ifdef DEBUG_EXEC_LINUX_ELF
		print_Phdr(&ph[i]);
#endif

		switch (ph[i].p_type) {
		case Elf32_pt_load:
			linux_elf_load_psection(vcset, nd.ni_vp, &ph[i], &addr,
						&size, &prot);
			/* Assume that the text segment is r-x only */
			if ((prot & PROT_WRITE) == 0) {
				*entry = addr + eh.e_entry;
				ap->arg_interp = addr;
				DPRINTF(("Interpreter@ 0x%x\n", addr));
			}
			addr += size;
			break;

		case Elf32_pt_dynamic:
		case Elf32_pt_phdr:
		case Elf32_pt_note:
			break;

		default:
			DPRINTF(("interp: Unexpected program header type %d\n",
				 ph[i].p_type));
			break;
		}
	}

bad:
	if (ph != NULL)
		free((char *) ph, M_TEMP);
	if (bp != NULL)
		free((char *) bp, M_TEMP);

	*last = addr;
	vrele(nd.ni_vp);
	return error;
}


/*
 * exec_linux_elf_makecmds(): Prepare an Elf binary's exec package
 *
 * First, set of the various offsets/lengths in the exec package.
 *
 * Then, mark the text image busy (so it can be demand paged) or error
 * out if this is not possible.  Finally, set up vmcmds for the
 * text, data, bss, and stack segments.
 */
int
exec_linux_elf_makecmds(p, epp)
	struct proc		*p;
	struct exec_package	*epp;
{
	Elf32_Ehdr     *eh = epp->ep_hdr;
	Elf32_Phdr     *ph, *pp;
	int             error;
	int             i;
	char            interp[MAXPATHLEN];
	u_long          pos = 0;
	u_long          phsize;

#ifdef DEBUG_EXEC_LINUX_ELF
	print_Ehdr(eh);
#endif
	if (epp->ep_hdrvalid < sizeof(Elf32_Ehdr))
		return ENOEXEC;

	if (linux_elf_check_header(eh, Elf32_et_exec))
		return ENOEXEC;

	/*
         * check if vnode is in open for writing, because we want to
         * demand-page out of it.  if it is, don't do it, for various
         * reasons
         */
	if (epp->ep_vp->v_writecount != 0) {
#ifdef DIAGNOSTIC
		if (epp->ep_vp->v_flag & VTEXT)
			panic("exec: a VTEXT vnode has writecount != 0\n");
#endif
		return ETXTBSY;
	}
	/*
         * Allocate space to hold all the program headers, and read them
         * from the file
         */
	phsize = eh->e_phnum * sizeof(Elf32_Phdr);
	ph = (Elf32_Phdr *) malloc(phsize, M_TEMP, M_WAITOK);

	if ((error = linux_elf_read_from(epp->ep_vp, eh->e_phoff, p,
				    (caddr_t) ph, phsize)) != 0)
		goto bad;

	epp->ep_tsize = ~0;
	epp->ep_dsize = ~0;

	interp[0] = '\0';

	/*
         * Load all the necessary sections
         */
	for (i = 0; i < eh->e_phnum; i++) {
		u_long          addr = ~0, size = 0;
		int             prot = 0;

		pp = &ph[i];
#ifdef DEBUG_EXEC_LINUX_ELF
		print_Phdr(pp);
#endif

		switch (ph[i].p_type) {
		case Elf32_pt_load:
			linux_elf_load_psection(&epp->ep_vmcmds, epp->ep_vp,
				&ph[i], &addr, &size, &prot);
			if ((error = linux_elf_set_segment(epp, addr, size,
						      prot)) != 0)
				goto bad;
			break;

		case Elf32_pt_shlib:
			DPRINTF(("No support for COFF libraries (yet)\n"));
			error = ENOEXEC;
			goto bad;

		case Elf32_pt_interp:
			if (pp->p_filesz >= sizeof(interp)) {
				DPRINTF(("Interpreter path too long %d\n",
					 pp->p_filesz));
				goto bad;
			}
			if ((error = linux_elf_read_from(epp->ep_vp, pp->p_offset, p,
				      (caddr_t) interp, pp->p_filesz)) != 0)
				goto bad;
			break;

		case Elf32_pt_dynamic:
		case Elf32_pt_phdr:
		case Elf32_pt_note:
			break;

		default:
			/*
			 * Not fatal, we don't need to understand everything
			 * :-)
			 */
			DPRINTF(("Unsupported program header type %d\n",
				 pp->p_type));
			break;
		}
	}

	/*
         * Check if we found a dynamically linked binary and arrange to load
         * it's interpreter
         */
	if (interp[0]) {
		struct elf_args *ap;
		pos = ~0;

		ap = (struct elf_args *) malloc(sizeof(struct elf_args),
						 M_TEMP, M_WAITOK);
		if ((error = linux_elf_load_file(p, interp, &epp->ep_vmcmds,
				&epp->ep_entry, ap, &pos)) != 0) {
			free((char *) ap, M_TEMP);
			goto bad;
		}
		/* Arrange to load the program headers. */
		pos = LINUX_ELF_ALIGN(pos + NBPG, NBPG);
		DPRINTF(("Program header @0x%x\n", pos));
		ap->arg_phaddr = pos;
		NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn, phsize,
			  pos, epp->ep_vp, eh->e_phoff,
			  VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE);
		pos += phsize;

		ap->arg_phentsize = eh->e_phentsize;
		ap->arg_phnum = eh->e_phnum;
		ap->arg_entry = eh->e_entry;

		epp->ep_emul_arg = ap;
	} else
		epp->ep_entry = eh->e_entry;

	DPRINTF(("taddr 0x%x tsize 0x%x daddr 0x%x dsize 0x%x\n",
	       epp->ep_taddr, epp->ep_tsize, epp->ep_daddr, epp->ep_dsize));

	free((char *) ph, M_TEMP);

	DPRINTF(("Elf entry@ 0x%x\n", epp->ep_entry));
	epp->ep_vp->v_flag |= VTEXT;

	epp->ep_emul = &emul_linux_elf;

	return exec_aout_setup_stack(p, epp);

bad:
	free((char *) ph, M_TEMP);
	kill_vmcmds(&epp->ep_vmcmds);
	return ENOEXEC;
}
/*
 * The Linux system call to load shared libraries, a.out version. The
 * a.out shared libs are just files that are mapped onto a fixed
 * address in the process' address space. The address is given in
 * a_entry. Read in the header, set up some VM commands and run them.
 *
 * Yes, both text and data are mapped at once, so we're left with
 * writeable text for the shared libs. The Linux crt0 seemed to break
 * sometimes when data was mapped seperately. It munmapped a uselib()
 * of ld.so by hand, which failed with shared text and data for ld.so
 * Yuck.
 *
 * Because of the problem with ZMAGIC executables (text starts
 * at 0x400 in the file, but needs to be mapped at 0), ZMAGIC
 * shared libs are not handled very efficiently :-(
 */

int
linux_uselib(p, uap, retval)
	struct proc *p;
	struct linux_uselib_args /* {
		syscallarg(char *) path;
	} */ *uap;
	register_t *retval;
{
	caddr_t sg;
	long bsize, dsize, tsize, taddr, baddr, daddr;
	struct nameidata ni;
	struct vnode *vp;
	struct exec hdr;
	struct exec_vmcmd_set vcset;
	int rem, i, magic, error;

	sg = stackgap_init();
	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	NDINIT(&ni, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, path), p);

	if ((error = namei(&ni)))
		return error;

	vp = ni.ni_vp;

	if ((error = vn_rdwr(UIO_READ, vp, (caddr_t) &hdr, LINUX_AOUT_HDR_SIZE,
			     0, UIO_SYSSPACE, IO_NODELOCKED, p->p_ucred,
			     &rem, p))) {
		vrele(vp);
		return error;
	}

	if (rem != 0) {
		vrele(vp);
		return ENOEXEC;
	}

	if (LINUX_N_MACHTYPE(&hdr) != LINUX_MID_MACHINE)
		return ENOEXEC;

	magic = LINUX_N_MAGIC(&hdr);
	taddr = hdr.a_entry & (~(NBPG - 1));
	tsize = hdr.a_text;
	daddr = taddr + tsize;
	dsize = hdr.a_data + hdr.a_bss;

	if ((hdr.a_text != 0 || hdr.a_data != 0) && vp->v_writecount != 0) {
		vrele(vp);
                return ETXTBSY;
        }
	vp->v_flag |= VTEXT;

	vcset.evs_cnt = 0;
	vcset.evs_used = 0;

	NEW_VMCMD(&vcset,
		  magic == ZMAGIC ? vmcmd_map_readvn : vmcmd_map_pagedvn,
		  hdr.a_text + hdr.a_data, taddr,
		  vp, LINUX_N_TXTOFF(hdr, magic),
		  VM_PROT_READ|VM_PROT_EXECUTE|VM_PROT_WRITE);

	baddr = roundup(daddr + hdr.a_data, NBPG);
	bsize = daddr + dsize - baddr;
        if (bsize > 0) {
                NEW_VMCMD(&vcset, vmcmd_map_zero, bsize, baddr,
                    NULLVP, 0, VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);
	}

	for (i = 0; i < vcset.evs_used && !error; i++) {
		struct exec_vmcmd *vcp;

		vcp = &vcset.evs_cmds[i];
		error = (*vcp->ev_proc)(p, vcp);
	}

	kill_vmcmds(&vcset);

	vrele(vp);

	return error;
}

/*
 * Execve(2). Just check the alternate emulation path, and pass it on
 * to the NetBSD execve().
 */
int
linux_execve(p, uap, retval)
	struct proc *p;
	struct linux_execve_args /* {
		syscallarg(char *) path;
		syscallarg(char **) argv;
		syscallarg(char **) envp;
	} */ *uap;
	register_t *retval;
{
	caddr_t sg;

	sg = stackgap_init();
	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	return execve(p, uap, retval);
}
