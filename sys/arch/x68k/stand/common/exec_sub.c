/*	$NetBSD: exec_sub.c,v 1.1 2002/05/18 13:54:38 isaki Exp $ */

#include <sys/cdefs.h>

#include "execkern.h"
#include <a.out.h>
#include <sys/param.h>

#ifdef BOOT
void B_PRINT __P((const unsigned char *p));
#endif

static __inline void bzero4 __P((void *ptr, size_t siz));
static void xk_aout __P((struct execkern_arg *xarg, struct exec *hdr));
static void xk_elf __P((struct execkern_arg *xarg, Elf32_Ehdr *hdr));

#ifdef LOADBSD
static void DPRINT_SEC __P((const char *ident,
			    const struct execkern_section *sec));
extern int opt_v;
extern const char *kernel_fn;

static void
DPRINT_SEC(ident, sec)
	const char *ident;
	const struct execkern_section *sec;
{

	if (opt_v)
		xwarnx("section (%s): img %p, sz %d, pad %d", ident,
			sec->sec_image, sec->sec_size, sec->sec_pad);
}

#define ERRX(arg)		xerrx arg

#else
#define DPRINT_SEC(ident, sec)	/**/
#define ERRX(arg)		return 1
#endif

/*
 * This code is size-hacked version of
 *
 *	sec->sec_image = (image);
 *	sec->sec_size = (size);
 *	sec->sec_pad = (pad);
 *	DPRINT_SEC((ident), sec);
 *	sec++;
 */
#define SECTION(sec, ident, image, size, pad)	\
	do {					\
		u_long *wp = (void *) sec;	\
		*(void **)wp++ = (image);	\
		*wp++ = (size);			\
		*wp++ = (pad);			\
		DPRINT_SEC((ident), sec);	\
		sec = (void *) wp;		\
	} while (0)

#define SECTION_NOPAD(sec, ident, image, size)	\
		SECTION(sec, (ident), (image), (size), 0)

static __inline void
bzero4(ptr, siz)
	void *ptr;
	size_t siz;
{
	u_long *p;
	u_short s;

	p = ptr;
	s = siz >> 2;

	while (s--)
		*p++ = 0;
}

/*
 * fill in loading information from an a.out executable
 */
static void
xk_aout(xarg, hdr)
	struct execkern_arg *xarg;
	struct exec *hdr;
{
	unsigned u;
	char *s;
	struct execkern_section *sec;

	xarg->entry_addr = hdr->a_entry;
	sec = xarg->sec;

	/* text section and padding between data section */
	s = (void *) (hdr + 1);
	SECTION(sec, "text", s, hdr->a_text, -hdr->a_text & (__LDPGSZ-1));

	/* data and bss sections */
	s += hdr->a_text;
	SECTION(sec, "data/bss", s, hdr->a_data, hdr->a_bss);

	/* size of symbol table */
	SECTION_NOPAD(sec, "symtab size", &sec[1].sec_size, sizeof(u_long));

	/* symbol table section */
	s += hdr->a_data;
	SECTION_NOPAD(sec, "symbol", s, u = hdr->a_syms);

	/* string table section */
	if (u) {
#ifdef LOADBSD
		if (opt_v)
			xwarnx("symbol table found");
#endif
		s += u;
		SECTION_NOPAD(sec, "string", s, *(u_long *) s);
	}
}

/*
 * fill in loading information from an ELF executable
 */
