/*	$NetBSD: multiboot2.c,v 1.8 2021/10/07 12:52:27 msaitoh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: multiboot2.c,v 1.8 2021/10/07 12:52:27 msaitoh Exp $");

#include "opt_multiboot.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cdefs_elf.h>
#include <sys/boot_flag.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>
#include <sys/optstr.h>
#include <sys/ksyms.h>
#include <sys/common_ansi.h>
#include <sys/intr.h>

#include <x86/efi.h>

#include <machine/bootinfo.h>
#include <arch/i386/include/multiboot2.h>

#ifdef _LOCORE64
typedef uint64_t   locore_vaddr_t;
typedef Elf64_Shdr locore_Elf_Shdr;
typedef Elf64_Word locore_Elf_Word;
typedef Elf64_Addr locore_Elf_Addr;
#else
typedef vaddr_t   locore_vaddr_t;
typedef Elf_Shdr locore_Elf_Shdr;
typedef Elf_Word locore_Elf_Word;
typedef Elf_Addr locore_Elf_Addr;
#endif

#if !defined(MULTIBOOT)
#  error "MULTIBOOT not defined; this cannot happen."
#endif

/*
 * This is an attempt to get an early debug output. It
 * requires EFI Boot Services, and it does not work anyway:
 * it causes EFI to try to handle interrupts while the kernel
 * already took that over, hence we get a reboot.
 */
#define BS_PRINT(efisystbl, wstring)					\
	efi_systbl->st_coutif->ei_outputstring(efi_systbl->st_coutif,	\
	    (efi_char *)__UNCONST(wstring))

struct multiboot_symbols {
	uint32_t s_symstart;
	uint32_t s_symsize;
	uint32_t s_strstart;
	uint32_t s_strsize;
};

void multiboot2_copy_syms(struct multiboot_tag_elf_sections *,
			  struct multiboot_symbols *,
			  bool *, int **, void *, vaddr_t);
/*
 * Because of clashes between multiboot.h and multiboot2.h we
 * cannot include both, and we need to redefine here:
 */
void            multiboot2_pre_reloc(char *);
void            multiboot2_post_reloc(void);
void            multiboot2_print_info(void);
bool            multiboot2_ksyms_addsyms_elf(void);

extern int              biosbasemem;
extern int              biosextmem;
#ifdef __i386__
extern int              biosmem_implicit;
#endif
extern int              boothowto;
extern struct bootinfo  bootinfo;
extern int              end;
extern int *            esym;
extern char             start;

/*
 * There is no way to perform dynamic allocation
 * at this time, hence we need to waste memory,
 * with the hope data will fit.
 */
char multiboot_info[16384] = "\0\0\0\0";
bool multiboot2_enabled = false;
bool has_syms = false;
struct multiboot_symbols Multiboot_Symbols;

#define RELOC(type, x) ((type)((vaddr_t)(x) - KERNBASE))

static void
efi_exit_bs(struct efi_systbl *efi_systbl, void *efi_ih)
{
	struct efi_bs *efi_bs;
	struct efi_md *desc;
	uintn bufsize, key, size;
	uint32_t vers;

	if (efi_systbl == NULL)
		panic("EFI system table is NULL");

	if (efi_systbl->st_hdr.th_sig != EFI_SYSTBL_SIG)
		panic("EFI system table signature is wrong");

	efi_bs = efi_systbl->st_bs;

	if (efi_bs == NULL)
		panic("EFI BS is NULL");

	if (efi_bs->bs_hdr.th_sig != EFI_BS_SIG)
		panic("EFI BS signature is wrong");

	if (efi_ih == NULL)
		panic("EFI IH is NULL");

	bufsize = 16384;

	if (efi_bs->bs_allocatepool(EFI_MD_TYPE_DATA,
	    bufsize, (void **)&desc) != 0)
		panic("EFI AllocatePool failed");

	if (efi_bs->bs_getmemorymap(&bufsize, desc, &key, &size, &vers) == 0)
		goto exit_bs;

	(void)efi_bs->bs_freepool((void *)desc);

	if (efi_bs->bs_allocatepool(EFI_MD_TYPE_DATA,
	    bufsize, (void **)&desc) != 0)
		panic("EFI AllocatePool failed");

	if (efi_bs->bs_getmemorymap(&bufsize, desc, &key, &size, &vers) != 0)
		panic("EFI GetMemoryMap failed");

exit_bs:
	if (efi_bs->bs_exitbootservices(efi_ih, key) != 0)
		panic("EFI ExitBootServices failed");

	return;
}

