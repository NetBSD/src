/* $NetBSD: rmreloc.c,v 1.1.8.2 2006/04/22 11:37:10 simonb Exp $ */

/*
 * Copyright 1996 John D. Polstra.
 * Copyright 1996 Matt Thomas <matt@3am-software.com>
 * Copyright 2002 Charles M. Hannum <root@ihack.net>
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
 * rmreloc.c - relocate an ELFish RISC OS relocatable module.
 */
/*
 * This code is a heavily hacked version of parts of:
 * lib/libexec/ld.elf_so/headers.c
 * lib/libexec/ld.elf_so/arch/arm/mdreloc.c
 *
 * At present it only deals with DT_REL tables containing R_ARM_NONE
 * and R_ARM_RELATIVE relocations, because those are all that my
 * linker emits.  More can be added as needed.  Note that this has to
 * handle relocating already-relocated code, e.g. after *RMTidy, so
 * most relocations have to reference oldbase, which ld.elf_so just
 * assumes is zero.  There may be a cleverer way to do this.
 */

#include <sys/types.h>
#include <sys/stdint.h>
#include <lib/libsa/stand.h>
#define ELFSIZE 32
#include <sys/exec_elf.h>

#include <riscoscalls.h>

os_error *relocate_self(Elf_Dyn *, caddr_t, caddr_t);

#define assert(x) /* nothing */

/*
 * While relocating ourselves, we must not refer to any global variables.
 * This includes _DYNAMIC -- the startup code finds it for us and passes
 * it to us along with the base address of the module.
 */

typedef struct {
	caddr_t         relocbase;	/* Reloc const = mapbase - *vaddrbase */
	Elf_Dyn        *dynamic;	/* Dynamic section */
	const Elf_Rel  *rel;		/* Relocation entries */
	const Elf_Rel  *rellim;		/* Limit of Relocation entries */
} Obj_Entry;

#define rdbg(x) /* nothing */

/*
 * It is possible for the compiler to emit relocations for unaligned data.
 * We handle this situation with these inlines.
 */
#define	RELOC_ALIGNED_P(x) \
	(((uintptr_t)(x) & (sizeof(void *) - 1)) == 0)

static inline Elf_Addr
load_ptr(void *where)
{
	Elf_Addr res;

	memcpy(&res, where, sizeof(res));

	return (res);
}

static inline void
store_ptr(void *where, Elf_Addr val)
{

	memcpy(where, &val, sizeof(val));
}

static struct os_error bad_reloc = {
	0, "Unhandled ELF redirection"
};

os_error *
relocate_self(Elf_Dyn *dynamic, caddr_t oldbase, caddr_t newbase)
{
	Elf_Dyn *dynp;
	Obj_Entry o, *obj; 
	const Elf_Rel *rel;
	Elf_Addr        relsz = 0;

	obj = &o; obj->dynamic = dynamic; obj->relocbase = newbase;

	for (dynp = obj->dynamic; dynp->d_tag != DT_NULL; ++dynp) {
		switch (dynp->d_tag) {
		case DT_REL:
			obj->rel = (const Elf_Rel *)
			    (obj->relocbase + dynp->d_un.d_ptr);
			break;
		case DT_RELSZ:
			relsz = dynp->d_un.d_val;
			break;
		case DT_RELENT:
			assert(dynp->d_un.d_val == sizeof(Elf_Rel));
			break;
		}
	}

	obj->rellim = (const Elf_Rel *)((caddr_t)obj->rel + relsz);

	for (rel = obj->rel; rel < obj->rellim; rel++) {
		Elf_Addr        *where;
		Elf_Addr         tmp;

		where = (Elf_Addr *)(obj->relocbase + rel->r_offset);

		switch (ELF_R_TYPE(rel->r_info)) {
		case R_TYPE(NONE):
			break;

		case R_TYPE(RELATIVE):	/* word32 B + A */
			if (__predict_true(RELOC_ALIGNED_P(where))) {
				tmp = *where + (Elf_Addr)obj->relocbase -
				    (Elf_Addr)oldbase;
				*where = tmp;
			} else {
				tmp = load_ptr(where) +
				    (Elf_Addr)obj->relocbase -
				    (Elf_Addr)oldbase;
				store_ptr(where, tmp);
			}
			rdbg(("RELATIVE in %s --> %p", obj->path,
			    (void *)tmp));
			break;

		default:
			return &bad_reloc;
		}
	}
	return NULL;
}
