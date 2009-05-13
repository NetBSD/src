/*
 *	definitions for exec_kernel()
 *
 *	written by Yasha (ITOH Yasufumi)
 *	public domain
 *
 *	$NetBSD: execkern.h,v 1.3.24.1 2009/05/13 17:18:43 jym Exp $
 */

#ifndef X68K_BOOT_EXECKERN_H
#define X68K_BOOT_EXECKERN_H

/*
 * Max number of ``sections''.
 * Currently this includes: .text, .data, size sym, Elf32_Ehdr, Elf32_Shdr x 4,
 *	.shstrtab, .symtab, .strtab, .stab
 */
#define XK_NSEC			12

#ifndef __ASSEMBLER__

#include <sys/types.h>
#include <sys/exec_elf.h>

struct execkern_arg {
	/* Don't change this structure (see exec_sub.c). */
	u_long		load_addr;	/* text start address */

	struct execkern_section {
		void	*sec_image;	/* section image source address */
		u_long	sec_size;	/* section size */
		u_long	sec_pad;	/* zero fill size after the image */
	} sec[XK_NSEC];

	unsigned	d5;		/* reserved */
	int		rootdev;
	u_long		boothowto;
	u_long		entry_addr;
	/* end of "Don't change this" */

	int		elfsymsiz;
	Elf32_Ehdr	ehdr;		/* saved ELF header */

#define SHSTRTAB_FAKE	"\0.shstrtab\0.symtab\0.strtab\0.stab"
#define SIZE_SHSTRTAB_FAKE_WITH_STAB	33	/* sizeof SHSTRTAB_FAKE */
#define SIZE_SHSTRTAB_FAKE		27	/*   - sizeof ".stab" */
#define SHNAME_OFF_SHSTRTAB		1
#define SHNAME_OFF_SYMTAB		11
#define SHNAME_OFF_STRTAB		19
#define SHNAME_OFF_STAB			27
	char shstrtab_fake[SIZE_SHSTRTAB_FAKE_WITH_STAB];
};

int xk_load(struct execkern_arg *, void *, u_long);
void __dead exec_kernel(struct execkern_arg *);

#endif /* __ASSEMBLER__ */

#endif /* X68K_BOOT_EXECKERN_H */
