/*	$NetBSD: elf_machdep.h,v 1.1 2001/01/11 22:28:07 bjh21 Exp $	*/

#define ELF32_MACHDEP_ENDIANNESS	ELFDATA2LSB

/* Processor specific flags for the ELF header e_flags field.  */
#define EF_ARM_RELEXEC		0x00000001
#define EF_ARM_HASENTRY		0x00000002
#define EF_ARM_INTERWORK	0x00000004 /* GNU binutils 000413 */
#define EF_ARM_SYMSARESORTED	0x00000004 /* ARM ELF A08 */
#define EF_ARM_APCS_26		0x00000008
#define EF_ARM_APCS_FLOAT	0x00000010
#define EF_ARM_PIC		0x00000020
#define EF_ARM_ALIGN8		0x00000040 /* 8-bit structure alignment.  */
#define EF_ARM_NEW_ABI		0x00000080
#define EF_ARM_OLD_ABI		0x00000100
#define EF_ARM_SOFT_FLOAT	0x00000200
#define EF_ARM_EABIMASK		0xff000000

#define	ELF32_MACHDEP_ID_CASES						\
		case EM_ARM:						\
			break;

#define ARCH_ELFSIZE		32	/* MD native binary size */

#define R_ARM_NONE		0
#define R_ARM_PC24		1
#define R_ARM_ABS32		2
#define R_ARM_REL32		3
#define R_ARM_PC13		4
#define R_ARM_ABS16		5
#define R_ARM_ABS12		6
#define R_ARM_THM_ABS5		7
#define R_ARM_ABS8		8
#define R_ARM_SBREL32		9
#define R_ARM_THM_PC22		10
#define R_ARM_THM_PC8		11
#define R_ARM_AMP_VCALL9	12
#define R_ARM_SWI24		13
#define R_ARM_THM_SWI8		14
#define R_ARM_XPC25		15
#define R_ARM_THM_XPC22		16

#define R_TYPE(name)		__CONCAT(R_ARM_,name)

