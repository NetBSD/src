/*	$NetBSD: multiboot.c,v 1.1 2006/02/03 11:08:24 jmmv Exp $	*/

/*-
 * Copyright (c) 2005, 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julio M. Merino Vidal.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: multiboot.c,v 1.1 2006/02/03 11:08:24 jmmv Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cdefs_elf.h>
#include <sys/boot_flag.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>
#include <sys/optstr.h>

#include <machine/bootinfo.h>
#include <machine/multiboot.h>

#if !defined(MULTIBOOT)
#  error "MULTIBOOT not defined; this cannot happen."
#endif

/* --------------------------------------------------------------------- */

/*
 * External variables.  All of them, with the exception of 'end', must
 * be set at some point within this file.
 */
extern int		biosbasemem;
extern int		biosextmem;
extern int		boothowto;
extern struct bootinfo	bootinfo;
extern int		end;
extern int *		esym;

/* --------------------------------------------------------------------- */

/*
 * Copy of the Multiboot information structure passed to us by the boot
 * loader.  The Multiboot_Info structure has some pointers adjusted to the
 * other variables -- see multiboot_pre_reloc() -- so you oughtn't access
 * them directly.  In other words, always access them through the
 * Multiboot_Info variable.
 */
static char			Multiboot_Cmdline[255];
static uint8_t			Multiboot_Drives[255];
static struct multiboot_info	Multiboot_Info;
static boolean_t		Multiboot_Loader = FALSE;
static char			Multiboot_Loader_Name[255];
static uint8_t			Multiboot_Mmap[1024];

/* --------------------------------------------------------------------- */

/*
 * Simplified ELF image that contains the symbol table for the booted
 * kernel.  Stuck after the kernel image (in memory).
 *
 * Must be less than MULTIBOOT_SYMTAB_SPACE in bytes.  Otherwise, there
 * is no guarantee that it will not overwrite other stuff passed in by
 * the boot loader.
 */
struct symbols_image {
	Elf32_Ehdr	i_ehdr;
	Elf32_Shdr	i_shdr[4];
	char		i_strtab[32];
	char		i_data; /* Actually longer. */
};

/* --------------------------------------------------------------------- */

/*
 * Prototypes for private functions.
 */
static void	bootinfo_add(struct btinfo_common *, int, int);
static void	copy_syms(struct multiboot_info *);
static void	setup_biosgeom(struct multiboot_info *);
static void	setup_bootdisk(struct multiboot_info *);
static void	setup_bootpath(struct multiboot_info *);
static void	setup_console(struct multiboot_info *);
static void	setup_howto(struct multiboot_info *);
static void	setup_memory(struct multiboot_info *);
static void	setup_memmap(struct multiboot_info *);
static void	setup_syms(struct multiboot_info *);

/* --------------------------------------------------------------------- */

/*
 * Sets up the kernel if it was booted by a Multiboot-compliant boot
 * loader.  This is executed before the kernel has relocated itself.
 * The main purpose of this function is to copy all the information
 * passed in by the boot loader to a safe place, so that it is available
 * after it has been relocated.
 *
 * WARNING: Because the kernel has not yet relocated itself to KERNBASE,
 * special care has to be taken when accessing memory because absolute
 * addresses (referring to kernel symbols) do not work.  So:
 *
 *     1) Avoid jumps to absolute addresses (such as gotos and switches).
 *     2) To access global variables use their physical address, which
 *        can be obtained using the RELOC macro.
 */
