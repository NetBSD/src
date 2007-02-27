/*	$NetBSD: bootxx.c,v 1.2.8.1 2007/02/27 16:50:25 yamt Exp $	*/

/*-
 * Copyright (c) 2004, 2005 The NetBSD Foundation, Inc.
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

#ifdef _STANDALONE
#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>
#endif

#include <sys/types.h>
#include <sys/param.h>
#include "bootxx.h"

#include <sys/exec_ecoff.h>
#include <sys/exec_elf.h>

#include <machine/sector.h>
#include <machine/sbd.h>

#include "common.h"

#define	FILHSZ	(sizeof(struct ecoff_filehdr))
#define	SCNHSZ	(sizeof(struct ecoff_scnhdr))
#define	N_TXTOFF(f, a)							\
	((a)->vstamp < 23 ?						\
	((FILHSZ +(f)->f_opthdr +(f)->f_nscns * SCNHSZ + 7) & ~7) :	\
	((FILHSZ +(f)->f_opthdr +(f)->f_nscns * SCNHSZ + 15) & ~15))

int main(void);
int loader(const char *, uint32_t *);
int load_elf(uint8_t *, uint32_t *);
int load_coff(uint8_t *, uint32_t *);
int dk_read(int, int, void *);

const char *errmsg[] = {
	[BERR_PDINFO]		= "No PDINFO",
	[BERR_RDVTOC]		= "VTOC read error",
	[BERR_VTOC]		= "VTOC bad magic",
	[BERR_NOBFS]		= "No BFS partition",
	[BERR_RDINODE]		= "I-node read error",
	[BERR_NOROOT]		= "No root",
	[BERR_RDDIRENT]		= "Dirent read error",
	[BERR_NOFILE]		= "No such a file",
	[BERR_RDFILE]		= "File read error",
	[BERR_FILEMAGIC]	= "Bad file magic",
	[BERR_AOUTHDR]		= "Not a OMAGIC",
	[BERR_ELFHDR]		= "Not a ELF",
	[BERR_TARHDR]		= "Read tar file",
	[BERR_TARMAGIC]		= "Bad tar magic",
};

const char *boottab[] = {
	"boot",		/* NetBSD (bfs,ustarfs) */
	"iopboot",	/* EWS-UX (bfs) */
	0		/* terminate */
};

int __dk_unit, __dk_type;
bool (*fd_position)(uint32_t, uint32_t *, int *);

int
main(void)
{
	char msg[] = "[+++] NetBSD/ews4800mips >> ";
	const char *p;
	const char **boot;
	uint32_t entry;
	int err;

#ifdef _STANDALONE
	int i, fd_format;

	boot_device(&__dk_type, &__dk_unit, &fd_format);
	msg[1] = __dk_type + '0';
	msg[2] = __dk_unit + '0';
	msg[3] = FS_SIGNATURE;
	for (i = 0; msg[i] != 0; i++)
		ROM_PUTC(32 + i * 12, 536, msg[i]);

	if (__dk_type == NVSRAM_BOOTDEV_FLOPPYDISK) {
		fd_position = blk_to_2d_position;
		if (fd_format == FD_FORMAT_2HD) {
			fd_position = blk_to_2hd_position;
			__dk_unit |= 0x1000000;		/* | 2HD flag */
		}
	}
#else
	printf("%s\n", msg);
	sector_init("/dev/rsd1p", DEV_BSIZE);
	sector_read(SDBOOT_PDINFOADDR, PDINFO_SECTOR);
#endif

	for (boot = boottab; *boot; boot++) {
		if ((err = loader(*boot, &entry)) == 0) {
#ifdef _STANDALONE
			ROM_PUTC(0, 0, '\n');
			ROM_PUTC(0, 0, '\r');
			return entry;
#else
			printf("entry=%p\n", (void *)entry);
			sector_fini();
			return 0;
#endif
		}
		p = errmsg[err];
#ifdef _STANDALONE
		for (i = 0; p[i] != 0; i++)
			ROM_PUTC(600 + i * 12, 536, p[i]);
#else
		DPRINTF("***ERROR *** %s\n", p);
#endif
	}

#ifdef _STANDALONE
	while (/*CONSTCOND*/1)
		;
	/* NOTREACHED */
#else
	sector_fini();
#endif
	return 0;
}

