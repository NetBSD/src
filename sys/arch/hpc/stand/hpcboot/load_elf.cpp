/*	$NetBSD: load_elf.cpp,v 1.1 2001/02/09 18:34:46 uch Exp $	*/

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

#include <load.h>
#include <load_elf.h>
#include <console.h>
#include <memory.h>
#include <file.h>

ElfLoader::ElfLoader(Console *&cons, MemoryManager *&mem)
	: Loader(cons, mem)
{
	DPRINTF((TEXT("Loader: ELF\n")));
}

ElfLoader::~ElfLoader(void)
{
}

BOOL
ElfLoader::setFile(File *&file)
{
	Loader::setFile(file);

	/* read ELF header and check it */
	if (!read_header())
		return FALSE;
	/* read program header */
	size_t sz = _eh.e_phnum * _eh.e_phentsize;

	return _file->read(_ph, sz, _eh.e_phoff) == sz;
}

size_t
ElfLoader::memorySize()
{
	int i;
	Elf_Phdr *ph = _ph;
	size_t sz = 0;

	DPRINTF((TEXT("file size: ")));
	for (i = 0; i < _eh.e_phnum; i++, ph++) {
		if (ph->p_type == PT_LOAD) {
			size_t filesz = ph->p_filesz;
			DPRINTF((TEXT("+0x%x"), filesz));
			sz += _mem->roundPage(filesz);
		}
	}
	DPRINTF((TEXT(" = 0x%x byte\n"), sz));
	return sz;
}

kaddr_t
ElfLoader::jumpAddr()
{
	DPRINTF((TEXT("kernel entry address: 0x%08x\n"), _eh.e_entry));
	return _eh.e_entry;
}

BOOL
ElfLoader::load()
{
	Elf_Phdr *ph;
	int i;
  
	_load_segment_start();

	for (i = 0, ph = _ph; i < _eh.e_phnum; i++, ph++) {
		if (ph->p_type == PT_LOAD) {
			size_t filesz = ph->p_filesz;
			size_t memsz = ph->p_memsz;
			vaddr_t kv = ph->p_vaddr;
			off_t fileofs = ph->p_offset;
			DPRINTF((TEXT("[%d] vaddr 0x%08x file size 0x%x mem size 0x%x\n"),
				 i, kv, filesz, memsz));
			_load_segment(kv, memsz, fileofs, filesz);
		}
	}
	/* tag chain still opening */

	return TRUE;
}

BOOL
ElfLoader::read_header(void)
{
	// read ELF header
	_file->read(&_eh, sizeof(Elf_Ehdr), 0);

	// check ELF Magic.
	if (!is_elf_file()) {
		DPRINTF((TEXT("not a ELF file.\n")));
		return FALSE;
	}
  
	// Windows CE is 32bit little-endian only.
	if (_eh.e_ident[EI_DATA] != ELFDATA2LSB ||
	    _eh.e_ident[EI_CLASS] != ELFCLASS32) {
		DPRINTF((TEXT("invalid class/data(%d/%d)\n"),
			 _eh.e_ident[EI_CLASS], _eh.e_ident[EI_DATA]));
		return FALSE;
	}

	// Is native architecture?
	switch(_eh.e_machine) {
		ELF32_MACHDEP_ID_CASES;
	default:
		DPRINTF((TEXT("not a native architecture. machine = %d\n"),
			 _eh.e_machine));
		return FALSE;
	}
  
	/* Check object type */
	if (_eh.e_type != ET_EXEC) {
		DPRINTF((TEXT("not a executable file. type = %d\n"),
			 _eh.e_type));
		return FALSE;
	}
  
	if (_eh.e_phoff == 0 || _eh.e_phnum == 0 || _eh.e_phnum > 16 ||
	    _eh.e_phentsize != sizeof(Elf_Phdr)) {
		DPRINTF((TEXT("invalid program header infomation.\n")));
		return FALSE;
	}

	return TRUE;
}
