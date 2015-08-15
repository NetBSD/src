/* Copyright 2013-2014 Free Software Foundation, Inc.
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __SYM_FILE_LOADER__
#define __SYM_FILE_LOADER__

#include <inttypes.h>
#include <ansidecl.h>
#include <elf/common.h>
#include <elf/external.h>

#ifdef TARGET_LP64

typedef Elf64_External_Phdr Elf_External_Phdr;
typedef Elf64_External_Ehdr Elf_External_Ehdr;
typedef Elf64_External_Shdr Elf_External_Shdr;
typedef Elf64_External_Sym Elf_External_Sym;
typedef uint64_t Elf_Addr;

#elif defined TARGET_ILP32

typedef Elf32_External_Phdr Elf_External_Phdr;
typedef Elf32_External_Ehdr Elf_External_Ehdr;
typedef Elf32_External_Shdr Elf_External_Shdr;
typedef Elf32_External_Sym Elf_External_Sym;
typedef uint32_t Elf_Addr;

#endif

#define GET(hdr, field) (\
sizeof ((hdr)->field) == 1 ? (uint64_t) (hdr)->field[0] : \
sizeof ((hdr)->field) == 2 ? (uint64_t) *(uint16_t *) (hdr)->field : \
sizeof ((hdr)->field) == 4 ? (uint64_t) *(uint32_t *) (hdr)->field : \
sizeof ((hdr)->field) == 8 ? *(uint64_t *) (hdr)->field : \
*(uint64_t *) NULL)

#define GETADDR(hdr, field) (\
sizeof ((hdr)->field) == sizeof (Elf_Addr) ? *(Elf_Addr *) (hdr)->field : \
*(Elf_Addr *) NULL)

struct segment
{
  uint8_t *mapped_addr;
  Elf_External_Phdr *phdr;
  struct segment *next;
};

/* Mini shared library loader.  No reallocation is performed
   for the sake of simplicity.  */

int
load_shlib (const char *file, Elf_External_Ehdr **ehdr_out,
	    struct segment **seg_out);

/* Return the section-header table.  */

Elf_External_Shdr *find_shdrtab (Elf_External_Ehdr *ehdr);

/* Return the string table of the section headers.  */

const char *find_shstrtab (Elf_External_Ehdr *ehdr, uint64_t *size);

/* Return the string table named SECTION.  */

const char *find_strtab (Elf_External_Ehdr *ehdr,
			 const char *section, uint64_t *strtab_size);

/* Return the section header named SECTION.  */

Elf_External_Shdr *find_shdr (Elf_External_Ehdr *ehdr, const char *section);

/* Return the symbol table.  */

Elf_External_Sym *find_symtab (Elf_External_Ehdr *ehdr,
			       uint64_t *symtab_size);

/* Translate a file offset to an address in a loaded segment.   */

int translate_offset (uint64_t file_offset, struct segment *seg, void **addr);

/* Lookup the address of FUNC.  */

int
lookup_function (const char *func, Elf_External_Ehdr* ehdr,
		 struct segment *seg, void **addr);

#endif