void
multiboot2_copy_syms(struct multiboot_tag_elf_sections *mbt_elf,
		     struct multiboot_symbols *ms,
		     bool *has_symsp, int **esymp, void *endp,
		     vaddr_t kernbase)
{
	int i;
	locore_Elf_Shdr *symtabp, *strtabp;
	locore_Elf_Word symsize, strsize;
	locore_Elf_Addr symaddr, straddr;
	locore_Elf_Addr symstart, strstart;
	locore_Elf_Addr cp1src, cp1dst;
	locore_Elf_Word cp1size;
	locore_Elf_Addr cp2src, cp2dst;
	locore_Elf_Word cp2size;

	/*
	 * Locate a symbol table and its matching string table in the
	 * section headers passed in by the boot loader.  Set 'symtabp'
	 * and 'strtabp' with pointers to the matching entries.
	 */
	symtabp = strtabp = NULL;
	for (i = 0; i < mbt_elf->num && symtabp == NULL &&
	    strtabp == NULL; i++) {
		locore_Elf_Shdr *shdrp;

		shdrp = &((locore_Elf_Shdr *)mbt_elf->sections)[i];

		if ((shdrp->sh_type == SHT_SYMTAB) &&
		    shdrp->sh_link != SHN_UNDEF) {
			locore_Elf_Shdr *shdrp2;

			shdrp2 = &((locore_Elf_Shdr *)mbt_elf->sections)
			    [shdrp->sh_link];

			if (shdrp2->sh_type == SHT_STRTAB) {
				symtabp = (locore_Elf_Shdr *)shdrp;
				strtabp = (locore_Elf_Shdr *)shdrp2;
			}
		}
	}
	if (symtabp == NULL || strtabp == NULL)
		return;

	symaddr = symtabp->sh_addr;
	straddr = strtabp->sh_addr;
	symsize = symtabp->sh_size;
	strsize = strtabp->sh_size;

	/*
	 * Copy the symbol and string tables just after the kernel's
	 * end address, in this order.  Only the contents of these ELF
	 * sections are copied; headers are discarded.  esym is later
	 * updated to point to the lowest "free" address after the tables
	 * so that they are mapped appropriately when enabling paging.
	 *
	 * We need to be careful to not overwrite valid data doing the
	 * copies, hence all the different cases below.  We can assume
	 * that if the tables start before the kernel's end address,
	 * they will not grow over this address.
	 */
	if ((void *)(uintptr_t)symaddr < endp &&
	    (void *)(uintptr_t)straddr < endp) {
		cp1src = symaddr; cp1size = symsize;
		cp2src = straddr; cp2size = strsize;
	} else if ((void *)(uintptr_t)symaddr > endp &&
		   (void *)(uintptr_t)straddr < endp) {
		cp1src = symaddr; cp1size = symsize;
		cp2src = straddr; cp2size = strsize;
	} else if ((void *)(uintptr_t)symaddr < endp &&
		   (void *)(uintptr_t)straddr > endp) {
		cp1src = straddr; cp1size = strsize;
		cp2src = symaddr; cp2size = symsize;
	} else {
		/* symaddr and straddr are both over end */
		if (symaddr < straddr) {
			cp1src = symaddr; cp1size = symsize;
			cp2src = straddr; cp2size = strsize;
		} else {
			cp1src = straddr; cp1size = strsize;
			cp2src = symaddr; cp2size = symsize;
		}
	}

	cp1dst = (locore_Elf_Addr)(uintptr_t)endp;
	cp2dst = (locore_Elf_Addr)(uintptr_t)endp + cp1size;

	(void)memcpy((void *)(uintptr_t)cp1dst,
		     (void *)(uintptr_t)cp1src, cp1size);
	(void)memcpy((void *)(uintptr_t)cp2dst,
		     (void *)(uintptr_t)cp2src, cp2size);

	symstart = (cp1src == symaddr) ? cp1dst : cp2dst;
	strstart = (cp1src == straddr) ? cp1dst : cp2dst;

	ms->s_symstart = symstart + kernbase;
	ms->s_symsize  = symsize;
	ms->s_strstart = strstart + kernbase;
	ms->s_strsize  = strsize;

	*has_symsp = true;
	*esymp = (int *)((uintptr_t)endp + symsize + strsize + kernbase);

}

