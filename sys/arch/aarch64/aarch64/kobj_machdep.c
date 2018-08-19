/*	$NetBSD: kobj_machdep.c,v 1.2 2018/08/19 20:02:22 ryo Exp $	*/

/*
 * Copyright (c) 2018 Ryo Shimizu <ryo@nerv.org>
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kobj_machdep.c,v 1.2 2018/08/19 20:02:22 ryo Exp $");

#define ELFSIZE		ARCH_ELFSIZE

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/kobj.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>
#include <sys/errno.h>
#include <sys/queue.h>
#include <sys/tree.h>

#include <aarch64/cpufunc.h>

/* #define KOBJ_MACHDEP_DEBUG */

#ifdef KOBJ_MACHDEP_DEBUG
#ifdef DDB
#include <aarch64/db_machdep.h>	/* for strdisasm() */
#endif

struct rtypeinfo {
	Elf_Word rtype;
	const char *name;
};

static const struct rtypeinfo rtypetbl[] = {
	{ R_AARCH64_ABS64,		"R_AARCH64_ABS64"		},
	{ R_AARCH64_ADD_ABS_LO12_NC,	"R_AARCH64_ADD_ABS_LO12_NC"	},
	{ R_AARCH_LDST64_ABS_LO12_NC,	"R_AARCH64_LDST64_ABS_LO12_NC"	},
	{ R_AARCH_LDST32_ABS_LO12_NC,	"R_AARCH64_LDST32_ABS_LO12_NC"	},
	{ R_AARCH_LDST16_ABS_LO12_NC,	"R_AARCH64_LDST16_ABS_LO12_NC"	},
	{ R_AARCH64_LDST8_ABS_LO12_NC,	"R_AARCH64_LDST8_ABS_LO12_NC"	},
	{ R_AARCH64_ADR_PREL_PG_HI21_NC, "R_AARCH64_ADR_PREL_PG_HI21_NC"},
	{ R_AARCH64_ADR_PREL_PG_HI21,	"R_AARCH64_ADR_PREL_PG_HI21"	},
	{ R_AARCH_JUMP26,		"R_AARCH64_JUMP26"		},
	{ R_AARCH_CALL26,		"R_AARCH64_CALL26"		},
	{ R_AARCH64_PREL32,		"R_AARCH64_PREL32"		},
	{ R_AARCH64_PREL16,		"R_AARCH64_PREL16"		}
};

static const char *
strrtype(Elf_Word rtype)
{
	int i;
	static char buf[64];

	for (i = 0; i < __arraycount(rtypetbl); i++) {
		if (rtypetbl[i].rtype == rtype)
			return rtypetbl[i].name;
	}
	snprintf(buf, sizeof(buf), "RELOCATION-TYPE-%d", rtype);
	return buf;
}
#endif /* KOBJ_MACHDEP_DEBUG */

static inline bool
checkalign(Elf_Addr addr, int alignbyte, void *where, Elf64_Addr off)
{
	if ((addr & (alignbyte - 1)) != 0) {
		printf("kobj_reloc: Relocation 0x%jx unaligned at %p"
		    " (base+0x%jx). must be aligned %d\n",
		    (uintptr_t)addr, where, off, alignbyte);
		return true;
	}
	return false;
}

static inline bool
checkoverflow(Elf_Addr addr, int bitwidth, Elf_Addr targetaddr,
    const char *bitscale, void *where, Elf64_Addr off)
{
	const Elf_Addr mask = ~__BITS(bitwidth - 1, 0);

	if (((addr & mask) != 0) && ((addr & mask) != mask)) {
		printf("kobj_reloc: Relocation 0x%jx too far from %p"
		    " (base+0x%jx) for %dbit%s\n",
		    (uintptr_t)targetaddr, where, off, bitwidth, bitscale);
		return true;
	}
	return false;
}

#define WIDTHMASK(w)	(0xffffffffffffffffUL >> (64 - (w)))

