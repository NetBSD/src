/*	$NetBSD: exec_elf.h,v 1.10 1997/03/21 05:34:40 cgd Exp $	*/

/*
 * Copyright (c) 1994 Christos Zoulas
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
 * 3. The name of the author may not be used to endorse or promote products
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

#ifndef _SYS_EXEC_ELF_H_
#define	_SYS_EXEC_ELF_H_

#include <machine/types.h>

typedef u_int8_t  Elf_Byte;

typedef u_int32_t Elf32_Addr;
typedef u_int32_t Elf32_Off;
typedef int32_t   Elf32_Sword;
typedef u_int32_t Elf32_Word;
typedef u_int16_t Elf32_Half;

typedef u_int64_t Elf64_Addr;
typedef u_int64_t Elf64_Off;
typedef int64_t   Elf64_Sword;
typedef int32_t   Elf64_Shalf;
typedef u_int64_t Elf64_Word;
typedef u_int32_t Elf64_Half;
typedef u_int16_t Elf64_Quarter;


#define	ELF_IDSIZE	16

enum Elf_e_type {
	Elf_et_none = 0,
	Elf_et_rel,
	Elf_et_exec,
	Elf_et_dyn,
	Elf_et_core,
	Elf_et_num
};

enum Elf_e_machine {
	Elf_em_none = 0,
	Elf_em_m32,
	Elf_em_sparc,
	Elf_em_386,
	Elf_em_68k,
	Elf_em_88k,
	Elf_em_486,
	Elf_em_860,
	Elf_em_mips,
	Elf_em_ppc = 20,
	Elf_em_alpha=0x9026,
	Elf_em_num
};

enum Elf_e_class {
	Elf_ec_invalid = 0,
	Elf_ec_32bit,
	Elf_ec_64bit
};

enum Elf_e_ident {
	Elf_ei_mag0 = 0,
	Elf_ei_mag1,
	Elf_ei_mag2,
	Elf_ei_mag3,
	Elf_ei_class,
	Elf_ei_data,
	Elf_ei_version,
	Elf_ei_pad,
	Elf_ei_size = 16,
};

enum Elf_e_version {
	Elf_ev_none,
	Elf_ev_current
};

enum Elf_e_datafmt {
	Elf_ed_none,
	Elf_ed_2lsb,
	Elf_ed_2msb
};

enum Elf_p_pf {
	Elf_pf_r = 4,
	Elf_pf_w = 2,
	Elf_pf_x = 1
};

typedef struct {
	unsigned char	e_ident[ELF_IDSIZE];	/* Id bytes */
	Elf32_Half	e_type;			/* file type */
	Elf32_Half	e_machine;		/* machine type */
	Elf32_Word	e_version;		/* version number */
	Elf32_Addr	e_entry;		/* entry point */
	Elf32_Off	e_phoff;		/* Program hdr offset */
	Elf32_Off	e_shoff;		/* Section hdr offset */
	Elf32_Word	e_flags;		/* Processor flags */
	Elf32_Half      e_ehsize;		/* sizeof ehdr */
	Elf32_Half      e_phentsize;		/* Program header entry size */
	Elf32_Half      e_phnum;		/* Number of program headers */
	Elf32_Half      e_shentsize;		/* Section header entry size */
	Elf32_Half      e_shnum;		/* Number of section headers */
	Elf32_Half      e_shstrndx;		/* String table index */
} Elf32_Ehdr;

#define	Elf32_e_ident "\177ELF\001"
#define	Elf32_e_siz (sizeof(Elf32_e_ident) - 1)
#define	ELF32_HDR_SIZE	(sizeof(Elf32_Ehdr))

typedef struct {
	unsigned char	e_ident[ELF_IDSIZE];	/* Id bytes */
	Elf64_Quarter	e_type;			/* file type */
	Elf64_Quarter	e_machine;		/* machine type */
	Elf64_Half	e_version;		/* version number */
	Elf64_Addr	e_entry;		/* entry point */
	Elf64_Off	e_phoff;		/* Program hdr offset */
	Elf64_Off	e_shoff;		/* Section hdr offset */
	Elf64_Half	e_flags;		/* Processor flags */
	Elf64_Quarter	e_ehsize;		/* sizeof ehdr */
	Elf64_Quarter	e_phentsize;		/* Program header entry size */
	Elf64_Quarter	e_phnum;		/* Number of program headers */
	Elf64_Quarter	e_shentsize;		/* Section header entry size */
	Elf64_Quarter	e_shnum;		/* Number of section headers */
	Elf64_Quarter	e_shstrndx;		/* String table index */
} Elf64_Ehdr;