void
multiboot2_pre_reloc(char *mbi)
{
	uint32_t mbi_size;
	void *mbidest = RELOC(void *, multiboot_info);
	char *cp;
	struct multiboot_tag_module *mbt;
	struct multiboot_tag_elf_sections *mbt_elf = NULL;
	struct efi_systbl *efi_systbl = NULL;
	void *efi_ih = NULL;
	bool has_bs = false;

	mbi_size = *(uint32_t *)mbi;
	if (mbi_size < sizeof(multiboot_info))
		memcpy(mbidest, mbi, mbi_size);
	else
		panic("multiboot_info too big"); /* will not show up */

	*RELOC(bool *, &multiboot2_enabled) = true;

	for (cp = mbi + (2 * sizeof(uint32_t));
	     cp - mbi < mbi_size;
	     cp += roundup(mbt->size, MULTIBOOT_INFO_ALIGN)) {
		mbt = (struct multiboot_tag_module *)cp;
		switch (mbt->type) {
		case MULTIBOOT_TAG_TYPE_ELF_SECTIONS:
			mbt_elf = (struct multiboot_tag_elf_sections *)mbt;
			break;
#ifdef __LP64__
		case MULTIBOOT_TAG_TYPE_EFI64:
			efi_systbl = (struct efi_systbl *)
				((struct multiboot_tag_efi64 *)mbt)->pointer;
			break;
		case MULTIBOOT_TAG_TYPE_EFI64_IH:
			efi_ih = (void *)
			    ((struct multiboot_tag_efi64_ih *)mbt)->pointer;
			break;
#else
		case MULTIBOOT_TAG_TYPE_EFI32:
			efi_systbl = (struct efi_systbl *)
				((struct multiboot_tag_efi32 *)mbt)->pointer;
			break;
		case MULTIBOOT_TAG_TYPE_EFI32_IH:
			efi_ih = (void *)
			    ((struct multiboot_tag_efi32_ih *)mbt)->pointer;
			break;
#endif
		case MULTIBOOT_TAG_TYPE_EFI_BS:
#if notyet
			has_bs = true;
#endif
			break;
		default:
			break;
		}
	}

	/* Broken */
	if (has_bs)
		efi_exit_bs(efi_systbl, efi_ih);

	if (mbt_elf)
		multiboot2_copy_syms(mbt_elf,
		    RELOC(struct multiboot_symbols *, &Multiboot_Symbols),
		    RELOC(bool *, &has_syms),
		    RELOC(int **, &esym),
		    RELOC(void *, &end),
		    KERNBASE);

	return;
}

static struct btinfo_common *
bootinfo_init(int type, int len)
{
	int i;
	struct bootinfo *bip = (struct bootinfo *)&bootinfo;
	vaddr_t data;

	data = (vaddr_t)&bip->bi_data;
	for (i = 0; i < bip->bi_nentries; i++) {
		struct btinfo_common *tmp;

		tmp = (struct btinfo_common *)data;
		data += tmp->len;
	}
	if (data + len < (vaddr_t)&bip->bi_data + sizeof(bip->bi_data)) {
		/* Initialize the common part */
		struct btinfo_common *item = (struct btinfo_common *)data;
		item->type = type;
		item->len = len;
		bip->bi_nentries++;
		return item;
	} else {
		return NULL;
	}
}

static void
bootinfo_add(struct btinfo_common *item, int type, int len)
{
	struct btinfo_common *bip = bootinfo_init(type, len);
	if (bip == NULL)
		return;

	/* Copy the data after the common part over */
	memcpy(&bip[1], &item[1], len - sizeof(*item));
}

static void
mbi_cmdline(struct multiboot_tag_string *mbt)
{
	char *cmdline = mbt->string;
	struct btinfo_console bic;
	struct btinfo_rootdevice bir;
	char *cl;

	if (optstr_get(cmdline, "console", bic.devname, sizeof(bic.devname))) {
		if (strncmp(bic.devname, "com", sizeof(bic.devname)) == 0) {
			char opt[10];

			if (optstr_get(cmdline, "console_speed",
				       opt, sizeof(opt)))
				bic.speed = strtoul(opt, NULL, 10);
			else
				bic.speed = 0; /* Use default speed. */

			if (optstr_get(cmdline, "console_addr",
				       opt, sizeof(opt))) {
				if (opt[0] == '0' && opt[1] == 'x')
					bic.addr = strtoul(opt + 2, NULL, 16);
				else
					bic.addr = strtoul(opt, NULL, 10);
			} else {
				bic.addr = 0; /* Use default address. */
			}

			bootinfo_add((struct btinfo_common *)&bic,
				     BTINFO_CONSOLE, sizeof(bic));

		}

		if (strncmp(bic.devname, "pc", sizeof(bic.devname)) == 0)
			bootinfo_add((struct btinfo_common *)&bic,
				     BTINFO_CONSOLE, sizeof(bic));
	}

	if (optstr_get(cmdline, "root", bir.devname, sizeof(bir.devname)))
		bootinfo_add((struct btinfo_common *)&bir, BTINFO_ROOTDEVICE,
		    sizeof(bir));

	/*
	 * Parse boot flags (-s and friends)
	 */
	cl = cmdline;

	/* Skip kernel file name. */
	while (*cl != '\0' && *cl != ' ')
		cl++;
	while (*cl == ' ')
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

	return;
}