void
multiboot_pre_reloc(struct multiboot_info *mi)
{
#define RELOC(type, x) ((type)((vaddr_t)(x) - KERNBASE))
	struct multiboot_info *midest =
	    RELOC(struct multiboot_info *, &Multiboot_Info);

	*RELOC(boolean_t *, &Multiboot_Loader) = TRUE;
	memcpy(midest, mi, sizeof(Multiboot_Info));

	if (mi->mi_flags & MULTIBOOT_INFO_HAS_CMDLINE) {
		strncpy(RELOC(void *, Multiboot_Cmdline), mi->mi_cmdline,
		    sizeof(Multiboot_Cmdline));
		midest->mi_cmdline = (char *)&Multiboot_Cmdline;
	}

	if (mi->mi_flags & MULTIBOOT_INFO_HAS_LOADER_NAME) {
		strncpy(RELOC(void *, Multiboot_Loader_Name),
		    mi->mi_loader_name, sizeof(Multiboot_Loader_Name));
		midest->mi_loader_name = (char *)&Multiboot_Loader_Name;
	}

	if (mi->mi_flags & MULTIBOOT_INFO_HAS_MMAP) {
		memcpy(RELOC(void *, Multiboot_Mmap),
		    (void *)mi->mi_mmap_addr, mi->mi_mmap_length);
		midest->mi_mmap_addr = (vaddr_t)&Multiboot_Mmap;
	}

	if (mi->mi_flags & MULTIBOOT_INFO_HAS_DRIVES) {
		memcpy(RELOC(void *, Multiboot_Drives),
		    (void *)mi->mi_drives_addr, mi->mi_drives_length);
		midest->mi_drives_addr = (vaddr_t)&Multiboot_Drives;
	}

	copy_syms(mi);
#undef RELOC
}

/* --------------------------------------------------------------------- */

/*
 * Sets up the kernel if it was booted by a Multiboot-compliant boot
 * loader.  This is executed just after the kernel has relocated itself.
 * At this point, executing any kind of code is safe, keeping in mind
 * that no devices have been initialized yet (not even the console!).
 */
void
multiboot_post_reloc(void)
{
	struct multiboot_info *mi;
	
	if (! Multiboot_Loader)
		return;

	mi = &Multiboot_Info;
	bootinfo.bi_nentries = 0;

	setup_memory(mi);
	setup_console(mi);
	setup_howto(mi);
	setup_bootpath(mi);
	setup_biosgeom(mi);
	setup_bootdisk(mi);
	setup_memmap(mi);
	setup_syms(mi);
}

/* --------------------------------------------------------------------- */

/*
 * Prints a summary of the information collected in the Multiboot
 * information header (if present).  Done as a separate function because
 * the console has to be available.
 */
void
multiboot_print_info(void)
{
	struct multiboot_info *mi = &Multiboot_Info;

	if (! Multiboot_Loader)
		return;

	printf("multiboot: Information structure flags: 0x%08x\n",
	    mi->mi_flags);

	if (mi->mi_flags & MULTIBOOT_INFO_HAS_LOADER_NAME)
		printf("multiboot: Boot loader: %s\n", mi->mi_loader_name);

	if (mi->mi_flags & MULTIBOOT_INFO_HAS_CMDLINE)
		printf("multiboot: Command line: %s\n", mi->mi_cmdline);

	if (mi->mi_flags & MULTIBOOT_INFO_HAS_MEMORY)
		printf("multiboot: %u KB lower memory, %u KB upper memory\n",
		    mi->mi_mem_lower, mi->mi_mem_upper);

	if (mi->mi_flags & MULTIBOOT_INFO_HAS_ELF_SYMS && esym == 0)
		printf("multiboot: Symbol table too big; try increasing "
		    "MULTIBOOT_SYMTAB_SPACE\n");
}

/* --------------------------------------------------------------------- */

/*
 * Adds the bootinfo entry given in 'item' to the bootinfo tables.
 * Sets the item type to 'type' and its length to 'len'.
 */
static void
bootinfo_add(struct btinfo_common *item, int type, int len)
{
	int i;
	struct bootinfo *bip = (struct bootinfo *)&bootinfo;
	vaddr_t data;

	item->type = type;
	item->len = len;

	data = (vaddr_t)&bip->bi_data;
	for (i = 0; i < bip->bi_nentries; i++) {
		struct btinfo_common *tmp;

		tmp = (struct btinfo_common *)data;
		data += tmp->len;
	}
	if (data + len < (vaddr_t)&bip->bi_data + sizeof(bip->bi_data)) {
		memcpy((void *)data, item, len);
		bip->bi_nentries++;
	}
}

