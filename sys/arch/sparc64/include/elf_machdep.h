/*	$NetBSD: elf_machdep.h,v 1.3.14.1 1999/11/15 00:39:28 fvdl Exp $	*/

#define ELF32_MACHDEP_ENDIANNESS	ELFDATA2MSB
#define	ELF32_MACHDEP_ID_CASES						\
		case EM_SPARC:						\
		case EM_SPARC32PLUS:					\
			break;

#define	ELF64_MACHDEP_ENDIANNESS	ELFDATA2MSB
#define	ELF64_MACHDEP_ID_CASES						\
		case EM_SPARC32PLUS:					\
		case EM_SPARCV9:					\
			break;

#define R_SPARC_NONE		0
#define R_SPARC_8		1
#define R_SPARC_16		2
#define R_SPARC_32		3
#define R_SPARC_DISP8		4
#define R_SPARC_DISP16		5
#define R_SPARC_DISP32		6
#define R_SPARC_WDISP30		7
#define R_SPARC_WDISP22		8
#define R_SPARC_HI22		9
#define R_SPARC_22		10
#define R_SPARC_13		11
#define R_SPARC_LO10		12
#define R_SPARC_GOT10		13
#define R_SPARC_GOT13		14
#define R_SPARC_GOT22		15
#define R_SPARC_PC10		16
#define R_SPARC_PC22		17
#define R_SPARC_WPLT30		18
#define R_SPARC_COPY		19
#define R_SPARC_GLOB_DAT	20
#define R_SPARC_JMP_SLOT	21
#define R_SPARC_RELATIVE	22
#define R_SPARC_UA32		23
#define R_SPARC_PLT32		24
#define R_SPARC_HIPLT22		25
#define R_SPARC_LOPLT10		26
#define R_SPARC_PCPLT32		27
#define R_SPARC_PCPLT22		28
#define R_SPARC_PCPLT10		29
#define R_SPARC_10		30
#define R_SPARC_11		31
#define R_SPARC_64		32
#define R_SPARC_OLO10		33
#define R_SPARC_HH22		34
#define R_SPARC_HM10		35
#define R_SPARC_LM22		36
#define R_SPARC_PC_HH22		37
#define R_SPARC_PC_HM10		38
#define R_SPARC_PC_LM22		39
#define R_SPARC_WDISP16		40
#define R_SPARC_WDISP19		41
#define R_SPARC_GLOB_JMP	42
#define R_SPARC_7		43
#define R_SPARC_5		44
#define R_SPARC_6		45

#define R_TYPE(name)		__CONCAT(R_SPARC_,name)