static void
mbi_modules(char *mbi, uint32_t mbi_size, int module_count)
{
	char *cp;
	struct multiboot_tag_module *mbt;
	size_t bim_len;
	struct bi_modulelist_entry *bie;
	struct btinfo_modulelist *bim;

	bim_len = sizeof(*bim) + (module_count * sizeof(*bie));
	bim = (struct btinfo_modulelist *)bootinfo_init(BTINFO_MODULELIST,
	    bim_len);
	if (bim == NULL)
		return;

	bim->num = module_count;
	bim->endpa = end;

	bie = (struct bi_modulelist_entry *)(bim + 1);

	for (cp = mbi + (2 * sizeof(uint32_t));
	     cp - mbi < mbi_size;
	     cp += roundup(mbt->size, MULTIBOOT_INFO_ALIGN)) {
		mbt = (struct multiboot_tag_module *)cp;
		if (mbt->type != MULTIBOOT_TAG_TYPE_MODULE)
			continue;

		strncpy(bie->path, mbt->cmdline, sizeof(bie->path));
		bie->type = BI_MODULE_ELF;
		bie->len = mbt->mod_end - mbt->mod_start;
		bie->base = mbt->mod_start;

		bie++;
	}
}

static void
mbi_basic_meminfo(struct multiboot_tag_basic_meminfo *mbt)
{
	/* Make sure we don't override user-set variables. */
	if (biosbasemem == 0) {
		biosbasemem = mbt->mem_lower;
#ifdef __i386__
		biosmem_implicit = 1;
#endif
	}
	if (biosextmem == 0) {
		biosextmem = mbt->mem_upper;
#ifdef __i386__
		biosmem_implicit = 1;
#endif
	}

	return;
}

static void
mbi_bootdev(struct multiboot_tag_bootdev *mbt)
{
	struct btinfo_bootdisk bid;

	bid.labelsector = -1;
	bid.biosdev = mbt->biosdev;
	bid.partition = mbt->slice;

	bootinfo_add((struct btinfo_common *)&bid,
	   BTINFO_BOOTDISK, sizeof(bid));
}

static void
mbi_mmap(struct multiboot_tag_mmap *mbt)
{
	struct btinfo_memmap *bim;
	int num;
	char *cp;

	if (mbt->entry_version != 0)
		return;

	/* Determine size */
	num = 0;
	for (cp = (char *)(mbt + 1);
	     cp - (char *)mbt < mbt->size;
	     cp += mbt->entry_size) {
		num++;
	}

	bim = (struct btinfo_memmap *)bootinfo_init(BTINFO_MEMMAP,
	    sizeof(num) + num * sizeof(struct bi_memmap_entry));
	if (bim == NULL)
		return;
	bim->num = 0;

	for (cp = (char *)(mbt + 1);
	     cp - (char *)mbt < mbt->size;
	     cp += mbt->entry_size) {
		struct multiboot_mmap_entry *mbe;
		struct bi_memmap_entry *bie;

		mbe = (struct multiboot_mmap_entry *)cp;
		bie = &bim->entry[bim->num];

		bie->addr = mbe->addr;
		bie->size = mbe->len;
		switch (mbe->type) {
		case MULTIBOOT_MEMORY_AVAILABLE:
			bie->type = BIM_Memory;
			break;
		case MULTIBOOT_MEMORY_RESERVED:
			bie->type = BIM_Reserved;
			break;
		case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
			bie->type = BIM_ACPI;
			break;
		case MULTIBOOT_MEMORY_NVS:
			bie->type = BIM_NVS;
			break;
		case MULTIBOOT_MEMORY_BADRAM:
		default:
			bie->type = BIM_Unusable;
			break;
		}

		bim->num++;
	}

	KASSERT(bim->num == num);
}

