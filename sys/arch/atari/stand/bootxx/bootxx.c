/*	$NetBSD: bootxx.c,v 1.4.2.1 2001/10/01 12:38:13 fvdl Exp $	*/

/*
 * Copyright (c) 1995 Waldi Ravens.
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
 *        This product includes software developed by Waldi Ravens.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#define	boot_BSD	bsd_startup

#include <stand.h>
#include <atari_stand.h>
#include <string.h>
#include <libkern.h>
#include <kparamb.h>
#include <sys/boot_flag.h>
#ifndef __ELF__
#include <sys/exec.h>
#else
#include <sys/exec_elf.h>
#endif
#include <sys/reboot.h>
#include <machine/cpu.h>

#include "bootxx.h"

void	boot_BSD __P((kparb *)__attribute__((noreturn)));
int	load_BSD __P((osdsc *));
int	sys_info __P((osdsc *));
int	usr_info __P((osdsc *));

int
bootxx(readsector, disklabel, autoboot)
	void	*readsector,
		*disklabel;
	int	autoboot;
{
	static osdsc	os_desc;
	extern char	end[], edata[];
	osdsc		*od = &os_desc;

	bzero(edata, end - edata);

	printf("\033v\nNetBSD/Atari boot loader ($Revision: 1.4.2.1 $)\n\n");

	if (init_dskio(readsector, disklabel, -1))
		return(-1);

	if (sys_info(od))
		return(-2);

	for (;;) {
		od->rootfs = 0;			/* partition a */
		od->osname = "/netbsd";
		od->ostype = &od->osname[1];
		od->boothowto = (RB_RDONLY);

		if (!autoboot) {
			int	pref;

			od->boothowto = (RB_RDONLY|RB_SINGLE);
			pref = usr_info(od);
			if (pref < 0)
				continue;
			if (pref > 0)
				return(pref);
		}
		autoboot = 0;			/* in case auto boot fails */

		if (init_dskio(readsector, disklabel, od->rootfs))
			continue;

		if (load_BSD(od))
			continue;

		boot_BSD(&od->kp);
	}
	/* NOTREACHED */
}

int
sys_info(od)
	osdsc	*od;
{
	long	*jar;
	int	tos;

	od->stmem_size = *ADDR_PHYSTOP;

	if (*ADDR_CHKRAMTOP == RAMTOP_MAGIC) {
		od->ttmem_size  = *ADDR_RAMTOP;
		if (od->ttmem_size > TTRAM_BASE) {
			od->ttmem_size  -= TTRAM_BASE;
			od->ttmem_start  = TTRAM_BASE;
		}
	}

	tos = (*ADDR_SYSBASE)->os_version;
	if ((tos > 0x300) && (tos < 0x306))
		od->cputype = ATARI_CLKBROKEN;

	if ((jar = *ADDR_P_COOKIE)) {
		while (jar[0]) {
			if ((jar[0] == 0x5f435055L) && ((jar[1] % 10) == 0)) { /* _CPU  */
				switch(jar[1] / 10) {
					case 0:
						od->cputype |= ATARI_68000;
						break;
					case 1:
						od->cputype |= ATARI_68010;
						break;
					case 2:
						od->cputype |= ATARI_68020;
						break;
					case 3:
						od->cputype |= ATARI_68030;
						break;
					case 4:
						od->cputype |= ATARI_68040;
						break;
					case 6:
						od->cputype |= ATARI_68060;
						break;
				}
			}
			jar += 2;
		}
	}
	if (!(od->cputype & ATARI_ANYCPU)) {
		printf("Unknown CPU-type.\n");
		return(-1);
	}

	return(0);
}

int
usr_info(od)
	osdsc	*od;
{
	static char	line[800];
	char		c, *p = line;

	printf("\nEnter os-type [.%s] root-fs [:a] kernel [%s]"
	       " options [none]:\n\033e", od->ostype, od->osname);
	gets(p);
	printf("\033f");

	for (;;) {
		while (isspace(*p))
			*p++ = '\0';

		switch (*p++) {
		  case '\0':
			goto done;
		  case ':':
			if ((c = *p) >= 'a' && c <= 'z')
				od->rootfs = c - 'a';
			else if (c >= 'A' && c <= 'Z')
				od->rootfs = c - ('A' - 27);
			else return(-1);

			if (!od->rootfs)
				break;
			*p = 'b';
			/* FALLTHROUGH */
		  case '-':
			if ((c = *p) == 'a')
				od->boothowto &= ~RB_SINGLE;
			else if (c == 'b')
				od->boothowto |= RB_ASKNAME;
			else
				BOOT_FLAG(c, od->boothowto);
			break;
		  case '.':
			od->ostype = p;
			break;
		  case '/':
			od->osname = --p;
			break;
		  default:
			return(-1);
		}

		while ((c = *p) && !isspace(c))
			p += 1;
	}

done:
	c = od->ostype[0];
	if (isupper(c))
		c = tolower(c);

	switch (c) {
	  case 'n':		/* NetBSD */
		return(0);
	  case 'l':		/* Linux  */
		return(0x10);
	  case 'a':		/* ASV    */
		return(0x40);
	  case 't':		/* TOS    */
		return(0x80);
	  default:
		return(-1);
	}
}

