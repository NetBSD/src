/*	$NetBSD: elf_machdep.h,v 1.7.56.1 2009/06/20 07:20:07 yamt Exp $	*/

#define	ELF32_MACHDEP_ENDIANNESS	ELFDATA2MSB
#define	ELF32_MACHDEP_ID_CASES						\
		case EM_PPC:						\
			break;

#define	ELF64_MACHDEP_ENDIANNESS	ELFDATA2MSB
#define	ELF64_MACHDEP_ID_CASES						\
		case EM_PPC64:						\
			break;

#define	ELF32_MACHDEP_ID	EM_PPC
#define	ELF64_MACHDEP_ID	EM_PPC64

#ifdef _LP64
#define ARCH_ELFSIZE		64	/* MD native binary size */
#else
#define ARCH_ELFSIZE		32	/* MD native binary size */
#endif

#define	R_PPC_NONE 		0
#define	R_PPC_32 		1
#define	R_PPC_24 		2
#define	R_PPC_16 		3
#define	R_PPC_16_LO 		4
#define	R_PPC_16_HI 		5 /* R_PPC_ADDIS */
#define	R_PPC_16_HA 		6
#define	R_PPC_14 		7
#define	R_PPC_14_TAKEN 		8
#define	R_PPC_14_NTAKEN 	9
#define	R_PPC_REL24 		10 /* R_PPC_BRANCH */
#define	R_PPC_REL14 		11
#define	R_PPC_REL14_TAKEN 	12
#define	R_PPC_REL14_NTAKEN 	13
#define	R_PPC_GOT16 		14
#define	R_PPC_GOT16_LO 		15
#define	R_PPC_GOT16_HI 		16
#define	R_PPC_GOT16_HA 		17
#define	R_PPC_PLT24 		18
#define	R_PPC_COPY 		19
#define	R_PPC_GLOB_DAT 		20
#define	R_PPC_JMP_SLOT 		21
#define	R_PPC_RELATIVE 		22
#define	R_PPC_LOCAL24PC 	23
#define	R_PPC_U32 		24
#define	R_PPC_U16 		25
#define	R_PPC_REL32 		26
#define	R_PPC_PLT32 		27
#define	R_PPC_PLTREL32 		28
#define	R_PPC_PLT16_LO 		29
#define	R_PPC_PLT16_HI 		30
#define	R_PPC_PLT16_HA 		31
#define	R_PPC_SDAREL 		32

/* TLS relocations */
#define	R_PPC_TLS		67

#define	R_PPC_DTPMOD32		68
#define	R_PPC_TPREL16		69
#define	R_PPC_TPREL16_LO	70
#define	R_PPC_TPREL16_HI	71
#define	R_PPC_TPREL16_HA	72
#define	R_PPC_TPREL32		73
#define	R_PPC_DTPREL16		74
#define	R_PPC_DTPREL16_LO	75
#define	R_PPC_DTPREL16_HI	76
#define	R_PPC_DTPREL16_HA	77
#define	R_PPC_DTPREL32		78

#define	R_PPC_GOT_TLSGD16	79
#define	R_PPC_GOT_TLSGD16_LO	80
#define	R_PPC_GOT_TLSGD16_HI	81
#define	R_PPC_GOT_TLSGD16_HA	82
#define	R_PPC_GOT_TLSLD16	83
#define	R_PPC_GOT_TLSLD16_LO	84
#define	R_PPC_GOT_TLSLD16_HI	85
#define	R_PPC_GOT_TLSLD16_HA	86

#define	R_PPC_GOT_TPREL16	87
#define	R_PPC_GOT_TPREL16_LO	88
#define	R_PPC_GOT_TPREL16_HI	89
#define	R_PPC_GOT_TPREL16_HA	90
#define	R_PPC_GOT_DTPREL16	91
#define	R_PPC_GOT_DTPREL16_LO	92
#define	R_PPC_GOT_DTPREL16_HI	93
#define	R_PPC_GOT_DTPREL16_HA	94
#define	R_PPC_TLSGD		95
#define	R_PPC_TLSLD		96

#define R_TYPE(name) 		__CONCAT(R_PPC_,name)