int
kobj_reloc(kobj_t ko, uintptr_t relocbase, const void *data,
    bool isrela, bool local)
{
	Elf_Addr saddr, addend, raddr, val;
	Elf64_Addr off, *where;
	Elf32_Addr *where32;
	uint16_t *where16;
	Elf_Word rtype, symidx;
	const Elf_Rela *rela;
	int error;
	uint32_t *insn, immhi, immlo, shift;
	bool nc = false;
#ifdef KOBJ_MACHDEP_DEBUG
#ifdef DDB
	char disasmbuf[256];
#endif
	Elf_Addr old;
#endif /* KOBJ_MACHDEP_DEBUG */


#ifdef KOBJ_MACHDEP_DEBUG
	printf("%s:%d: ko=%p, relocbase=0x%jx, data=%p"
	    ", isrela=%d, local=%d\n", __func__, __LINE__,
	    ko, relocbase, data, isrela, local);
#endif /* KOBJ_MACHDEP_DEBUG */

	if (!isrela) {
		printf("kobj_reloc: REL relocations not supported");
		error = 1;
		goto done;
	}

	rela = (const Elf_Rela *)data;
	addend = rela->r_addend;
	rtype = ELF_R_TYPE(rela->r_info);
	symidx = ELF_R_SYM(rela->r_info);
	off = rela->r_offset;
	where = (Elf_Addr *)(relocbase + off);

	/* pointer to 32bit, 16bit, and instruction */
	where32 = (void *)where;
	where16 = (void *)where;
	insn = (uint32_t *)where;

	/* no need to lookup any symbols */
	switch (rtype) {
	case R_AARCH64_NONE:
	case R_AARCH64_NONE2:
		return 0;
	}

	error = kobj_sym_lookup(ko, symidx, &saddr);
	if (error != 0) {
		printf("kobj_reloc: symidx %d lookup failure."
		    " relocation type %d at %p (base+0x%jx)\n",
		    symidx, rtype, where, off);
		goto done;
	}

#ifdef KOBJ_MACHDEP_DEBUG
	printf("%s:%d: symidx=%d, saddr=0x%jx, addend=0x%jx\n",
	    __func__, __LINE__, symidx, (uintptr_t)saddr, (uintptr_t)addend);
	printf("%s:%d: rtype=%s, where=%p (base+0x%jx)\n",
	    __func__, __LINE__, strrtype(rtype), where, off);
	old = *where;
#ifdef DDB
	snprintf(disasmbuf, sizeof(disasmbuf), "%08x %s",
	    *insn, strdisasm((vaddr_t)insn));
#endif
#endif /* KOBJ_MACHDEP_DEBUG */

	switch (rtype) {
	case R_AARCH64_ABS64:
		/*
		 * S + A
		 *  e.g.) .quad <sym>+addend
		 */
		*where = saddr + addend;
		break;
	case R_AARCH64_ABS32:
		/*
		 * S + A
		 *  e.g.) .word <sym>+addend
		 */
		*where32 = saddr + addend;
		break;
	case R_AARCH64_ABS16:
		/*
		 * S + A
		 *  e.g.) .short <sym>+addend
		 */
		*where16 = saddr + addend;
		break;
	case R_AARCH64_ADD_ABS_LO12_NC:
	case R_AARCH64_LDST8_ABS_LO12_NC:
	case R_AARCH_LDST16_ABS_LO12_NC:
	case R_AARCH_LDST32_ABS_LO12_NC:
	case R_AARCH_LDST64_ABS_LO12_NC:
		switch (rtype) {
		case R_AARCH64_ADD_ABS_LO12_NC:
		case R_AARCH64_LDST8_ABS_LO12_NC:
			shift = 0;
			break;
		case R_AARCH_LDST16_ABS_LO12_NC:
			shift = 1;
			break;
		case R_AARCH_LDST32_ABS_LO12_NC:
			shift = 2;
			break;
		case R_AARCH_LDST64_ABS_LO12_NC:
			shift = 3;
			break;
		default:
			panic("illegal rtype: %d\n", rtype);
		}
		/*
		 * S + A
		 *  e.g.) add  x0,x0,#:lo12:<sym>+<addend>
		 *        ldrb w0,[x0,#:lo12:<sym>+<addend>]
		 *        ldrh w0,[x0,#:lo12:<sym>+<addend>]
		 *        ldr  w0,[x0,#:lo12:<sym>+<addend>]
		 *        ldr  x0,[x0,#:lo12:<sym>+<addend>]
		 */
		val = saddr + addend;
		if (checkalign(val, 1 << shift, where, off)) {
			error = 1;
			break;
		}
		val &= WIDTHMASK(12);
		val >>= shift;
		*insn = (*insn & ~__BITS(21,10)) | (val << 10);
		break;

	case R_AARCH64_ADR_PREL_PG_HI21_NC:
		nc = true;
		/* FALLTHRU */
	case R_AARCH64_ADR_PREL_PG_HI21:
		/*
		 * Page(S + A) - Page(P)
		 *  e.g.) adrp x0,<sym>+<addend>
		 */
		val = saddr + addend;
		val = val >> 12;
		raddr = val << 12;
		val -= (uintptr_t)where >> 12;
		if (!nc && checkoverflow(val, 21, raddr, " x 4k", where, off)) {
			error = 1;
			break;
		}
		immlo = val & WIDTHMASK(2);
		immhi = (val >> 2) & WIDTHMASK(19);
		*insn = (*insn & ~(__BITS(30,29) | __BITS(23,5))) |
		    (immlo << 29) | (immhi << 5);
		break;

	case R_AARCH_JUMP26:
	case R_AARCH_CALL26:
		/*
		 * S + A - P
		 *  e.g.) b <sym>+<addend>
		 *        bl <sym>+<addend>
		 */
		raddr = saddr + addend;
		val = raddr - (uintptr_t)where;
		if (checkalign(val, 4, where, off)) {
			error = 1;
			break;
		}
		val = (intptr_t)val >> 2;
		if (checkoverflow(val, 26, raddr, " word", where, off)) {
			error = 1;
			break;
		}
		val &= WIDTHMASK(26);
		*insn = (*insn & ~__BITS(25,0)) | val;
		break;

	case R_AARCH64_PREL64:
		/*
		 * S + A - P
		 *  e.g.) 1: .quad <sym>+<addend>-1b
		 */
		raddr = saddr + addend;
		val = raddr - (uintptr_t)where;
		if (checkoverflow(val, 64, raddr, "", where, off)) {
			error = 1;
			break;
		}
		*where = val;
		break;
	case R_AARCH64_PREL32:
		/*
		 * S + A - P
		 *  e.g.) 1: .word <sym>+<addend>-1b
		 */
		raddr = saddr + addend;
		val = raddr - (uintptr_t)where;
		if (checkoverflow(val, 32, raddr, "", where, off)) {
			error = 1;
			break;
		}
		*where32 = val;
		break;
	case R_AARCH64_PREL16:
		/*
		 * S + A - P
		 *  e.g.) 1: .short <sym>+<addend>-1b
		 */
		raddr = saddr + addend;
		val = raddr - (uintptr_t)where;
		if (checkoverflow(val, 16, raddr, "", where, off)) {
			error = 1;
			break;
		}
		*where16 = val;
		break;
	default:
		printf("kobj_reloc: unsupported relocation type %d"
		    " at %p (base+0x%jx) symidx %u\n",
		    rtype, where, off, symidx);
		error = 1;
		break;
	}

#ifdef KOBJ_MACHDEP_DEBUG
	printf("%s: reloc\n", __func__);
	printf("%s:  *where %016jx\n", __func__, (uintptr_t)old);
	printf("%s:      -> %016jx\n", __func__, (uintptr_t)*where);
#ifdef DDB
	printf("%s:    insn %s\n", __func__, disasmbuf);
	printf("%s:      -> %08x %s\n", __func__,
	    *insn, strdisasm((vaddr_t)insn));
#endif
	printf("\n");
#endif /* KOBJ_MACHDEP_DEBUG */

 done:
	if (error != 0)
		return -1;
	return 0;
}

int
kobj_machdep(kobj_t ko, void *base, size_t size, bool load)
{
	return 0;
}