/* --------------------------------------------------------------------- */

/*
 * Copies the symbol table and the strings table passed in by the boot
 * loader after the kernel's image, and sets up 'esym' accordingly so
 * that this data is properly copied into upper memory during relocation.
 *
 * WARNING: This code runs before the kernel has relocated itself.  See
 * the note in multiboot_pre_reloc() for more information.
 */
static void
copy_syms(struct multiboot_info *mi)
{
#define RELOC(type, x) ((type)((vaddr_t)(x) - KERNBASE))
	int i;
	Elf32_Shdr *symtabp, *strtabp;
	struct symbols_image *si;

	/*
	 * Check if the Multiboot information header has symbols or not.
	 */
	if (!(mi->mi_flags & MULTIBOOT_INFO_HAS_ELF_SYMS))
		return;

	/*
	 * Locate a symbol table and its matching string table in the
	 * section headers passed in by the boot loader.  Set 'symtabp'
	 * and 'strtabp' with pointers to the matching entries.
	 */
	symtabp = strtabp = NULL;
	for (i = 0; i < mi->mi_elfshdr_num && symtabp == NULL &&
	    strtabp == NULL; i++) {
		Elf32_Shdr *shdrp;

		shdrp = &((Elf32_Shdr *)mi->mi_elfshdr_addr)[i];

		if ((shdrp->sh_type & SHT_SYMTAB) &&
		    shdrp->sh_link != SHN_UNDEF) {
			Elf32_Shdr *shdrp2;

			shdrp2 = &((Elf32_Shdr *)mi->mi_elfshdr_addr)
			    [shdrp->sh_link];

			if (shdrp2->sh_type & SHT_STRTAB) {
				symtabp = shdrp;
				strtabp = shdrp2;
			}
		}
	}
	if (symtabp == NULL || strtabp == NULL)
		return;

	/*
	 * Check if the symbol table will fit after the kernel image.
	 * If not, ignore it; there is nothing else we can do (except
	 * maybe copying only part of the table... but that could be
	 * as useless as not copying it).
	 */
	if (sizeof(si) + symtabp->sh_size + strtabp->sh_size >
	    MULTIBOOT_SYMTAB_SPACE)
		return;

	/*
	 * Prepare space after the kernel to create a simple ELF image
	 * that holds the symbol table and the string table previously
	 * found.
	 *
	 * This goes just after the BSS section to let the bootstraping
	 * code relocate it (up to esym's value).
	 */
	memset(RELOC(char *, &end), 0, MULTIBOOT_SYMTAB_SPACE);
	si = RELOC(struct symbols_image *, &end);

	/*
	 * Create a minimal ELF header (as required by ksyms_init).
	 */
	memcpy(si->i_ehdr.e_ident, ELFMAG, SELFMAG);
	si->i_ehdr.e_ident[EI_CLASS] = ELFCLASS32;
	si->i_ehdr.e_type = ET_EXEC;
	si->i_ehdr.e_version = 1;
	si->i_ehdr.e_shoff = offsetof(struct symbols_image, i_shdr);
	si->i_ehdr.e_ehsize = sizeof(si->i_ehdr);
	si->i_ehdr.e_shentsize = sizeof(si->i_shdr[0]);
	si->i_ehdr.e_shnum = 2;
	si->i_ehdr.e_shstrndx = SHN_UNDEF;

	/*
	 * First section: default empty entry; cleared by the previous
	 * memset.  Explicitly set fields that use symbolic values.
	 */
	si->i_shdr[0].sh_type = SHT_NULL;
	si->i_shdr[0].sh_link = SHN_UNDEF;

	/*
	 * Second section: the symbol table.
	 */
	memcpy(&si->i_shdr[1], symtabp, sizeof(si->i_shdr[1]));
	si->i_shdr[1].sh_name = 1;
	si->i_shdr[1].sh_addr = (vaddr_t)(&end) +
	    offsetof(struct symbols_image, i_shdr[1]);
	si->i_shdr[1].sh_offset = offsetof(struct symbols_image, i_data);
	si->i_shdr[1].sh_link = 2;

	memcpy(RELOC(uint8_t *, (vaddr_t)(&end)) + si->i_shdr[1].sh_offset,
	    (void *)symtabp->sh_addr, symtabp->sh_size);

	/*
	 * Third section: the strings table.
	 */
	memcpy(&si->i_shdr[2], strtabp, sizeof(si->i_shdr[2]));
	si->i_shdr[1].sh_name = 9;
	si->i_shdr[2].sh_addr = (vaddr_t)(&end) +
	    offsetof(struct symbols_image, i_shdr[2]);
	si->i_shdr[2].sh_offset = offsetof(struct symbols_image, i_data) +
	    si->i_shdr[1].sh_size;
	si->i_shdr[2].sh_link = SHN_UNDEF;

	memcpy(RELOC(uint8_t *, (vaddr_t)(&end)) + si->i_shdr[2].sh_offset,
	    (void *)strtabp->sh_addr, strtabp->sh_size);

	/*
	 * Fourth section: the section header strings table used by this
	 * minimal ELF image.
	 */
	si->i_shdr[3].sh_name = 17;
	si->i_shdr[3].sh_type = SHT_STRTAB;
	si->i_shdr[3].sh_offset = offsetof(struct symbols_image, i_strtab);
	si->i_shdr[3].sh_size = sizeof(si->i_strtab);
	si->i_shdr[3].sh_addralign = 1;
	si->i_shdr[3].sh_link = SHN_UNDEF;

	strcpy(&si->i_strtab[1], ".symtab");
	strcpy(&si->i_strtab[9], ".strtab");
	strcpy(&si->i_strtab[17], ".shstrtab");

	/*
	 * Set up the 'esym' global variable to point to the end of the
	 * minimal ELF image (end of symbols).
	 */
	*RELOC(int *, &esym) = ((vaddr_t)&end) + si->i_shdr[2].sh_offset +
	    si->i_shdr[2].sh_size;
#undef RELOC
}

