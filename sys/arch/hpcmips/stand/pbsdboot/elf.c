/*	$NetBSD: elf.c,v 1.7.24.1 2007/03/12 05:48:14 rmind Exp $	*/

/*-
 * Copyright (c) 1999 Shin Takemura.
 * All rights reserved.
 *
 * This software is part of the PocketBSD.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#include <pbsdboot.h>

#define	ELFSIZE 32
//#include <sys/param.h>
//#include <sys/exec.h>
#include <sys/exec_elf.h>

#if 1
#define LOAD_DEBUG_INFO
#define DEBUG_INFO_ALIGN 4096
#define ROUNDUP(a, n)	((((int)(a)) + (n)-1)/(n)*(n))
#endif

static int
scanfile(int fd, void **start, void **end, void **entry, int load);

static long total_bytes = 0;

int
getinfo(int fd, void **start, void **end)
{
	return (scanfile(fd, start, end, NULL, 0));
}

int
loadfile(int fd, void **entry)
{
	return (scanfile(fd, NULL, NULL, entry, 1));
}

enum {
	VMEM_CLEAR, VMEM_LOAD, VMEM_COPY
};

int
vmem_sub(int opr, void* xxx, void *addr, int nbytes, int *byte_count)
{
	int n;
	void *end_addr, *vaddr;
	int count = 0;
	int progress = 0;
	int fd = (int)xxx;
	void *src_addr = (void *)xxx;

	debug_printf(TEXT("loadfile_sub(%x-%x, %S)\n"),
		     addr, addr + nbytes,
		     opr == VMEM_CLEAR ? "clear" : (opr == VMEM_LOAD ? "load" : "copy"));

	for (end_addr = addr + nbytes;
	     addr < end_addr;
	     addr += n) {
		if ((vaddr = vmem_get(addr, &n)) == NULL) {
			debug_printf(TEXT("vmem_get(0x%x) failed.\n"), addr);
			msg_printf(MSG_ERROR, whoami, TEXT("vmem_get(0x%x) failed.\n"), addr);
			return (-1);
		}
		if (end_addr < addr + n) {
			n = end_addr - addr;
		}
		switch (opr) {
		case VMEM_CLEAR:
			memset(vaddr, 0, n);
			break;
		case VMEM_LOAD:
			if (read(fd, vaddr, n) != n) {
				debug_printf(TEXT("read segment error.\n"));
				msg_printf(MSG_ERROR, whoami, TEXT("read segment error.\n"));
				return (-1);
			}
			break;
		case VMEM_COPY:
			memcpy(vaddr, src_addr, n);
			src_addr += n;
			break;
		}
		if (total_bytes != 0) {
			int tmp_progress = *byte_count * 100 / total_bytes;
			*byte_count += n;
			if (progress != tmp_progress) {
				progress =  tmp_progress;
				if (CheckCancel(progress)) {
					return (-1);
				}
			}
		}
	}
	return (0);
}
      

static int
scanfile(int fd, void **start, void **end, void **entry, int load)
{
	Elf_Ehdr elfx, *elf = &elfx;
	int i, first;
	long byte_count;
	int progress;
	Elf_Shdr *shtbl = NULL;
	Elf_Phdr *phtbl = NULL;
	void *min_addr, *max_addr;
	int sh_symidx, sh_stridx;
	int dbg_hdr_size = sizeof(Elf_Ehdr) + sizeof(Elf_Shdr) * 2;

	if (lseek(fd, 0, SEEK_SET) == -1)  {
		debug_printf(TEXT("seek error\n"));
		msg_printf(MSG_ERROR, whoami, TEXT("seek error.\n"));
		goto error_cleanup;
	}
	if (read(fd, (void*)elf, sizeof(Elf_Ehdr)) != sizeof(Elf_Ehdr)) {
		debug_printf(TEXT("read header error\n"));
		msg_printf(MSG_ERROR, whoami, TEXT("read header error.\n"));
		goto error_cleanup;
	}

	if ((phtbl = (Elf_Phdr *)alloc(sizeof(*phtbl) * elf->e_phnum)) == NULL ||
		(shtbl = (Elf_Shdr *)alloc(sizeof(*shtbl) * elf->e_shnum)) == NULL) {
		debug_printf(TEXT("alloc() error\n"));
		msg_printf(MSG_ERROR, whoami, TEXT("malloc() error.\n"));
		goto error_cleanup;
	}

	if (lseek(fd, elf->e_phoff, SEEK_SET) == -1)  {
		debug_printf(TEXT("seek for program header table error\n"));
		msg_printf(MSG_ERROR, whoami, TEXT("seek for program header table error.\n"));
		goto error_cleanup;
	}
	if (read(fd, (void *)phtbl, sizeof(Elf_Phdr) * elf->e_phnum)
			!= (int)(sizeof(Elf_Phdr) * elf->e_phnum)) {
		debug_printf(TEXT("read program header table error\n"));
		msg_printf(MSG_ERROR, whoami, TEXT("read program header table error.\n"));
		goto error_cleanup;
	}

	if (lseek(fd, elf->e_shoff, SEEK_SET) == -1)  {
		debug_printf(TEXT("seek for segment header table error.\n"));
		msg_printf(MSG_ERROR, whoami, TEXT("seek for segment header table error.\n"));
		goto error_cleanup;
	}
	if (read(fd, (void *)shtbl, sizeof(Elf_Shdr) * elf->e_shnum)
			!= (int)(sizeof(Elf_Shdr) * elf->e_shnum)) {
		debug_printf(TEXT("read segment header table error\n"));
		msg_printf(MSG_ERROR, whoami, TEXT("read segment header table error.\n"));
		goto error_cleanup;
	}

	/*
	 *  scan program header table
	 */
	first = 1;
	byte_count = 0;
	progress = 0;
	for (i = 0; i < elf->e_phnum; i++) {
		if (phtbl[i].p_type != PT_LOAD) {
			continue;
		}

		if (first || max_addr < (void *)(phtbl[i].p_vaddr + phtbl[i].p_memsz)) {
			max_addr = (void *)(phtbl[i].p_vaddr + phtbl[i].p_memsz);
		}
		if (first || (void *)phtbl[i].p_vaddr < min_addr) {
			min_addr = (void *)phtbl[i].p_vaddr;
		}

		if (load) {
			if (lseek(fd, phtbl[i].p_offset, SEEK_SET) == -1)  {
				debug_printf(TEXT("seek for segment error\n"));
				msg_printf(MSG_ERROR, whoami, TEXT("seek for segment error.\n"));
				goto error_cleanup;
			}

			if (vmem_sub(VMEM_LOAD, (void*)fd,
				     (void *)phtbl[i].p_vaddr,
				     phtbl[i].p_filesz,
				     &byte_count) != 0) {
				goto error_cleanup;
			}
			if (vmem_sub(VMEM_CLEAR, NULL,
				     (void *)phtbl[i].p_vaddr + phtbl[i].p_filesz,
				     phtbl[i].p_memsz - phtbl[i].p_filesz,
				     &byte_count) != 0) {
				goto error_cleanup;
			}
		} else {
			byte_count += phtbl[i].p_memsz;
		}

		first = 0;
	}

	if (first) {
		debug_printf(TEXT("can't find loadable segment\n"));
		msg_printf(MSG_ERROR, whoami, TEXT("can't find loadable segment\n"));
		goto error_cleanup;
	}
	total_bytes = byte_count;

	debug_printf(TEXT("entry=%x  addr=%x-%x\n"),
		     elf->e_entry, min_addr, max_addr);