#ifndef __ELF__		/* a.out */
int
load_BSD(od)
	osdsc		*od;
{
	struct exec	hdr;
	int		err, fd;
	u_int		textsz, strsz;

	/*
	 * Read kernel's exec-header.
	 */
	err = 1;
	if ((fd = open(od->osname, 0)) < 0)
		goto error;
	err = 2;
	if (read(fd, &hdr, sizeof(hdr)) != sizeof(hdr))
		goto error;
	err = 3;
	if ((N_GETMAGIC(hdr) != NMAGIC) && (N_GETMAGIC(hdr) != OMAGIC))
		goto error;

	/*
	 * Extract sizes from kernel executable.
	 */
	textsz     = (hdr.a_text + __LDPGSZ - 1) & ~(__LDPGSZ - 1);
	od->ksize  = textsz + hdr.a_data + hdr.a_bss;
	od->kentry = hdr.a_entry;
	od->kstart = NULL;
	od->k_esym = 0;

	if (hdr.a_syms) {
	    u_int x = sizeof(hdr) + hdr.a_text + hdr.a_data + hdr.a_syms;
	    err = 8;
	    if (lseek(fd, (off_t)x, SEEK_SET) != x)
		goto error;
	    err = 9;
	    if (read(fd, &strsz, sizeof(strsz)) != sizeof(strsz))
		goto error;
	    err = 10;
	    if (lseek(fd, (off_t)sizeof(hdr), SEEK_SET) != sizeof(hdr))
		goto error;
	    od->ksize += sizeof(strsz) + hdr.a_syms + strsz;
	}

	/*
	 * Read text & data, clear bss
	 */
	err = 16;
	if ((od->kstart = alloc(od->ksize)) == NULL)
		goto error;
	err = 17;
	if (read(fd, od->kstart, hdr.a_text) != hdr.a_text)
		goto error;
	err = 18;
	if (read(fd, od->kstart + textsz, hdr.a_data) != hdr.a_data)
		goto error;
	bzero(od->kstart + textsz + hdr.a_data, hdr.a_bss);

	/*
	 * Read symbol and string table
	 */
	if (hdr.a_syms) {
		char *p = od->kstart + textsz + hdr.a_data + hdr.a_bss;
		*((u_int32_t *)p)++ = hdr.a_syms;
		err = 24;
		if (read(fd, p, hdr.a_syms) != hdr.a_syms)
			goto error;
		p += hdr.a_syms;
		err = 25;
		if (read(fd, p, strsz) != strsz)
			goto error;
		od->k_esym = (p - (char *)od->kstart) + strsz;
	}

	return(0);

error:
	if (fd >= 0) {
		if (od->kstart)
			free(od->kstart, od->ksize);
		close(fd);
	}
	printf("Error %d in load_BSD.\n", err);
	return(-1);
}

#else			/* __ELF__ */

#define ELFMAGIC	((ELFMAG0 << 24) | (ELFMAG1 << 16) | (ELFMAG2 << 8) | ELFMAG3)

