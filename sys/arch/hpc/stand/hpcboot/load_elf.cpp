/*	$NetBSD: load_elf.cpp,v 1.7 2002/02/11 17:08:55 uch Exp $	*/

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

#include <hpcmenu.h>
#include <menu/window.h>
#include <menu/rootwindow.h>

#include <load.h>
#include <load_elf.h>
#include <console.h>
#include <memory.h>
#include <file.h>

#define ROUND4(x)	(((x) + 3) & ~3)

ElfLoader::ElfLoader(Console *&cons, MemoryManager *&mem)
	: Loader(cons, mem)
{
	_sym_blk.enable = FALSE;

	DPRINTF((TEXT("Loader: ELF\n")));
}

ElfLoader::~ElfLoader(void)
{
	if (_sym_blk.header != NULL)
		free (_sym_blk.header);
}

BOOL
ElfLoader::setFile(File *&file)
{
	size_t sz;
	Loader::setFile(file);

	// read ELF header and check it
	if (!read_header())
		return FALSE;
	// read section header
	sz = _eh.e_shnum * _eh.e_shentsize;
	_file->read(_sh, _eh.e_shentsize * _eh.e_shnum, _eh.e_shoff);

	// read program header
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

	// reserve for symbols
	size_t symblk_sz = symbol_block_size();
	if (symblk_sz) {
		sz += symblk_sz;
		DPRINTF((TEXT(" = 0x%x]"), symblk_sz));
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
	vaddr_t kv;
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

	load_symbol_block(kv);

	// tag chain still opening 

	return _load_success();
}

//
// Prepare ELF headers for symbol table.
//
//   ELF header
//   section header
//   shstrtab
//   strtab
//   symtab
//
size_t
ElfLoader::symbol_block_size()
{
	size_t shstrsize = ROUND4(_sh[_eh.e_shstrndx].sh_size);
	size_t shtab_sz = _eh.e_shentsize * _eh.e_shnum;
	off_t shstrtab_offset = sizeof(Elf_Ehdr) + shtab_sz;
	int i;

	memset(&_sym_blk, 0, sizeof(_sym_blk));
	_sym_blk.enable = FALSE;
	_sym_blk.header_size = sizeof(Elf_Ehdr) + shtab_sz + shstrsize;

	// inquire string and symbol table size
	_sym_blk.header = static_cast<char *>(malloc(_sym_blk.header_size));
	if (_sym_blk.header == NULL) {
		MessageBox(HPC_MENU._root->_window,
		    TEXT("couldn't determine symbol block size"),
		    TEXT("WARNING"), 0);
		return (0);
	}

	// set pointer for symbol block
	Elf_Ehdr *eh = reinterpret_cast<Elf_Ehdr *>(_sym_blk.header);
	Elf_Shdr *sh = reinterpret_cast<Elf_Shdr *>
	    (_sym_blk.header + sizeof(Elf_Ehdr));
	char *shstrtab = _sym_blk.header + shstrtab_offset;

	// initialize headers
	memset(_sym_blk.header, 0, _sym_blk.header_size);
	memcpy(eh, &_eh, sizeof(Elf_Ehdr));
	eh->e_phoff = 0;
	eh->e_phnum = 0;
	eh->e_entry = 0; // XXX NetBSD kernel check this member. see machdep.c
	eh->e_shoff = sizeof(Elf_Ehdr);
	memcpy(sh, _sh, shtab_sz);

	// inquire symbol/string table information
	_file->read(shstrtab, shstrsize, _sh[_eh.e_shstrndx].sh_offset);
	for (i = 0; i < _eh.e_shnum; i++, sh++) {
		if (strcmp(".strtab", shstrtab + sh->sh_name) == 0) {
			_sym_blk.shstr = sh;
			_sym_blk.stroff = sh->sh_offset;
		} else if (strcmp(".symtab", shstrtab + sh->sh_name) == 0) {
			_sym_blk.shsym = sh;
			_sym_blk.symoff = sh->sh_offset;
		}

		sh->sh_offset = (i == _eh.e_shstrndx) ? shstrtab_offset : 0;
	}

	if (_sym_blk.shstr == NULL || _sym_blk.shsym == NULL) {
		if (HPC_PREFERENCE.safety_message)
			MessageBox(HPC_MENU._root->_window,
			    TEXT("no symbol and/or string table in binary. (not fatal)"),
			    TEXT("Information"), 0);
		free(_sym_blk.header);
		_sym_blk.header = NULL;

		return (0);
	}

	// set Section Headers for symbol/string table
	_sym_blk.shstr->sh_offset = shstrtab_offset + shstrsize;
	_sym_blk.shsym->sh_offset = shstrtab_offset + shstrsize +
	    ROUND4(_sym_blk.shstr->sh_size);
	_sym_blk.enable = TRUE;

	DPRINTF((TEXT("+[(symbol block: header %d string %d symbol %d byte)"),
	    _sym_blk.header_size,_sym_blk.shstr->sh_size, 
	    _sym_blk.shsym->sh_size));

	// return total amount of symbol block
	return (_sym_blk.header_size + ROUND4(_sym_blk.shstr->sh_size) +
	    _sym_blk.shsym->sh_size);
}

void
ElfLoader::load_symbol_block(vaddr_t kv)
{
	size_t sz;
	
	if (!_sym_blk.enable)
		return;

	// load header
	_load_memory(kv, _sym_blk.header_size, _sym_blk.header);
	kv += _sym_blk.header_size;

	// load string table
	sz = _sym_blk.shstr->sh_size;
	_load_segment(kv, sz, _sym_blk.stroff, sz);
	kv += ROUND4(sz);

	// load symbol table
	sz = _sym_blk.shsym->sh_size;
	_load_segment(kv, sz, _sym_blk.symoff, sz);
}

BOOL
ElfLoader::read_header()
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
  
	// Check object type
	if (_eh.e_type != ET_EXEC) {
		DPRINTF((TEXT("not a executable file. type = %d\n"),
		    _eh.e_type));
		return FALSE;
	}
  
	if (_eh.e_phoff == 0 || _eh.e_phnum == 0 || _eh.e_phnum > 16 ||
	    _eh.e_phentsize != sizeof(Elf_Phdr)) {
		DPRINTF((TEXT("invalid program header information.\n")));
		return FALSE;
	}

	return TRUE;
}
