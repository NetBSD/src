/*	$NetBSD: elf_machdep.h,v 1.1.4.1 1999/11/15 00:39:49 fvdl Exp $	*/

#define	ELF32_MACHDEP_ENDIANNESS	ELFDATA2LSB
#define	ELF32_MACHDEP_ID_CASES						\
		case EM_VAX:						\
			break;

#define	ELF64_MACHDEP_ENDIANNESS	XXX	/* break compilation */
#define	ELF64_MACHDEP_ID_CASES						\
		/* no 64-bit ELF machine types supported */

/* VAX relocations */
#define	R_VAX_NONE	0
#define	R_VAX_8		1	/* S + A */
#define	R_VAX_16	2
#define	R_VAX_32	3
#define R_VAX_RELATIVE	4
#define	R_VAX_PC8	5	/* S + A - P */
#define	R_VAX_PC16	6
#define	R_VAX_PC32	7
#define	R_VAX_COPY	8
#define	R_VAX_REL32	9	/* S + A + D */
#define	R_VAX_CALL_SLOT	10
#define	R_VAX_JMP_SLOT	11
#define	R_VAX_GLOB_DAT	12

#define	R_TYPE(name)	__CONCAT(R_VAX_,name)