#define	Elf64_e_ident "\177ELF\002"
#define	Elf64_e_siz (sizeof(Elf64_e_ident) - 1)
#define	ELF64_HDR_SIZE	(sizeof(Elf64_Ehdr))

enum Elf_p_pt {
	Elf_pt_null = 0,		/* Program header table entry unused */
	Elf_pt_load = 1,		/* Loadable program segment */
	Elf_pt_dynamic = 2,		/* Dynamic linking information */
	Elf_pt_interp = 3,		/* Program interpreter */
	Elf_pt_note = 4,		/* Auxiliary information */
	Elf_pt_shlib = 5,		/* Reserved, unspecified semantics */
	Elf_pt_phdr = 6,		/* Entry for header table itself */
	Elf_pt_loproc = 0x70000000,	/* Processor-specific */
	Elf_pt_hiproc = 0x7FFFFFFF	/* Processor-specific */
#define	Elf_pt_mips_reginfo	0x70000000
};

typedef struct {
	Elf32_Word	p_type;		/* entry type */
	Elf32_Off	p_offset;	/* offset */
	Elf32_Addr	p_vaddr;	/* virtual address */
	Elf32_Addr	p_paddr;	/* physical address */
	Elf32_Word	p_filesz;	/* file size */
	Elf32_Word	p_memsz;	/* memory size */
	Elf32_Word	p_flags;	/* flags */
	Elf32_Word	p_align;	/* memory & file alignment */
} Elf32_Phdr;

typedef struct {
	Elf64_Half	p_type;		/* entry type */
	Elf64_Half	p_flags;	/* flags */
	Elf64_Off	p_offset;	/* offset */
	Elf64_Addr	p_vaddr;	/* virtual address */
	Elf64_Addr	p_paddr;	/* physical address */
	Elf64_Word	p_filesz;	/* file size */
	Elf64_Word	p_memsz;	/* memory size */
	Elf64_Word	p_align;	/* memory & file alignment */
} Elf64_Phdr;

/*
 * Section Headers
 */
enum Elf_s_sht {
	Elf_sht_null = 0,
	Elf_sht_progbits = 1,
	Elf_sht_symtab = 2,
	Elf_sht_strtab = 3,
	Elf_sht_rela = 4,
	Elf_sht_hash = 5,
	Elf_sht_dynamic = 6,
	Elf_sht_note = 7,
	Elf_sht_nobits = 8,
	Elf_sht_rel =	9,
	Elf_sht_shlib = 10,
	Elf_sht_dynsym = 11,
	Elf_sht_loproc = 0x70000000,
	Elf_sht_hiproc = 0x7FFFFFFF
};

typedef struct {
	Elf32_Word	sh_name;	/* section name */
	Elf32_Word	sh_type;	/* section type */
	Elf32_Word	sh_flags;	/* section flags */
	Elf32_Addr	sh_addr;	/* virtual address */
	Elf32_Off	sh_offset;	/* file offset */
	Elf32_Word	sh_size;	/* section size */
	Elf32_Word	sh_link;	/* link to another */
	Elf32_Word	sh_info;	/* misc info */
	Elf32_Word	sh_addralign;	/* memory alignment */
	Elf32_Word	sh_entsize;	/* table entry size */
} Elf32_Shdr;

typedef struct {
	Elf64_Half	sh_name;	/* section name */
	Elf64_Half	sh_type;	/* section type */
	Elf64_Word	sh_flags;	/* section flags */
	Elf64_Addr	sh_addr;	/* virtual address */
	Elf64_Off	sh_offset;	/* file offset */
	Elf64_Word	sh_size;	/* section size */
	Elf64_Half	sh_link;	/* link to another */
	Elf64_Half	sh_info;	/* misc info */
	Elf64_Word	sh_addralign;	/* memory alignment */
	Elf64_Word	sh_entsize;	/* table entry size */
} Elf64_Shdr;

/*
 * Symbol Definitions
 */
typedef struct {
	Elf32_Word	st_name;	/* Symbol name index in str table */
	Elf32_Word	st_value;	/* value of symbol */
	Elf32_Word	st_size;	/* size of symbol */
	Elf_Byte	st_info;	/* type / binding attrs */
	Elf_Byte	st_other;	/* unused */
	Elf32_Half	st_shndx;	/* section index of symbol */
} Elf32_Sym;

