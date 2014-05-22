/*	$NetBSD: libelf_data.c,v 1.4.6.1 2014/05/22 15:48:04 yamt Exp $	*/

/*-
 * Copyright (c) 2006 Joseph Koshy
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if HAVE_NBTOOL_CONFIG_H
# include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
/* __FBSDID("$FreeBSD: src/lib/libelf/libelf_data.c,v 1.4.2.1.2.1 2009/10/25 01:10:29 kensmith Exp $"); */

#include <libelf.h>

#include "_libelf.h"

int
_libelf_xlate_shtype(uint32_t sht)
{
	switch (sht) {
	case SHT_DYNAMIC:
		return (ELF_T_DYN);
	case SHT_DYNSYM:
		return (ELF_T_SYM);
	case SHT_FINI_ARRAY:
		return (ELF_T_ADDR);
	case SHT_GROUP:
		return (ELF_T_WORD);
	case SHT_HASH:
		return (ELF_T_WORD);
	case SHT_INIT_ARRAY:
		return (ELF_T_ADDR);
	case SHT_NOBITS:
		return (ELF_T_BYTE);
	case SHT_NOTE:
		return (ELF_T_NOTE);
	case SHT_PREINIT_ARRAY:
		return (ELF_T_ADDR);
	case SHT_PROGBITS:
		return (ELF_T_BYTE);
	case SHT_REL:
		return (ELF_T_REL);
	case SHT_RELA:
		return (ELF_T_RELA);
	case SHT_STRTAB:
		return (ELF_T_BYTE);
	case SHT_SYMTAB:
		return (ELF_T_SYM);
	case SHT_SYMTAB_SHNDX:
		return (ELF_T_WORD);
#if defined(__LIBELF_HAVE_ELF_VERS)
	case SHT_GNU_verdef:	/* == SHT_SUNW_verdef */
		return (ELF_T_VDEF);
	case SHT_GNU_verneed:	/* == SHT_SUNW_verneed */
		return (ELF_T_VNEED);
	case SHT_GNU_versym:	/* == SHT_SUNW_versym */
		return (ELF_T_HALF);
#endif /* __LIBELF_HAVE_ELF_VERS */
#if defined(__LIBELF_HAVE_ELF_MOVE)
	case SHT_SUNW_move:
		return (ELF_T_MOVE);
#endif /* __LIBELF_HAVE_ELF_MOVE */
#if defined(__LIBELF_HAVE_ELF_SYMINFO)
	case SHT_SUNW_syminfo:
		return (ELF_T_SYMINFO);
#endif /* __LIBELF_HAVE_ELF_SYMINFO */
#if defined(__LIBELF_HAVE_ELF_ATTRIBUTES)
	case SHT_GNU_ATTRIBUTES:
		return (ELF_T_BYTE);	/* XXX */
#endif /* __LIBELF_HAVE_ELF_ATTRIBUTES */
	default:
		if (sht >= SHT_LOPROC && sht <= SHT_HIPROC)
			return (ELF_T_BYTE);
		return (-1);
	}
}