static void
mbi_vbe(struct multiboot_tag_vbe *mbt, struct btinfo_framebuffer *bif)
{
	bif->vbemode = mbt->vbe_mode;
	return;
}

static void
mbi_framebuffer(struct multiboot_tag_framebuffer *mbt,
     struct btinfo_framebuffer *bif)
{
	bif->physaddr = mbt->common.framebuffer_addr;
	bif->width = mbt->common.framebuffer_width;
	bif->height = mbt->common.framebuffer_height;
	bif->depth = mbt->common.framebuffer_bpp;
	bif->stride = mbt->common.framebuffer_pitch;

	return;
}

static void
mbi_efi32(struct multiboot_tag_efi32 *mbt)
{
	struct btinfo_efi bie;

	bie.systblpa = mbt->pointer;
	bie.flags = BI_EFI_32BIT;

	bootinfo_add((struct btinfo_common *)&bie, BTINFO_EFI, sizeof(bie));
}

static void
mbi_efi64(struct multiboot_tag_efi64 *mbt)
{
	struct btinfo_efi bie;

	bie.systblpa = mbt->pointer;

	bootinfo_add((struct btinfo_common *)&bie, BTINFO_EFI, sizeof(bie));
}

static void
mbi_efi_mmap(struct multiboot_tag_efi_mmap *mbt)
{
	struct btinfo_efimemmap *bie;
	size_t bie_len;

	if (mbt->descr_vers != 0)
		return;

	bie_len = sizeof(*bie) + mbt->size - sizeof(*mbt);
	bie = (struct btinfo_efimemmap *)bootinfo_init(BTINFO_EFIMEMMAP,
	    bie_len);
	if (bie == NULL)
		return;

	bie->num = (mbt->size - sizeof(*mbt)) / mbt->descr_size;
	bie->version = mbt->descr_vers;
	bie->size = mbt->descr_size;
	memcpy(bie->memmap, mbt + 1, mbt->size - sizeof(*mbt));
}

void
multiboot2_post_reloc(void)
{
	uint32_t mbi_size;
	struct multiboot_tag *mbt;
	char *mbi = multiboot_info;
	char *cp;
	int module_count = 0;
	struct btinfo_framebuffer fbinfo;
	bool has_fbinfo = false;

	if (multiboot2_enabled == false)
		goto out;

	mbi_size = *(uint32_t *)multiboot_info;
	if (mbi_size < 2 * sizeof(uint32_t))
		goto out;

	bootinfo.bi_nentries = 0;

	memset(&fbinfo, 0, sizeof(fbinfo));

	for (cp = mbi + (2 * sizeof(uint32_t));
	     cp - mbi < mbi_size;
	     cp += roundup(mbt->size, MULTIBOOT_INFO_ALIGN)) {
		mbt = (struct multiboot_tag *)cp;
		switch (mbt->type) {
		case MULTIBOOT_TAG_TYPE_CMDLINE:
			mbi_cmdline((void *)mbt);
			break;
		case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
			break;
		case MULTIBOOT_TAG_TYPE_MMAP:
			mbi_mmap((void *)mbt);
			break;
		case MULTIBOOT_TAG_TYPE_MODULE:
			module_count++;
			break;
		case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:
			mbi_basic_meminfo((void *)mbt);
			break;
		case MULTIBOOT_TAG_TYPE_BOOTDEV:
			mbi_bootdev((void *)mbt);
			break;
		case MULTIBOOT_TAG_TYPE_VBE:
			mbi_vbe((void *)mbt, &fbinfo);
			break;
		case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:
			mbi_framebuffer((void *)mbt, &fbinfo);
			has_fbinfo = true;
			break;
		case MULTIBOOT_TAG_TYPE_ELF_SECTIONS:
		case MULTIBOOT_TAG_TYPE_APM:
			break;
		case MULTIBOOT_TAG_TYPE_EFI32:
			mbi_efi32((void *)mbt);
			break;
		case MULTIBOOT_TAG_TYPE_EFI64:
			mbi_efi64((void *)mbt);
			break;
		case MULTIBOOT_TAG_TYPE_SMBIOS:
		case MULTIBOOT_TAG_TYPE_ACPI_OLD:
		case MULTIBOOT_TAG_TYPE_ACPI_NEW:
		case MULTIBOOT_TAG_TYPE_NETWORK:
			break;
		case MULTIBOOT_TAG_TYPE_EFI_MMAP:
			mbi_efi_mmap((void *)mbt);
			break;
		case MULTIBOOT_TAG_TYPE_EFI_BS:
		case MULTIBOOT_TAG_TYPE_EFI32_IH:
		case MULTIBOOT_TAG_TYPE_EFI64_IH:
		case MULTIBOOT_TAG_TYPE_LOAD_BASE_ADDR:
		case MULTIBOOT_TAG_TYPE_END:
		default:
			break;
		}
	}

	if (has_fbinfo)
		bootinfo_add((struct btinfo_common *)&fbinfo,
		    BTINFO_FRAMEBUFFER, sizeof(fbinfo));

	if (module_count > 0)
		mbi_modules(mbi, mbi_size, module_count);

out:
	return;
}