static void
xk_elf(xarg, hdr)
	struct execkern_arg *xarg;
	Elf32_Ehdr *hdr;
{
	char *top = (void *) hdr;
	struct execkern_section *sec;
	Elf32_Phdr *ph;
	Elf32_Shdr *sh, *sym, *str, *stab, *shstr;
	const char *shstrtab, *shname;
	unsigned u, dpos, pd;
	const char *const shstrtab_new = SHSTRTAB_FAKE;

	xarg->entry_addr = hdr->e_entry;

	/*
	 * text, data, bss
	 */
	ph = (void *) (top + hdr->e_phoff);
	xarg->load_addr = ph->p_vaddr;

	sec = xarg->sec;
	sec->sec_image = top + ph->p_offset;
	sec->sec_size = ph->p_filesz;

	if (hdr->e_phnum != 1) {
		sec->sec_pad = ph[1].p_vaddr - (ph->p_vaddr + ph->p_filesz);
		DPRINT_SEC("program (text)", sec);
		sec++;
		ph++;
		sec->sec_image = top + ph->p_offset;
		sec->sec_size = ph->p_filesz;
	}

	sec->sec_pad = ph->p_memsz - ph->p_filesz;
	DPRINT_SEC("program (data/bss)", sec);
	sec++;

	/*
	 * symbol size
	 */
	xarg->elfsymsiz = 0;		/* no symbol */
	SECTION_NOPAD(sec, "symtab size", &xarg->elfsymsiz, sizeof(int));

	/*
	 * ELF header
	 */
	xarg->ehdr = *hdr;
	xarg->ehdr.e_shstrndx = 0;	/* .shstrtab will be the 1st section */
	SECTION_NOPAD(sec, "ELF header", &xarg->ehdr, sizeof(Elf32_Ehdr));

	sh = (void *) (top + hdr->e_shoff);		/* section header */
	shstr = sh + hdr->e_shstrndx;			/* .shstrtab */
	shstrtab = top + shstr->sh_offset;

	sym = str = stab = 0;
	for (u = 0; sh++, ++u < hdr->e_shnum; ) {
		shname = shstrtab + sh->sh_name;
		if (!strcmp(shname, shstrtab_new + SHNAME_OFF_SYMTAB))
			sym = sh;				/* .symtab */
		if (!strcmp(shname, shstrtab_new + SHNAME_OFF_STRTAB))
			str = sh;				/* .strtab */
		if (!strcmp(shname, shstrtab_new + SHNAME_OFF_STAB))
			stab = sh;				/* .stab */
	}

	if (shstr == 0 || sym == 0 || str == 0)
		xarg->ehdr.e_shnum = 0;		/* no symbol */
	else {
#ifdef LOADBSD
		if (opt_v) {
			xwarnx("symbol table found");
			if (stab)
				xwarnx("debugging information found");
		}
#endif
		xarg->elfsymsiz = 1;		/* has symbol */
		xarg->ehdr.e_shnum = 3;
		xarg->ehdr.e_shoff = sizeof(Elf32_Ehdr);

		SECTION_NOPAD(sec, "section header (shstrtab)",
				shstr, sizeof(Elf32_Shdr));

		SECTION_NOPAD(sec, "section header (symbol)",
				sym, sizeof(Elf32_Shdr));

		SECTION_NOPAD(sec, "section header (string)",
				str, sizeof(Elf32_Shdr));

		dpos = sizeof(Elf32_Ehdr) + sizeof(Elf32_Shdr) * 3;
		u = SIZE_SHSTRTAB_FAKE;

		if (stab) {
			xarg->ehdr.e_shnum++;
			SECTION_NOPAD(sec, "section header (stab)",
					stab, sizeof(Elf32_Shdr));
			dpos += sizeof(Elf32_Shdr);
			u = SIZE_SHSTRTAB_FAKE_WITH_STAB;
		}

		/* new .shstrtab section */
		memcpy(xarg->shstrtab_fake, shstrtab_new, u);
		/*
		 * DDB requires symtab be aligned.
		 */
		pd = -u & ALIGNBYTES;
		SECTION(sec, "shstrtab", &xarg->shstrtab_fake, u, pd);
		shstr->sh_name = SHNAME_OFF_SHSTRTAB;
		shstr->sh_offset = dpos;
		dpos += u + pd;

		SECTION_NOPAD(sec, "symtab",
				top + sym->sh_offset, sym->sh_size);
		sym->sh_name = SHNAME_OFF_SYMTAB;
		sym->sh_offset = dpos;
		dpos += sym->sh_size;

		SECTION_NOPAD(sec, "strtab",
				top + str->sh_offset, str->sh_size);
		str->sh_name = SHNAME_OFF_STRTAB;
		str->sh_offset = dpos;
		dpos += str->sh_size;

		if (stab) {
			SECTION_NOPAD(sec, "stab",
					top + stab->sh_offset, stab->sh_size);
			stab->sh_name = SHNAME_OFF_STAB;
			stab->sh_offset = dpos;
		}
	}
}


int
xk_load(xarg, buf, loadaddr)
	struct execkern_arg *xarg;
	void *buf;
	u_long loadaddr;	/* for a.out */
{
	struct exec *ahdr;
	Elf32_Ehdr *ehdr;
	unsigned u;

	/* Unused section entries should be cleared to zero. */
	bzero4(xarg->sec, sizeof xarg->sec);

	xarg->load_addr = loadaddr;

	/*
	 * check exec header
	 */
	ahdr = buf;
	ehdr = buf;

	if (N_GETMAGIC(*ahdr) == NMAGIC) {
		/*
		 * this is an a.out
		 */
#ifdef LOADBSD
		if (opt_v)
			xwarnx("%s: is an a.out", kernel_fn);
#endif
#ifdef BOOT
B_PRINT("This is an a.out\r\n");
#endif

		if ((u = N_GETMID(*ahdr)) != MID_M68K)
			ERRX((1, "%s: Wrong architecture (mid %u)",
					kernel_fn, u));

		/*
		 * fill in loading information
		 */
		xk_aout(xarg, ahdr);

	} else {

		/*
		 * check ELF header
		 */
		if (*(u_int32_t *)&ehdr->e_ident[EI_MAG0] !=
			(ELFMAG0<<24 | ELFMAG1<<16 | ELFMAG2<<8 | ELFMAG3) ||
		    *(u_int16_t *)&ehdr->e_ident[EI_CLASS] !=
			(ELFCLASS32 << 8 | ELFDATA2MSB))
			ERRX((1, "%s: Not an NMAGIC a.out or a 32bit BE ELF",
					kernel_fn));

		/*
		 * this is an ELF
		 */
#ifdef LOADBSD
		if (opt_v)
			xwarnx("%s: is an ELF", kernel_fn);
#endif
#ifdef BOOT
B_PRINT("This is an ELF\r\n");
#endif

		if (ehdr->e_ident[EI_VERSION] != EV_CURRENT ||
		    ehdr->e_version != EV_CURRENT)
			ERRX((1, "%s: Unsupported ELF version", kernel_fn));

		if ((u = ehdr->e_machine) != EM_68K)
			ERRX((1, "%s: Wrong architecture (mid %u)",
					kernel_fn, u));
		if (ehdr->e_type != ET_EXEC)
			ERRX((1, "%s: Not an executable", kernel_fn));
		if ((u = ehdr->e_phnum) != 1 && u != 2)
			ERRX((1, "%s: Wrong number (%u) of loading sections",
					kernel_fn, u));

		/*
		 * fill in loading information
		 */
		xk_elf(xarg, ehdr);
	}

	return 0;
}
