/* -*-C++-*-	$NetBSD: load_elf.h,v 1.2 2001/03/21 14:06:25 toshii Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#ifndef _HPCBOOT_LOAD_ELF_H_
#define _HPCBOOT_LOAD_ELF_H_

#include <exec_elf.h>

class ElfLoader : public Loader {
private:
	Elf_Ehdr _eh;
	Elf_Phdr _ph[16];
	Elf_Shdr _sh[16];

	BOOL is_elf_file(void) {
		return
			_eh.e_ident[EI_MAG0] == ELFMAG0 &&
			_eh.e_ident[EI_MAG1] == ELFMAG1 &&
			_eh.e_ident[EI_MAG2] == ELFMAG2 &&
			_eh.e_ident[EI_MAG3] == ELFMAG3;
	}
	BOOL read_header(void);
	struct PageTag *load_page(vaddr_t, off_t, size_t, struct PageTag *);
			     
public:
	ElfLoader(Console *&, MemoryManager *&);
	virtual ~ElfLoader(void);
  
	BOOL setFile(File *&);
	size_t memorySize(void);
	BOOL load(void);
	kaddr_t jumpAddr(void);
};

#endif //_HPCBOOT_LOAD_ELF_H_