#ifdef DEBUG
static void
mbi_hexdump(char *addr, size_t len)
{
	int i,j;

	for (i = 0; i < len; i += 16) {
		printf("  %p ", addr + i);
		for (j = 0; j < 16 && i + j < len; j++) {
			char *cp = addr + i + j;
			printf("%s%s%x",
			       (i+j) % 4 ? "" : " ",
			       (unsigned char)*cp < 0x10 ? "0" : "",
			       (unsigned char)*cp);
		}
		printf("\n");
	}

	return;
}
#endif

static const char *
mbi_tag_name(uint32_t type)
{
	const char *tag_name;

	switch (type) {
	case MULTIBOOT_TAG_TYPE_END:
		tag_name = "";
		break;
	case MULTIBOOT_TAG_TYPE_CMDLINE:
		tag_name = "command line"; break;
	case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
		tag_name = "boot loader name"; break;
	case MULTIBOOT_TAG_TYPE_MODULE:
		tag_name = "module"; break;
	case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:
		tag_name = "basic meminfo"; break;
	case MULTIBOOT_TAG_TYPE_BOOTDEV:
		tag_name = "boot device"; break;
	case MULTIBOOT_TAG_TYPE_MMAP:
		tag_name = "memory map"; break;
	case MULTIBOOT_TAG_TYPE_VBE:
		tag_name = "VESA BIOS Extensions"; break;
	case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:
		tag_name = "framebuffer"; break;
	case MULTIBOOT_TAG_TYPE_ELF_SECTIONS:
		tag_name = "ELF sections"; break;
	case MULTIBOOT_TAG_TYPE_APM:
		tag_name = "APM"; break;
	case MULTIBOOT_TAG_TYPE_EFI32:
		tag_name = "EFI system table"; break;
	case MULTIBOOT_TAG_TYPE_EFI64:
		tag_name = "EFI system table"; break;
	case MULTIBOOT_TAG_TYPE_SMBIOS:
		tag_name = "SMBIOS"; break;
	case MULTIBOOT_TAG_TYPE_ACPI_OLD:
		tag_name = "ACPI 2"; break;
	case MULTIBOOT_TAG_TYPE_ACPI_NEW:
		tag_name = "ACPI 3"; break;
	case MULTIBOOT_TAG_TYPE_NETWORK:
		tag_name = "network"; break;
	case MULTIBOOT_TAG_TYPE_EFI_MMAP:
		tag_name = "EFI memory map"; break;
	case MULTIBOOT_TAG_TYPE_EFI_BS:
		tag_name = "EFI boot services available"; break;
	case MULTIBOOT_TAG_TYPE_EFI32_IH:
		tag_name = "EFI ImageHandle"; break;
	case MULTIBOOT_TAG_TYPE_EFI64_IH:
		tag_name = "EFI ImaheHandle"; break;
	case MULTIBOOT_TAG_TYPE_LOAD_BASE_ADDR:
		tag_name = "load base"; break;
	default:
		tag_name = ""; break;
	}

	return tag_name;
}