/* --------------------------------------------------------------------- */

/*
 * Sets up the biosgeom bootinfo structure if the Multiboot information
 * structure provides information about disk drives.
 */
static void
setup_biosgeom(struct multiboot_info *mi)
{
	size_t pos;
	uint8_t bidata[1024];
	struct btinfo_biosgeom *bi;

	if (!(mi->mi_flags & MULTIBOOT_INFO_HAS_DRIVES))
		return;

	memset(bidata, 0, sizeof(bidata));
	bi = (struct btinfo_biosgeom *)bidata;
	pos = 0;

	while (pos < mi->mi_drives_length) {
		struct multiboot_drive *md;
		struct bi_biosgeom_entry bbe;

		md = (struct multiboot_drive *)
		    &((uint8_t *)mi->mi_drives_addr)[pos];

		memset(&bbe, 0, sizeof(bbe));
		bbe.sec = md->md_sectors;
		bbe.head = md->md_heads;
		bbe.cyl = md->md_cylinders;
		bbe.dev = md->md_number;

		memcpy(&bi->disk[bi->num], &bbe, sizeof(bbe));
		bi->num++;

		pos += md->md_length;
	}

	bootinfo_add((struct btinfo_common *)bi, BTINFO_BIOSGEOM,
	    sizeof(struct btinfo_biosgeom) +
	    bi->num * sizeof(struct bi_biosgeom_entry));
}

/* --------------------------------------------------------------------- */

/*
 * Sets up the default root device if the Multiboot information
 * structure provides information about the boot drive (where the kernel
 * image was loaded from) or if the user gave a 'root' parameter on the
 * boot command line.
 */
