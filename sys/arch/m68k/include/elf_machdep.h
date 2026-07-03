/*	$NetBSD: elf_machdep.h,v 1.11 2026/07/03 19:38:53 thorpej Exp $	*/

#if !defined(_SYS_ELFDEFINITIONS_H_)
/*
 * Machine-dependent ELF flags.  These are defined by the GNU tools.
 *
 * The upper 24 bits encode the architecture, the lower 8 bits
 * encode the ColdFire variant.  If any of the lower 8 ColdFire
 * bits are used, the upper 24 bits will either be EF_M68K_CFV4E
 * or 0.
 */
#define	EF_M68K_CPU32	0x00810000
#define	EF_M68K_M68000	0x01000000
#define	EF_M68K_CFV4E	0x00008000
#define	EF_M68K_FIDO	0x02000000

#define	EF_M68K_ARCH_MASK	(EF_M68K_CPU32 | EF_M68K_M68000 |	\
				 EF_M68K_CFV4E | EF_M68K_FIDO)

/* XXX add ColdFire bits here eventually */

#define	EF_M68K_CF_ISA_MASK	0x0f
#define	EF_M68K_CF_MASK		0xff

/* m68k relocation types */
#define	R_68K_NONE	0
#define	R_68K_32	1
#define	R_68K_16	2
#define	R_68K_8		3
#define	R_68K_PC32	4
#define	R_68K_PC16	5
#define	R_68K_PC8	6
#define	R_68K_GOT32	7
#define	R_68K_GOT16	8
#define	R_68K_GOT8	9
#define	R_68K_GOT32O	10
#define	R_68K_GOT16O	11
#define	R_68K_GOT8O	12
#define	R_68K_PLT32	13
#define	R_68K_PLT16	14
#define	R_68K_PLT8	15
#define	R_68K_PLT32O	16
#define	R_68K_PLT16O	17
#define	R_68K_PLT8O	18
#define	R_68K_COPY	19
#define	R_68K_GLOB_DAT	20
#define	R_68K_JMP_SLOT	21
#define	R_68K_RELATIVE	22

/* TLS relocations */
#define R_68K_TLS_GD32		25
#define R_68K_TLS_GD16		26
#define R_68K_TLS_GD8		27
#define R_68K_TLS_LDM32		28
#define R_68K_TLS_LDM16		29
#define R_68K_TLS_LDM8		30
#define R_68K_TLS_LDO32		31
#define R_68K_TLS_LDO16		32
#define R_68K_TLS_LDO8		33
#define R_68K_TLS_IE32		34
#define R_68K_TLS_IE16		35
#define R_68K_TLS_IE8		36
#define R_68K_TLS_LE32		37
#define R_68K_TLS_LE16		38
#define R_68K_TLS_LE8		39
#define R_68K_TLS_DTPMOD32	40
#define R_68K_TLS_DTPREL32	41
#define R_68K_TLS_TPREL32	42

#endif /* !defined(_SYS_ELFDEFINITIONS_H_) */

#define	ELF32_MACHDEP_ENDIANNESS	ELFDATA2MSB
#define	ELF32_MACHDEP_ID_CASES						\
		case EM_68K:						\
			break;

#define	ELF64_MACHDEP_ENDIANNESS	XXX	/* break compilation */
#define	ELF64_MACHDEP_ID_CASES						\
		/* no 64-bit ELF machine types supported */

#define	ELF32_MACHDEP_ID	EM_68K

#define	KERN_ELFSIZE		32
#define ARCH_ELFSIZE		32	/* MD native binary size */

#if defined(__mc68010__)
#define	ELF32_EHDR_FLAGS_OK(eh)						\
	(((eh)->e_flags & EF_M68K_M68000) != 0)
#endif

#define	R_TYPE(name)	__CONCAT(R_68K_,name)