void
multiboot2_print_info(void)
{
	struct multiboot_tag *mbt;
	char *cp;
	uint32_t total_size;
	uint32_t reserved;
#ifdef DEBUG
	int i = 0;
#endif

	if (multiboot2_enabled == false)
		goto out;

	total_size = *(uint32_t *)multiboot_info;
	reserved = *(uint32_t *)multiboot_info + 1;
	mbt = (struct multiboot_tag *)(uint32_t *)multiboot_info + 2;

	for (cp = multiboot_info + sizeof(total_size) + sizeof(reserved);
	     cp - multiboot_info < total_size;
	     cp = cp + roundup(mbt->size, MULTIBOOT_TAG_ALIGN)) {
		const char *tag_name;

		mbt = (struct multiboot_tag *)cp;
		tag_name = mbi_tag_name(mbt->type);

#ifdef DEBUG
		printf("multiboot2: tag[%d].type = %d(%s), .size = %d ",
		    i++, mbt->type, tag_name, mbt->size);
#else
		if (*tag_name == '\0')
			break;

		printf("multiboot2: %s ", mbi_tag_name(mbt->type));
#endif

		switch (mbt->type) {
		case MULTIBOOT_TAG_TYPE_CMDLINE:
			printf("%s\n",
			    ((struct multiboot_tag_string *)mbt)->string);
			break;
		case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
			printf("%s\n",
			    ((struct multiboot_tag_string *)mbt)->string);
			break;
		case MULTIBOOT_TAG_TYPE_MODULE:
			printf("0x%08x - 0x%08x %s\n",
			    ((struct multiboot_tag_module *)mbt)->mod_start,
			    ((struct multiboot_tag_module *)mbt)->mod_end,
			    ((struct multiboot_tag_module *)mbt)->cmdline);
			break;
		case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO: {
			struct multiboot_tag_basic_meminfo *meminfo;

			meminfo = (struct multiboot_tag_basic_meminfo *)mbt;
			printf("ower = %uKB, upper = %uKB\n",
			    meminfo->mem_lower, meminfo->mem_upper);
			break;
		}
		case MULTIBOOT_TAG_TYPE_BOOTDEV:
			printf ("biosdev = 0x%x, slice = %d, part = %d\n",
			    ((struct multiboot_tag_bootdev *)mbt)->biosdev,
			    ((struct multiboot_tag_bootdev *)mbt)->slice,
			    ((struct multiboot_tag_bootdev *)mbt)->part);
			break;
		case MULTIBOOT_TAG_TYPE_MMAP: {
			struct multiboot_tag_mmap *memmap;
			multiboot_memory_map_t *mmap;
			uint32_t entry_size;
			uint32_t entry_version;

			memmap = (struct multiboot_tag_mmap *)mbt;
			entry_size = memmap->entry_size;
			entry_version = memmap->entry_version;
			printf ("entry version = %d\n", entry_version);

			if (entry_version != 0)
				break;

			for (mmap = ((struct multiboot_tag_mmap *)mbt)->entries;
			    (char *)mmap - (char *)mbt < mbt->size;
			    mmap = (void *)((char *)mmap + entry_size))
				printf("  0x%016"PRIx64" @ 0x%016"PRIx64" "
				    "type 0x%x\n",
				    (uint64_t)mmap->len, (uint64_t)mmap->addr,
				    mmap->type);
			break;
		}
		case MULTIBOOT_TAG_TYPE_FRAMEBUFFER: {
			struct multiboot_tag_framebuffer *fb = (void *)mbt;

			printf ("%dx%dx%d @ 0x%"PRIx64"\n",
			    fb->common.framebuffer_width,
			    fb->common.framebuffer_height,
			    fb->common.framebuffer_bpp,
			    (uint64_t)fb->common.framebuffer_addr);
#ifdef DEBUG
			mbi_hexdump((char *)mbt, mbt->size);
#endif
			break;
		}
		case MULTIBOOT_TAG_TYPE_ELF_SECTIONS:
			printf("num = %d, entsize = %d, shndx = %d\n",
			    ((struct multiboot_tag_elf_sections *)mbt)->num,
			    ((struct multiboot_tag_elf_sections *)mbt)->entsize,
			    ((struct multiboot_tag_elf_sections *)mbt)->shndx);
#ifdef DEBUG
			mbi_hexdump((char *)mbt, mbt->size);
#endif
			break;
		case MULTIBOOT_TAG_TYPE_APM:
			printf("version = %d, cseg = 0x%x, offset = 0x%x, "
			    "cseg_16 = 0x%x, dseg = 0x%x, flags = 0x%x, "
			    "cseg_len = %d, cseg_16_len = %d, "
			    "dseg_len = %d\n",
			    ((struct multiboot_tag_apm *)mbt)->version,
			    ((struct multiboot_tag_apm *)mbt)->cseg,
			    ((struct multiboot_tag_apm *)mbt)->offset,
			    ((struct multiboot_tag_apm *)mbt)->cseg_16,
			    ((struct multiboot_tag_apm *)mbt)->dseg,
			    ((struct multiboot_tag_apm *)mbt)->flags,
			    ((struct multiboot_tag_apm *)mbt)->cseg_len,
			    ((struct multiboot_tag_apm *)mbt)->cseg_16_len,
			    ((struct multiboot_tag_apm *)mbt)->dseg_len);
			break;
		case MULTIBOOT_TAG_TYPE_EFI32:
			printf("0x%x\n",
			    ((struct multiboot_tag_efi32 *)mbt)->pointer);
			break;
		case MULTIBOOT_TAG_TYPE_EFI64:
			printf("0x%"PRIx64"\n", (uint64_t)
			    ((struct multiboot_tag_efi64 *)mbt)->pointer);
			break;
		case MULTIBOOT_TAG_TYPE_SMBIOS:
			printf("major = %d, minor = %d\n",
			    ((struct multiboot_tag_smbios *)mbt)->major,
			    ((struct multiboot_tag_smbios *)mbt)->minor);
#ifdef DEBUG
			mbi_hexdump((char *)mbt, mbt->size);
#endif
			break;
		case MULTIBOOT_TAG_TYPE_ACPI_OLD:
			printf("\n");
#ifdef DEBUG
			mbi_hexdump((char *)mbt, mbt->size);
#endif
			break;
		case MULTIBOOT_TAG_TYPE_ACPI_NEW:
			printf("\n");
#ifdef DEBUG
			mbi_hexdump((char *)mbt, mbt->size);
#endif
			break;
		case MULTIBOOT_TAG_TYPE_NETWORK:
			printf("\n");
#ifdef DEBUG
			mbi_hexdump((char *)mbt, mbt->size);
#endif
			break;
		case MULTIBOOT_TAG_TYPE_EFI_MMAP:
			printf("\n");
#ifdef DEBUG
			mbi_hexdump((char *)mbt, mbt->size);
#endif
			break;
		case MULTIBOOT_TAG_TYPE_EFI_BS:
			printf("\n");
			break;
		case MULTIBOOT_TAG_TYPE_EFI32_IH:
			printf("0x%"PRIx32"\n",
			    ((struct multiboot_tag_efi32_ih *)mbt)->pointer);
			break;
		case MULTIBOOT_TAG_TYPE_EFI64_IH:
			printf("0x%"PRIx64"\n", (uint64_t)
			    ((struct multiboot_tag_efi64_ih *)mbt)->pointer);
			break;
		case MULTIBOOT_TAG_TYPE_LOAD_BASE_ADDR: {
			struct multiboot_tag_load_base_addr *ld = (void *)mbt;
			printf("0x%x\n", ld->load_base_addr);
			break;
		}
		case MULTIBOOT_TAG_TYPE_END:
			printf("\n");
			break;
		default:
			printf("\n");
#ifdef DEBUG
			mbi_hexdump((char *)mbt, mbt->size);
#endif
			break;
		}
	}

out:
	return;
}