static void
setup_bootdisk(struct multiboot_info *mi)
{
	boolean_t found;
	struct btinfo_rootdevice bi;

	found = FALSE;

	if (mi->mi_flags & MULTIBOOT_INFO_HAS_CMDLINE)
		found = optstr_get(mi->mi_cmdline, "root", bi.devname,
		    sizeof(bi.devname));

	if (!found && (mi->mi_flags & MULTIBOOT_INFO_HAS_BOOT_DEVICE)) {
		const char *devprefix;

		/* XXX: This should use x86_alldisks to find the correct
		 * match.  But, at this point, neither the drivers nor the
		 * vector are initialized... */
		switch (mi->mi_boot_device_drive) {
		case 0x00:	devprefix = "fd0";	break;
		case 0x01:	devprefix = "fd1";	break;
		case 0x80:	devprefix = "wd0";	break;
		case 0x81:	devprefix = "wd1";	break;
		case 0x82:	devprefix = "wd2";	break;
		case 0x83:	devprefix = "wd3";	break;
		default:	devprefix = "";
		}

		if (devprefix[0] != '\0') {
			strcpy(bi.devname, devprefix);
			if (mi->mi_boot_device_part2 != 0xFF)
				bi.devname[3] = mi->mi_boot_device_part2 + 'a';
			else
				bi.devname[3] = 'a';
			bi.devname[4] = '\0';

			found = TRUE;
		}
	}

	if (found) {
		bootinfo_add((struct btinfo_common *)&bi, BTINFO_ROOTDEVICE,
		    sizeof(struct btinfo_rootdevice));
	}
}

/* --------------------------------------------------------------------- */

/*
 * Sets up the bootpath bootinfo structure with an appropriate kernel
 * name derived from the boot command line.  The Multiboot information
 * structure does not provide this detail directly, so we try to derive
 * it from the command line setting.
 */
static void
setup_bootpath(struct multiboot_info *mi)
{
	struct btinfo_bootpath bi;
	char *cl, *cl2, old;
	int len;

	if (strncmp(Multiboot_Loader_Name, "GNU GRUB ",
	    sizeof(Multiboot_Loader_Name)) > 0) {
		cl = mi->mi_cmdline;
		while (*cl != '\0' && *cl != '/')
			cl++;
		cl2 = cl;
		len = 0;
		while (*cl2 != '\0' && *cl2 != ' ') {
			len++;
			cl2++;
		}

		old = *cl2;
		*cl2 = '\0';
		memcpy(bi.bootpath, cl, MIN(sizeof(bi.bootpath), len));
		*cl2 = old;
		bi.bootpath[MIN(sizeof(bi.bootpath), len)] = '\0';

		bootinfo_add((struct btinfo_common *)&bi, BTINFO_BOOTPATH,
		    sizeof(struct btinfo_bootpath));
	}
}

/* --------------------------------------------------------------------- */

/*
 * Sets up the console bootinfo structure if the user gave a 'console'
 * argument on the boot command line.  The Multiboot information
 * structure gives no hint about this, so the only way to know where the
 * console is is to let the user specify it.
 *
 * If there wasn't any 'console' argument, this does not generate any
 * bootinfo entry, falling back to the kernel's default console.
 *
 * If there weren't any of 'console_speed' or 'console_addr' arguments,
 * this falls back to the default values for the serial port.
 */