typedef struct {
	Elf64_Half	st_name;	/* Symbol name index in str table */
	Elf_Byte	st_info;	/* type / binding attrs */
	Elf_Byte	st_other;	/* unused */
	Elf64_Quarter	st_shndx;	/* section index of symbol */
	Elf64_Word	st_value;	/* value of symbol */
	Elf64_Word	st_size;	/* size of symbol */
} Elf64_Sym;

#define	ELF_SYM_UNDEFINED	0

enum elf_e_symbol_binding {
	Elf_estb_local,
	Elf_estb_global,
	Elf_estb_weak
};

#define	ELF_SYM_BIND(info)	((enum elf_e_symbol_binding) (((u_int32_t)(info)) >> 4))

enum elf_e_symbol_type {
	Elf_estt_notype,
	Elf_estt_object,
	Elf_estt_func,
	Elf_estt_section,
	Elf_estt_file
};

#define	ELF_SYM_TYPE(info)	((enum elf_e_symbol_type) ((info) & 0x0F))

enum elf_e_symbol_section_index {
	Elf_eshn_undefined	=0,
	Elf_eshn_mips_acommon	=0xFF00,
	Elf_eshn_mips_scommon	=0xFF03,
	Elf_eshn_absolute	=0xFFF1,
	Elf_eshn_common		=0xFFF2
};

/*
 * Relocation Entries
 */
typedef struct {
	Elf32_Word	r_offset;	/* where to do it */
	Elf32_Word	r_info;		/* index & type of relocation */
} Elf32_Rel;

typedef struct {
	Elf32_Word	r_offset;	/* where to do it */
	Elf32_Word	r_info;		/* index & type of relocation */
	Elf32_Word	r_addend;	/* adjustment value */
} Elf32_RelA;

typedef struct {
	Elf64_Word	r_offset;	/* where to do it */
	Elf64_Word	r_info;		/* index & type of relocation */
} Elf64_Rel;

#define	ELF32_R_SYM(info)	((info) >> 8)
#define	ELF32_R_TYPE(info)	((info) & 0xFF)

typedef struct {
	Elf64_Word	r_offset;	/* where to do it */
	Elf64_Word	r_info;		/* index & type of relocation */
	Elf64_Word	r_addend;	/* adjustment value */
} Elf64_RelA;

#define	ELF64_R_SYM(info)	((info) >> 32)
#define	ELF64_R_TYPE(info)	((info) & 0xFFFFFFFF)

typedef struct {
	Elf32_Word	d_tag;		/* entry tag value */
	union {
	    Elf32_Addr	d_ptr;
	    Elf32_Word	d_val;
	} d_un;
} Elf32_Dyn;

typedef struct {
	Elf64_Word	d_tag;		/* entry tag value */
	union {
	    Elf64_Addr	d_ptr;
	    Elf64_Word	d_val;
	} d_un;
} Elf64_Dyn;

enum Elf_e_dynamic_type {
    Elf_edt_null,
    Elf_edt_needed,
    Elf_edt_pltrelsz,
    Elf_edt_pltgot,
    Elf_edt_hash,
    Elf_edt_strtab,
    Elf_edt_symtab,
    Elf_edt_rela,
    Elf_edt_relasz,
    Elf_edt_relaent,
    Elf_edt_strsz,
    Elf_edt_syment,
    Elf_edt_init,
    Elf_edt_fini,
    Elf_edt_soname,
    Elf_edt_rpath,
    Elf_edt_symbolic,
    Elf_edt_rel,
    Elf_edt_relsz,
    Elf_edt_relent,
    Elf_edt_pltrel,
    Elf_edt_debug,
    Elf_edt_textrel,
    Elf_edt_jmprel
};

typedef struct {
	Elf32_Sword	au_id;				/* 32-bit id */
	Elf32_Word	au_v;				/* 32-bit value */
} Aux32Info;

typedef struct {
	Elf64_Shalf	au_id;				/* 32-bit id */
	Elf64_Word	au_v;				/* 64-bit id */
} Aux64Info;

enum AuxID {
	AUX_null = 0,
	AUX_ignore = 1,
	AUX_execfd = 2,
	AUX_phdr = 3,		/* &phdr[0] */
	AUX_phent = 4,		/* sizeof(phdr[0]) */
	AUX_phnum = 5,		/* # phdr entries */
	AUX_pagesz = 6,		/* PAGESIZE */
	AUX_base = 7,		/* ld.so base addr */
	AUX_flags = 8,		/* processor flags */
	AUX_entry = 9,		/* a.out entry */
	AUX_debug = 10,		/* debug flag */
	AUX_count = 11,		/* not really the limit */
	AUX_sun_uid = 2000,	/* euid */
	AUX_sun_ruid = 2001,	/* ruid */
	AUX_sun_gid = 2002,	/* egid */
	AUX_sun_rgid = 2003	/* rgid */
};

