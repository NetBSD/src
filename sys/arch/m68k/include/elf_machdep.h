/*	$NetBSD: elf_machdep.h,v 1.5.12.2 2002/02/28 04:10:34 nathanw Exp $	*/

#define	ELF32_MACHDEP_ENDIANNESS	ELFDATA2MSB
#define	ELF32_MACHDEP_ID_CASES						\
		case EM_68K:						\
			break;

#define	ELF64_MACHDEP_ENDIANNESS	XXX	/* break compilation */
#define	ELF64_MACHDEP_ID_CASES						\
		/* no 64-bit ELF machine types supported */

#define	ELF32_MACHDEP_ID	EM_68K

/*
 * Machine-dependent ELF flags.  These are defined by the GNU tools.
 */
#define	EF_CPU32	0x00810000
#define	EF_M68000	0x01000000

#define ARCH_ELFSIZE		32	/* MD native binary size */

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

#define	R_TYPE(name)	__CONCAT(R_68K_,name)