static void
setup_console(struct multiboot_info *mi)
{
	struct btinfo_console bi;
	boolean_t found;

	found = FALSE;

	if (mi->mi_flags & MULTIBOOT_INFO_HAS_CMDLINE)
		found = optstr_get(mi->mi_cmdline, "console", bi.devname,
		    sizeof(bi.devname));

	if (found) {
		if (strncmp(bi.devname, "com", sizeof(bi.devname)) == 0) {
			char tmp[10];

			found = optstr_get(mi->mi_cmdline, "console_speed",
			    tmp, sizeof(tmp));
			if (found)
				bi.speed = strtoul(tmp, NULL, 10);
			else
				bi.speed = 0; /* Use default speed. */

			found = optstr_get(mi->mi_cmdline, "console_addr",
			    tmp, sizeof(tmp));
			if (found) {
				if (tmp[0] == '0' && tmp[1] == 'x')
					bi.addr = strtoul(tmp + 2, NULL, 16);
				else
					bi.addr = strtoul(tmp, NULL, 10);
			} else
				bi.addr = 0; /* Use default address. */
		}

		bootinfo_add((struct btinfo_common *)&bi, BTINFO_CONSOLE,
		    sizeof(struct btinfo_console));
	}
}

/* --------------------------------------------------------------------- */

/*
 * Sets up the 'boothowto' variable based on the options given in the
 * boot command line, if any.
 */
static void
setup_howto(struct multiboot_info *mi)
{
	char *cl;

	if (!(mi->mi_flags & MULTIBOOT_INFO_HAS_CMDLINE))
		return;

	cl = mi->mi_cmdline;

	/* Skip kernel file name. */
	while (*cl != '\0' && *cl != ' ')
		cl++;
	while (*cl != '\0' && *cl == ' ')
		cl++;

	/* Check if there are flags and set 'howto' accordingly. */
	if (*cl == '-') {
		int howto = 0;

		cl++;
		while (*cl != '\0' && *cl != ' ') {
			BOOT_FLAG(*cl, howto);
			cl++;
		}
		if (*cl == ' ')
			cl++;

		boothowto = howto;
	}
}

/* --------------------------------------------------------------------- */

/*
 * Sets up the memmap bootinfo structure to describe available memory as
 * given by the BIOS.
 */
static void
setup_memmap(struct multiboot_info *mi)
{
	char data[1024];
	size_t i;
	struct btinfo_memmap *bi;

	if (!(mi->mi_flags & MULTIBOOT_INFO_HAS_MMAP))
		return;

	bi = (struct btinfo_memmap *)data;
	bi->num = 0;

	i = 0;
	while (i < mi->mi_mmap_length) {
		struct multiboot_mmap *mm;
		struct bi_memmap_entry *bie;

		bie = &bi->entry[bi->num];

		mm = (struct multiboot_mmap *)(mi->mi_mmap_addr + i);
		bie->addr = mm->mm_base_addr;
		bie->size = mm->mm_length;
		if (mm->mm_type == 1)
			bie->type = BIM_Memory;
		else
			bie->type = BIM_Reserved;

		bi->num++;
		i += mm->mm_size + 4;
	}

	bootinfo_add((struct btinfo_common *)bi, BTINFO_MEMMAP,
	    sizeof(data));
}

/* --------------------------------------------------------------------- */

/*
 * Sets up the 'biosbasemem' and 'biosextmem' variables if the
 * Multiboot information structure provides information about memory.
 */
static void
setup_memory(struct multiboot_info *mi)
{
	if (!(mi->mi_flags & MULTIBOOT_INFO_HAS_MEMORY))
		return;

	biosbasemem = mi->mi_mem_lower;
	biosextmem = mi->mi_mem_upper;
}

/* --------------------------------------------------------------------- */

/*
 * Sets up the symtab bootinfo structure to describe where the symbols
 * are if copy_syms() found them.
 */
static void
setup_syms(struct multiboot_info *mi)
{
	struct btinfo_symtab bi;
	struct symbols_image *si;

	if (esym == 0)
		return;

	si = (struct symbols_image *)(&end);

	bi.ssym = (int)(&end) - KERNBASE;
	bi.esym = (int)esym - KERNBASE;
	bi.nsym = si->i_shdr[1].sh_size / sizeof(Elf32_Sym);

	bootinfo_add((struct btinfo_common *)&bi, BTINFO_SYMTAB,
	    sizeof(struct btinfo_symtab));
}
