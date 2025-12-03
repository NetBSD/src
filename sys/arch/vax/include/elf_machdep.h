/*	$NetBSD: elf_machdep.h,v 1.7 2025/12/03 21:36:14 jkoshy Exp $	*/

#if !defined(_SYS_ELFDEFINITIONS_H_)
/* VAX relocations */
#define	R_VAX_NONE	0
#define	R_VAX_32	1	/* S + A */
#define	R_VAX_16	2
#define	R_VAX_8		3
#define	R_VAX_PC32	4	/* S + A - PC */
#define	R_VAX_PC16	5
#define	R_VAX_PC8	6
#define	R_VAX_COPY	19
#define	R_VAX_GLOB_DAT	20
#define	R_VAX_JMP_SLOT	21
#define R_VAX_RELATIVE	22	/* S + A + D */
#endif /* !defined(_SYS_ELFDEFINITIONS_H_) */

/*
 * Local symbols.
 */
#define	ELF32_MACHDEP_ENDIANNESS	ELFDATA2LSB
#define	ELF32_MACHDEP_ID_CASES						\
		case EM_VAX:						\
			break;

#define	ELF64_MACHDEP_ENDIANNESS	XXX	/* break compilation */
#define	ELF64_MACHDEP_ID_CASES						\
		/* no 64-bit ELF machine types supported */

#define	ELF32_MACHDEP_ID	EM_VAX

#define	KERN_ELFSIZE		32
#define ARCH_ELFSIZE		32	/* MD native binary size */

#define	R_TYPE(name)	__CONCAT(R_VAX_,name)