/*
 * Sets up the initial kernel symbol table.  Returns true if this was
 * passed in by Multiboot; false otherwise.
 */
bool
multiboot2_ksyms_addsyms_elf(void)
{
	struct multiboot_symbols *ms = &Multiboot_Symbols;
	vaddr_t symstart = (vaddr_t)ms->s_symstart;
	vaddr_t strstart = (vaddr_t)ms->s_strstart;
	Elf_Ehdr ehdr;

	if (!multiboot2_enabled || !has_syms)
		return false;

	KASSERT(esym != 0);

#ifdef __LP64__
	/* Adjust pointer as 64 bits */
	symstart &= 0xffffffff;
	symstart |= ((vaddr_t)KERNBASE_HI << 32);
	strstart &= 0xffffffff;
	strstart |= ((vaddr_t)KERNBASE_HI << 32);
#endif

	memset(&ehdr, 0, sizeof(ehdr));
	memcpy(ehdr.e_ident, ELFMAG, SELFMAG);
	ehdr.e_ident[EI_CLASS] = ELFCLASS;
	ehdr.e_ident[EI_DATA] = ELFDATA2LSB;
	ehdr.e_ident[EI_VERSION] = EV_CURRENT;
	ehdr.e_ident[EI_OSABI] = ELFOSABI_SYSV;
	ehdr.e_ident[EI_ABIVERSION] = 0;
	ehdr.e_type = ET_EXEC;
#ifdef __amd64__
	ehdr.e_machine = EM_X86_64;
#elif __i386__
	ehdr.e_machine = EM_386;
#else
	#error "Unknown ELF machine type"
#endif
	ehdr.e_version = 1;
	ehdr.e_entry = (Elf_Addr)&start;
	ehdr.e_ehsize = sizeof(ehdr);

	ksyms_addsyms_explicit((void *)&ehdr,
	    (void *)symstart, ms->s_symsize,
	    (void *)strstart, ms->s_strsize);

	return true;
}