int
loader(const char *fname, uint32_t *entry)
{
	int err;
	uint16_t mag;
	size_t sz;
#ifdef _STANDALONE
	int i;

	for (i = 0; fname[i] != 0; i++)
		ROM_PUTC(380 + i * 12, 536, fname[i]);
#else
	printf("%s\n", fname);
#endif

	if ((err = fileread(fname, &sz)) != 0)
		return err;

	DPRINTF("%s found. %d bytes.\n", fname, sz);

	mag = *(uint16_t *)SDBOOT_SCRATCHADDR;
	if (mag == 0x7f45)
		return load_elf((uint8_t *)SDBOOT_SCRATCHADDR, entry);
	else if (mag == ECOFF_MAGIC_MIPSEB)
		return load_coff((uint8_t *)SDBOOT_SCRATCHADDR, entry);

	return BERR_FILEMAGIC;
}

int
load_elf(uint8_t *buf, uint32_t *entry)
{
	Elf32_Ehdr *e = (void *)buf;
	Elf32_Phdr *p;

	if (e->e_ident[EI_MAG2] != 'L' || e->e_ident[EI_MAG3] != 'F' ||
	    e->e_ident[EI_CLASS] != ELFCLASS32 ||
	    e->e_ident[EI_DATA] != ELFDATA2MSB ||
	    e->e_type != ET_EXEC ||
	    e->e_machine != EM_MIPS)
		return BERR_ELFHDR;

	BASSERT(e->e_phentsize == sizeof(Elf32_Phdr));
	p = (void *)(buf + e->e_phoff);
#ifdef _STANDALONE
	memcpy((void *)p->p_vaddr, buf + p->p_offset, p->p_filesz);
	p++;
	memcpy((void *)p->p_vaddr, buf + p->p_offset, p->p_filesz);
#else
	DPRINTF("ELF entry point 0x%08x\n", e->e_entry);
	DPRINTF("[text] 0x%08x 0x%x %dbyte.\n", p->p_vaddr, p->p_offset,
	    p->p_filesz);
	p++;
	DPRINTF("[data] 0x%08x 0x%x %dbyte.\n", p->p_vaddr, p->p_offset,
	    p->p_filesz);
#endif
	*entry = e->e_entry;

	return 0;
}

int
load_coff(uint8_t *p, uint32_t *entry)
{
	struct ecoff_exechdr *eh = (void *)p;
	struct ecoff_aouthdr *a = &eh->a;
	struct ecoff_filehdr *f = &eh->f;

	if (a->magic != ECOFF_OMAGIC)
		return BERR_AOUTHDR;

	p += N_TXTOFF(f, a);
	DPRINTF("file offset = 0x%x\n", N_TXTOFF(f, a));
#ifdef _STANDALONE
	memcpy((void *)a->text_start, p, a->tsize);
	memcpy((void *)a->data_start, p + a->tsize, a->dsize);
#else
	DPRINTF("COFF entry point 0x%08lx\n", a->entry);
	DPRINTF("[text] 0x%08lx %ld byte.\n", a->text_start, a->tsize);
	DPRINTF("[data] 0x%08lx %ld byte.\n", a->data_start, a->dsize);
#endif

	*entry = a->entry;

	return 0;
}

int
dk_read(int sector, int count, void *buf)
{
#ifdef _STANDALONE
	int i, cnt;
	uint32_t pos;
	uint8_t *p = buf;

	if (__dk_type == NVSRAM_BOOTDEV_HARDDISK) {	/* DISK */
		if ((ROM_DK_READ(__dk_unit, sector, count, p) & 0x7f) != 0)
			return 1;
	} else {	/* FD */
		for (i = 0; i < count; i++, p += 512) {
			fd_position(sector + i, &pos, &cnt);
			if ((ROM_FD_READ(__dk_unit, pos, cnt, p) & 0x7f) != 0)
				return 1;
		}
	}
#else
	sector_read_n(buf, sector, count);
#endif
	return 0;
}