/*
 * Note Definitions
 */
typedef struct {
	Elf32_Word namesz;
	Elf32_Word descsz;
	Elf32_Word type;
} Elf32_Note;

typedef struct {
	Elf64_Half namesz;
	Elf64_Half descsz;
	Elf64_Half type;
} Elf64_Note;

/* "name" for NetBSD-specific notes. */
#define	ELF_NOTE_NETBSD_NAME		"NetBSD"

/* NetBSD-specific note type: OS Version.  desc is 4-byte NetBSD integer. */
#define	ELF_NOTE_NETBSD_TYPE_OSVERSION	1

/* NetBSD-specific note type: Emulation name.  desc is emul name string. */
#define	ELF_NOTE_NETBSD_TYPE_EMULNAME	2


#include <machine/elf_machdep.h>

#if defined(ELFSIZE) && (ELFSIZE == 32)
#define	Elf_Ehdr	Elf32_Ehdr
#define	Elf_Phdr	Elf32_Phdr
#define	Elf_Shdr	Elf32_Shdr
#define	Elf_Sym		Elf32_Sym
#define	Elf_Rel		Elf32_Rel
#define	Elf_RelA	Elf32_RelA
#define	Elf_Dyn		Elf32_Dyn
#define	Elf_Word	Elf32_Word
#define	Elf_Addr	Elf32_Addr
#define	Elf_Off		Elf32_Off
#define	Elf_Note	Elf32_Note

#define	ELF_R_SYM	ELF32_R_SYM
#define	ELF_R_TYPE	ELF32_R_TYPE
#define	Elf_e_ident	Elf32_e_ident
#define	Elf_e_siz	Elf32_e_siz

#define	AuxInfo		Aux32Info
#elif defined(ELFSIZE) && (ELFSIZE == 64)
#define	Elf_Ehdr	Elf64_Ehdr
#define	Elf_Phdr	Elf64_Phdr
#define	Elf_Shdr	Elf64_Shdr
#define	Elf_Sym		Elf64_Sym
#define	Elf_Rel		Elf64_Rel
#define	Elf_RelA	Elf64_RelA
#define	Elf_Dyn		Elf64_Dyn
#define	Elf_Word	Elf64_Word
#define	Elf_Addr	Elf64_Addr
#define	Elf_Off		Elf64_Off
#define	Elf_Note	Elf64_Note

#define	ELF_R_SYM	ELF64_R_SYM
#define	ELF_R_TYPE	ELF64_R_TYPE
#define	Elf_e_ident	Elf64_e_ident
#define	Elf_e_siz	Elf64_e_siz

#define	AuxInfo		Aux64Info
#endif

#ifdef _KERNEL

#define ELF32_NO_ADDR	(~(Elf32_Addr)0) /* Indicates addr. not yet filled in */
#define ELF64_NO_ADDR	(~(Elf64_Addr)0) /* Indicates addr. not yet filled in */
#define ELF_AUX_ENTRIES	8		/* Size of aux array passed to loader */

struct elf_args {
        u_long  arg_entry;      /* program entry point */
        u_long  arg_interp;     /* Interpreter load address */
        u_long  arg_phaddr;     /* program header address */
        u_long  arg_phentsize;  /* Size of program header */
        u_long  arg_phnum;      /* Number of program headers */
};

#ifdef EXEC_ELF32
int	exec_elf32_makecmds __P((struct proc *, struct exec_package *));
int	elf32_read_from __P((struct proc *, struct vnode *, u_long,
	    caddr_t, int));
void	*elf32_copyargs __P((struct exec_package *, struct ps_strings *,
	    void *, void *));
#endif

#ifdef EXEC_ELF64
int	exec_elf64_makecmds __P((struct proc *, struct exec_package *));
int	elf64_read_from __P((struct proc *, struct vnode *, u_long,
	    caddr_t, int));
void	*elf64_copyargs __P((struct exec_package *, struct ps_strings *,
	    void *, void *));
#endif

/* common */
int	exec_elf_setup_stack __P((struct proc *, struct exec_package *));

#endif /* _KERNEL */

#endif /* !_SYS_EXEC_ELF_H_ */