#ifdef LOAD_DEBUG_INFO
	if (pref.load_debug_info) {
		/*
		 *  scan section header table
		 *  to search for debugging information
		 */
		sh_symidx = -1;
		sh_stridx = -1;
		for (i = 0; i < elf->e_shnum; i++) {
			if (shtbl[i].sh_type == SHT_SYMTAB) {
				sh_symidx = i;
			}
			if ((shtbl[i].sh_type == SHT_STRTAB)
			    && (shtbl[i].sh_size >= 0x4000)) {
				sh_stridx = i;
			}
		}
		if (sh_symidx == -1 || sh_stridx == -1) {
			debug_printf(TEXT("debugging information not found\n"));
		} else
		if (load) {
			Elf_Ehdr dbg_eh;
			Elf_Shdr dbg_sh[2];
			memset(&dbg_eh, 0, sizeof(Elf_Ehdr));
			memset(dbg_sh, 0, sizeof(Elf_Shdr) * 2);

			memcpy(dbg_eh.e_ident, elf->e_ident,
			       sizeof(elf->e_ident));
			dbg_eh.e_machine = elf->e_machine;
			dbg_eh.e_version = elf->e_version;
			dbg_eh.e_entry = 0;
			dbg_eh.e_phoff = 0;
			dbg_eh.e_shoff = sizeof(Elf_Ehdr);
			dbg_eh.e_flags = elf->e_flags;
			dbg_eh.e_ehsize = sizeof(Elf_Ehdr);
			dbg_eh.e_phentsize = 0;
			dbg_eh.e_phnum = 0;
			dbg_eh.e_shentsize = sizeof(Elf_Shdr);
			dbg_eh.e_shnum = 2;
			dbg_eh.e_shstrndx = 0;	/* ??? */

			/*
			 *  XXX, pass debug info size in e_entry.
			 */
			dbg_eh.e_entry = ROUNDUP(dbg_hdr_size +
						 shtbl[sh_symidx].sh_size +
						 shtbl[sh_stridx].sh_size,
						 DEBUG_INFO_ALIGN);

			if (vmem_sub(VMEM_COPY, (void*)&dbg_eh,
				     max_addr,
				     sizeof(Elf_Ehdr),
				     &byte_count) != 0) {
				goto error_cleanup;
			}

			dbg_sh[0].sh_type = shtbl[sh_symidx].sh_type;
			dbg_sh[0].sh_offset = dbg_hdr_size;
			dbg_sh[0].sh_size = shtbl[sh_symidx].sh_size;
			dbg_sh[0].sh_addralign = shtbl[sh_symidx].sh_addralign;
			dbg_sh[1].sh_type = shtbl[sh_stridx].sh_type;
			dbg_sh[1].sh_offset = dbg_hdr_size + shtbl[sh_symidx].sh_size;
			dbg_sh[1].sh_size = shtbl[sh_stridx].sh_size;
			dbg_sh[1].sh_addralign = shtbl[sh_stridx].sh_addralign;
			if (vmem_sub(VMEM_COPY, (void*)dbg_sh,
				     max_addr + sizeof(Elf_Ehdr),
				     sizeof(Elf_Shdr) * 2,
				     &byte_count) != 0) {
				goto error_cleanup;
			}

			if (lseek(fd, shtbl[sh_symidx].sh_offset, SEEK_SET) == -1)  {
				debug_printf(TEXT("seek for debug symbol error\n"));
				msg_printf(MSG_ERROR, whoami,
					   TEXT("seek for segment error.\n"));
				goto error_cleanup;
			}
			if (vmem_sub(VMEM_LOAD, (void*)fd,
				     max_addr + dbg_hdr_size,
				     shtbl[sh_symidx].sh_size,
				     &byte_count) != 0) {
				goto error_cleanup;
			}

			if (lseek(fd, shtbl[sh_stridx].sh_offset, SEEK_SET) == -1)  {
				debug_printf(TEXT("seek for string table error\n"));
				msg_printf(MSG_ERROR, whoami,
					   TEXT("seek for segment error.\n"));
				goto error_cleanup;
			}
			if (vmem_sub(VMEM_LOAD, (void*)fd,
				     max_addr + dbg_hdr_size + shtbl[sh_symidx].sh_size,
				     shtbl[sh_stridx].sh_size,
				     &byte_count) != 0) {
				goto error_cleanup;
			}
		} else {
			/*
			 *  make space for debugging information
			 */
			int dbg_info_size = ROUNDUP(dbg_hdr_size +
						    shtbl[sh_symidx].sh_size +
						    shtbl[sh_stridx].sh_size,
						    DEBUG_INFO_ALIGN);
			debug_printf(TEXT("%x bytes debug information\n"),
				     dbg_info_size);
			max_addr += dbg_info_size;
			total_bytes += dbg_info_size;
		}
	}
#endif /* LOAD_DEBUG_INFO */

	if (phtbl) dealloc(phtbl, sizeof(*phtbl) * elf->e_phnum);
	if (shtbl) dealloc(shtbl, sizeof(*shtbl) * elf->e_shnum);

	if (start) *start = min_addr;
	if (end) *end = max_addr;
	if (entry) *entry = (void *)elf->e_entry;
	return (0);

 error_cleanup:
	if (phtbl) dealloc(phtbl, sizeof(*phtbl) * elf->e_phnum);
	if (shtbl) dealloc(shtbl, sizeof(*shtbl) * elf->e_shnum);
	return (-1);
}
