/*	$NetBSD: load_elf.cpp,v 1.3 2001/05/08 18:51:23 uch Exp $	*/

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
	size_t sz;
	Loader::setFile(file);

	/* read ELF header and check it */
	if (!read_header())
		return FALSE;
	/* read section header */
	sz = _eh.e_shnum * _eh.e_shentsize;
	_file->read(_sh, _eh.e_shentsize * _eh.e_shnum, _eh.e_shoff);

	/* read program header */
	sz = _eh.e_phnum * _eh.e_phentsize;

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
	/* XXX reserve 192kB for symbols */
	sz += 0x30000;

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
	Elf_Shdr *sh, *shstr, *shsym;
	off_t stroff = 0, symoff = 0, off;
	vaddr_t kv;

	size_t shstrsize;
	char buf[1024];
	int i;
  
	_load_segment_start();

	for (i = 0, ph = _ph; i < _eh.e_phnum; i++, ph++) {
		if (ph->p_type == PT_LOAD) {
			size_t filesz = ph->p_filesz;
			size_t memsz = ph->p_memsz;
			kv = ph->p_vaddr;
			off_t fileofs = ph->p_offset;
			DPRINTF((TEXT("[%d] vaddr 0x%08x file size 0x%x mem size 0x%x\n"),
			    i, kv, filesz, memsz));
			_load_segment(kv, memsz, fileofs, filesz);
			kv += memsz;
		}
	}

	/*
	 * Prepare ELF headers for symbol table.
	 *
	 *   ELF header
	 *   section header
	 *   shstrtab
	 *   strtab
	 *   symtab
	 */
	memcpy(buf, &_eh, sizeof(_eh));
	((Elf_Ehdr *)buf)->e_phoff = 0;
	((Elf_Ehdr *)buf)->e_phnum = 0;
	((Elf_Ehdr *)buf)->e_entry = 0;
	((Elf_Ehdr *)buf)->e_shoff = sizeof(_eh);
	off = ((Elf_Ehdr *)buf)->e_shoff;
	memcpy(buf + off, _sh, _eh.e_shentsize * _eh.e_shnum);
	sh = (Elf_Shdr *)(buf + off);
	off += _eh.e_shentsize * _eh.e_shnum;

	/* load shstrtab and find desired sections */
	shstrsize = (sh[_eh.e_shstrndx].sh_size + 3) & ~0x3;
	_file->read(buf + off, shstrsize, sh[_eh.e_shstrndx].sh_offset);
	for(i = 0; i < _eh.e_shnum; i++, sh++) {
		if (strcmp(".strtab", buf + off + sh->sh_name) == 0) {
			stroff = sh->sh_offset;
			shstr = sh;
		} else if (strcmp(".symtab", buf + off + sh->sh_name) == 0) {
			symoff = sh->sh_offset;
			shsym = sh;
		}
		if (i == _eh.e_shstrndx)
			sh->sh_offset = off;
		else
			sh->sh_offset = 0;
	}
	/* silently return if strtab or symtab can't be found */
	if (! stroff || ! symoff)
		return TRUE;

	shstr->sh_offset = off + shstrsize;
	shsym->sh_offset = off + shstrsize + ((shstr->sh_size + 3) & ~0x3);
	_load_memory(kv, off + shstrsize, buf);
	kv += off + shstrsize;
	_load_segment(kv, shstr->sh_size, stroff, shstr->sh_size);
	kv += (shstr->sh_size + 3) & ~0x3;
	_load_segment(kv, shsym->sh_size, symoff, shsym->sh_size);

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