int
load_BSD(od)
	osdsc		*od;
{
    int err, fd, i, j;
    Elf32_Ehdr ehdr;
    Elf32_Word kernsize;
#if 0
    Elf32_Word symsize, symstart;
#endif

    /*
     * Read kernel's exec-header.
     */
    od->kstart = NULL;
    err = 1;
    if ((fd = open(od->osname, 0)) < 0)
	goto error;
    err = 2;
    if (read(fd, &ehdr, sizeof(ehdr)) != sizeof(ehdr))
	goto error;
    err = 3;
    if (*((u_int *)ehdr.e_ident) != ELFMAGIC)
	goto error;
    err = 4;
    if (lseek(fd, (off_t)ehdr.e_phoff, SEEK_SET) < 0)
	goto error;

    /*
     * calculate highest used address
     */
    i = ehdr.e_phnum + 1;
    kernsize = 0;
    while (--i) {
	Elf32_Phdr phdr;
	Elf32_Word sum;
	err = 5;
	if (read(fd, &phdr, sizeof(phdr)) != sizeof(phdr))
	    goto error;
	sum = phdr.p_vaddr + phdr.p_memsz;
	if ((phdr.p_flags & (PF_W|PF_X)) && (sum > kernsize))
	    kernsize = sum;
    }

#if 0
    /*
     * look for symbols and calculate the size
     */
    err = 6;
    if (lseek(fd, (off_t)ehdr.e_shoff, SEEK_SET) < 0)
	goto error;
    i = ehdr.e_shnum + 1;
    symsize = 0;
    symstart = 0;
    while (--i) {
	Elf32_Shdr shdr;
	err = 7;
	if (read(fd, &shdr, sizeof(shdr)) != sizeof(shdr))
	    goto error;
	if ((shdr.sh_type == SHT_SYMTAB) || (shdr.sh_type == SHT_STRTAB))
	    symsize += shdr.sh_size;
    }

    if (symsize) {
	symstart = kernsize;
	kernsize += symsize + sizeof(ehdr) + ehdr.e_shoff * sizeof(Elf32_Shdr);
    }
#endif

    od->ksize = kernsize;
    od->kentry = ehdr.e_entry;
    od->k_esym = 0;

    err = 10;
    if ((od->kstart = alloc(od->ksize)) == NULL)
	goto error; 

    /*
     * read segements, clear bss
     */
    i = ehdr.e_phnum + 1;
    j = 0;
    while (--i) {
	Elf32_Phdr phdr;
	Elf32_Word pos;
	u_char *p;
	pos = ehdr.e_phoff + j * sizeof(phdr);
	err = 11;
	if (lseek(fd, (off_t)pos, SEEK_SET) < 0)
	    goto error;
	err = 12;
	if (read(fd, &phdr, sizeof(phdr)) != sizeof(phdr))
	    goto error;
	if (phdr.p_flags & (PF_W|PF_X)) {
	    err = 13;
            if (lseek(fd, (off_t)phdr.p_offset, SEEK_SET) < 0)
		goto error;
	    p = (u_char *)(od->kstart) + phdr.p_vaddr;
	    err = 14;
	    if (read(fd, p, phdr.p_filesz) != phdr.p_filesz)
		goto error;
	    if (phdr.p_memsz > phdr.p_filesz)
		bzero(p + phdr.p_filesz, phdr.p_memsz - phdr.p_filesz);
	}
	++j;
    }

#if 0
    /*
     * read symbol and string
     */
    if (symsize) {
	Elf32_Word pos;
	int k = ehdr.e_shoff;
	j = symstart + sizeof(ehdr);
	pos = symstart + sizeof(ehdr) + ehdr.e_shoff * sizeof(Elf32_Shdr);
	bzero((u_char *)(od->kstart) + j, ehdr.e_shoff * sizeof(Elf32_Shdr));
	i = ehdr.e_shnum + 1;
	while (--i) {
	    Elf32_Shdr shdr;
	    u_char *p;
	    err = 20;
	    if (lseek(fd, (off_t)k, SEEK_SET) < 0)
		goto error;
	    err = 21;
	    if (read(fd, &shdr, sizeof(shdr)) != sizeof(shdr))
		goto error;
	    if ((shdr.sh_type == SHT_SYMTAB) || (shdr.sh_type == SHT_STRTAB)) {
		err = 22;
		if (lseek(fd, (off_t)shdr.sh_offset, SEEK_SET) < 0)
		    goto error;
		p = (u_char *)(od->kstart) + pos;
		err = 23;
		if (read(fd, p, shdr.sh_size) != shdr.sh_size)
		    goto error;
		shdr.sh_offset = pos;
		pos += shdr.sh_size;
		bcopy(&shdr, (u_char *)(od->kstart) + j, sizeof(shdr));
	    }
	    j += sizeof(shdr);
	    k += sizeof(shdr);
	}
	ehdr.e_shoff = symstart + sizeof(ehdr);
	bcopy(&ehdr, (u_char *)(od->kstart) + symstart, sizeof(ehdr));
    }
#endif

    return(0);

error:
    
    if (fd >= 0) {
	if (od->kstart)
	    free(od->kstart, od->ksize);
	close(fd);
    }
    printf("Error %d in load_BSD.\n", err);
    return(-1);

}
#endif /* __ELF__ */
