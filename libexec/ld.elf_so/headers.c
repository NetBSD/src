/*	$NetBSD: headers.c,v 1.4 1999/03/01 16:40:07 christos Exp $	 */

/*
 * Copyright 1996 John D. Polstra.
 * Copyright 1996 Matt Thomas <matt@3am-software.com>
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
 *      This product includes software developed by John Polstra.
 * 4. The name of the author may not be used to endorse or promote products
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Dynamic linker for ELF.
 *
 * John Polstra <jdp@polstra.com>.
 */

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <dirent.h>

#include "debug.h"
#include "rtld.h"

/*
 * Process a shared object's DYNAMIC section, and save the important
 * information in its Obj_Entry structure.
 */
void
_rtld_digest_dynamic(obj)
	Obj_Entry *obj;
{
	Elf_Dyn        *dynp;
	Needed_Entry  **needed_tail = &obj->needed;
	const Elf_Dyn  *dyn_rpath = NULL;
	enum Elf_e_dynamic_type plttype = Elf_edt_rel;
	Elf_Word        relsz = 0, relasz = 0;
	Elf_Word	pltrelsz = 0, pltrelasz = 0;

	for (dynp = obj->dynamic; dynp->d_tag != Elf_edt_null; ++dynp) {
		switch (dynp->d_tag) {

		case Elf_edt_rel:
			obj->rel = (const Elf_Rel *)
			    (obj->relocbase + dynp->d_un.d_ptr);
			break;

		case Elf_edt_relsz:
			relsz = dynp->d_un.d_val;
			break;

		case Elf_edt_relent:
			assert(dynp->d_un.d_val == sizeof(Elf_Rel));
			break;

		case Elf_edt_jmprel:
			if (plttype == Elf_edt_rel) {
				obj->pltrel = (const Elf_Rel *)
				    (obj->relocbase + dynp->d_un.d_ptr);
			} else {
				obj->pltrela = (const Elf_RelA *)
				    (obj->relocbase + dynp->d_un.d_ptr);
			}
			break;

		case Elf_edt_pltrelsz:
			if (plttype == Elf_edt_rel) {
				pltrelsz = dynp->d_un.d_val;
			} else {
				pltrelasz = dynp->d_un.d_val;
			}
			break;

		case Elf_edt_rela:
			obj->rela = (const Elf_RelA *)
			    (obj->relocbase + dynp->d_un.d_ptr);
			break;

		case Elf_edt_relasz:
			relasz = dynp->d_un.d_val;
			break;

		case Elf_edt_relaent:
			assert(dynp->d_un.d_val == sizeof(Elf_RelA));
			break;

		case Elf_edt_pltrel:
			plttype = dynp->d_un.d_val;
			assert(plttype == Elf_edt_rel ||
			    plttype == Elf_edt_rela);
			if (plttype == Elf_edt_rela) {
				obj->pltrela = (const Elf_RelA *) obj->pltrel;
				obj->pltrel = NULL;
				pltrelasz = pltrelsz;
				pltrelsz = 0;
			}
			break;

		case Elf_edt_symtab:
			obj->symtab = (const Elf_Sym *)
				(obj->relocbase + dynp->d_un.d_ptr);
			break;

		case Elf_edt_syment:
			assert(dynp->d_un.d_val == sizeof(Elf_Sym));
			break;

		case Elf_edt_strtab:
			obj->strtab = (const char *)
			    (obj->relocbase + dynp->d_un.d_ptr);
			break;

		case Elf_edt_strsz:
			obj->strsize = dynp->d_un.d_val;
			break;

		case Elf_edt_hash:
			{
				const Elf_Word *hashtab = (const Elf_Word *)
				(obj->relocbase + dynp->d_un.d_ptr);

				obj->nbuckets = hashtab[0];
				obj->nchains = hashtab[1];
				obj->buckets = hashtab + 2;
				obj->chains = obj->buckets + obj->nbuckets;
			}
			break;

		case Elf_edt_needed:
			assert(!obj->rtld);
			{
				Needed_Entry *nep = NEW(Needed_Entry);

				nep->name = dynp->d_un.d_val;
				nep->obj = NULL;
				nep->next = NULL;

				*needed_tail = nep;
				needed_tail = &nep->next;
			}
			break;

		case Elf_edt_pltgot:
			obj->pltgot = (Elf_Addr *)
			    (obj->relocbase + dynp->d_un.d_ptr);
			break;

		case Elf_edt_textrel:
			obj->textrel = true;
			break;

		case Elf_edt_symbolic:
			obj->symbolic = true;
			break;

		case Elf_edt_rpath:
			/*
		         * We have to wait until later to process this, because
			 * we might not have gotten the address of the string
			 * table yet.
		         */
			dyn_rpath = dynp;
			break;

		case Elf_edt_soname:
			/* Not used by the dynamic linker. */
			break;

		case Elf_edt_init:
			obj->init = (void (*) __P((void)))
			    (obj->relocbase + dynp->d_un.d_ptr);
			break;

		case Elf_edt_fini:
			obj->fini = (void (*) __P((void)))
			    (obj->relocbase + dynp->d_un.d_ptr);
			break;

		case Elf_edt_debug:
#ifdef RTLD_LOADER
			dynp->d_un.d_ptr = (Elf_Addr)&_rtld_debug;
#endif
			break;

#if defined(__mips__)
		case DT_MIPS_LOCAL_GOTNO:
			obj->local_gotno = dynp->d_un.d_val;
			break;

		case DT_MIPS_SYMTABNO:
			obj->symtabno = dynp->d_un.d_val;
			break;

		case DT_MIPS_GOTSYM:
			obj->gotsym = dynp->d_un.d_val;
			break;

		case DT_MIPS_RLD_MAP:
#ifdef RTLD_LOADER
			*((Elf_Addr *)(dynp->d_un.d_ptr)) = (Elf_Addr)
			    &_rtld_debug;
#endif
			break;
#endif
		}
	}

	obj->rellim = (const Elf_Rel *)((caddr_t)obj->rel + relsz);
	obj->relalim = (const Elf_RelA *)((caddr_t)obj->rela + relasz);
	obj->pltrellim = (const Elf_Rel *)((caddr_t)obj->pltrel + pltrelsz);
	obj->pltrelalim = (const Elf_RelA *)((caddr_t)obj->pltrela + pltrelasz);

	if (dyn_rpath != NULL) {
		_rtld_add_paths(&obj->rpaths, obj->strtab +
		    dyn_rpath->d_un.d_val, true);
	}
}

