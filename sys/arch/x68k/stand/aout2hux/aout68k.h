/*
 *	m68k a.out / ELF file structure definitions
 *
 *	written by Yasha (ITOH Yasufumi)
 *	public domain
 *
 *	$NetBSD: aout68k.h,v 1.3 1999/11/19 03:54:08 itohy Exp $
 */
/*
 * NetBSD/m68k a.out format (OMAGIC, NMAGIC)
 *
 *	----------------------------
 *	|  file header (32 bytes)  |
 *	|--------------------------|
 *	|  text                    |
 *	|--------------------------|
 *	|  data                    |
 *	|--------------------------|
 *	|  text relocation table   |
 *	|--------------------------|
 *	|  data relocation table   |
 *	|--------------------------|
 *	|  symbol table            |
 *	|--------------------------|
 *	|  string table            |
 *	----------------------------
 *
 * OMAGIC: text and data segments are loaded contiguous
 * NMAGIC: data segment is loaded at the next page of the text
 */

struct aout_m68k {
	be_uint32_t	a_magic;	/* encoded magic number */
	be_uint32_t	a_text;		/* size of text section */
	be_uint32_t	a_data;		/* size of data section */
	be_uint32_t	a_bss;		/* size of bss */
	be_uint32_t	a_syms;		/* size of symbol table */
	be_uint32_t	a_entry;	/* entry point address */
	be_uint32_t	a_trsize;	/* size of text relocation */
	be_uint32_t	a_drsize;	/* size of data relocation */
};

#define AOUT_GET_MAGIC(e)	(((e)->a_magic.val[2]<<8) | (e)->a_magic.val[3])
#define	AOUT_OMAGIC	0407
#define	AOUT_NMAGIC	0410
#define	AOUT_ZMAGIC	0413	/* demand paging --- header is in the text */

#define AOUT_GET_FLAGS(e)	((e)->a_magic.val[0] >> 2)
#define AOUT_FLAG_PIC		0x10
#define AOUT_FLAG_DYNAMIC	0x20

/* machine ID */
#define AOUT_GET_MID(e)		((((e)->a_magic.val[0] & 0x03) << 8) |	\
					(e)->a_magic.val[1])
#define	AOUT_MID_M68K	135	/* m68k BSD binary, 8KB page */
#define	AOUT_MID_M68K4K	136	/* m68k BSD binary, 4KB page */

#define AOUT_PAGESIZE(e)	(AOUT_GET_MAGIC(e) == AOUT_OMAGIC ? 1 :	\
				AOUT_GET_MID(e) == AOUT_MID_M68K ? 8192 : 4096)

/*
 * m68k ELF executable format
 *
 *	--------------------------------------
 *	|  ELF header     (52 bytes)         |
 *	|------------------------------------|
 *	|  Program header (32bytes x 1 or 2) |
 *	|------------------------------------|
 *	|  section 1 (text)                  |
 *	|------------------------------------|
 *	|  section 2 (data)                  |
 *	|------------------------------------|
 *	|  ...                               |
 *	|------------------------------------|
 *	|  section header table (optional)   |
 *	--------------------------------------
 */

/*
 * ELF header for m68k
 */
struct elf_m68k_hdr {
#define EI_MAG0		0
#define EI_MAG1		1
#define EI_MAG2		2
#define EI_MAG3		3
#define EI_CLASS	4
#define EI_DATA		5
#define EI_VERSION	6
#define EL_NIDENT	16
	u_int8_t	e_ident[EL_NIDENT];	/* ELF magic */
#define ELFMAG0		0x7f
#define ELFMAG1		0x45	/* 'E' */
#define ELFMAG2		0x4c	/* 'L' */
#define ELFMAG3		0x46	/* 'F' */
#define ELFCLASS32	1	/* 32bit */
#define ELFDATA2MSB	2	/* big endian */
	be_uint16_t	e_type;		/* type of this file */
#define ET_EXEC		2
	be_uint16_t	e_machine;	/* architecture id */
#define EM_68K		4
	be_uint32_t	e_version;
#define EV_CURRENT	1
	be_uint32_t	e_entry;	/* entry address */
	be_uint32_t	e_phoff;	/* program header address */
	be_uint32_t	e_shoff;
	be_uint32_t	e_flags;
	be_uint16_t	e_ehsize;
	be_uint16_t	e_phentsize;	/* program header entry size */
	be_uint16_t	e_phnum;	/* number of program header entries */
	be_uint16_t	e_shentsize;	/* section header entry size */
	be_uint16_t	e_shnum;	/* number of section header entries */
	be_uint16_t	e_shstrndx;
};

#define SIZE_ELF68K_HDR		(sizeof(struct elf_m68k_hdr))

/*
 * Section header for m68k ELF
 */
struct elf_m68k_shdr {
	be_uint32_t	sh_name;
	be_uint32_t	sh_type;
#define SHT_PROGBITS	1
	be_uint32_t	sh_flags;
#define SHF_WRITE	1
#define SHF_ALLOC	2
#define SHF_EXECINSTR	4
	be_uint32_t	sh_addr;
	be_uint32_t	sh_offset;
	be_uint32_t	sh_size;
	be_uint32_t	sh_link;
	be_uint32_t	sh_info;
	be_uint32_t	sh_addralign;
	be_uint32_t	sh_entsize;
};

#define ELF68K_ISDATASEG(sh)	\
	(get_uint32(&(sh)->sh_type) == SHT_PROGBITS &&		\
	 (get_uint32(&(sh)->sh_flags) &				\
		(SHF_WRITE | SHF_ALLOC | SHF_EXECINSTR)) ==	\
			(SHF_WRITE | SHF_ALLOC))	/* no SHF_EXECINSTR */

#define SIZE_ELF68K_SHDR	(sizeof(struct elf_m68k_shdr))

/*
 * Program header for m68k ELF
 */
struct elf_m68k_phdr {
	be_uint32_t	p_type;		/* type of segment */
#define PT_LOAD		1
	be_uint32_t	p_offset;	/* file offset */
	be_uint32_t	p_vaddr;	/* virtual address */
	be_uint32_t	p_paddr;	/* physical address (ignored) */
	be_uint32_t	p_filesz;	/* size on file */
	be_uint32_t	p_memsz;	/* size on memory */
	be_uint32_t	p_flags;
#define PF_R		4
#define PF_W		2
#define PF_X		1
	be_uint32_t	p_align;
};

#define SIZE_ELF68K_PHDR	(sizeof(struct elf_m68k_phdr))