/*
 * Process a shared object's program header.  This is used only for the
 * main program, when the kernel has already loaded the main program
 * into memory before calling the dynamic linker.  It creates and
 * returns an Obj_Entry structure.
 */
Obj_Entry *
_rtld_digest_phdr(phdr, phnum, entry)
	const Elf_Phdr *phdr;
	int phnum;
	caddr_t entry;
{
	Obj_Entry      *obj = CNEW(Obj_Entry);
	const Elf_Phdr *phlimit = phdr + phnum;
	const Elf_Phdr *ph;
	int             nsegs = 0;

	for (ph = phdr; ph < phlimit; ++ph) {
		switch (ph->p_type) {

		case Elf_pt_phdr:
			assert((const Elf_Phdr *) ph->p_vaddr == phdr);
			obj->phdr = (const Elf_Phdr *) ph->p_vaddr;
			obj->phsize = ph->p_memsz;
			break;

		case Elf_pt_load:
			assert(nsegs < 2);
			if (nsegs == 0) {	/* First load segment */
				obj->vaddrbase = round_down(ph->p_vaddr);
				obj->mapbase = (caddr_t) obj->vaddrbase;
				obj->relocbase = obj->mapbase - obj->vaddrbase;
				obj->textsize = round_up(ph->p_vaddr +
				    ph->p_memsz) - obj->vaddrbase;
			} else {		/* Last load segment */
				obj->mapsize = round_up(ph->p_vaddr +
				    ph->p_memsz) - obj->vaddrbase;
			}
			++nsegs;
			break;

		case Elf_pt_dynamic:
			obj->dynamic = (Elf_Dyn *) ph->p_vaddr;
			break;
		}
	}
	assert(nsegs == 2);

	obj->entry = entry;
	return obj;
}
