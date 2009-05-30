/*	$NetBSD: elf_machdep.h,v 1.11 2009/05/30 05:56:52 skrll Exp $	*/

#define	ELF32_MACHDEP_ID_CASES						\
		case EM_MIPS:						\
			break;

#define	ELF64_MACHDEP_ID_CASES						\
		/* no 64-bit ELF machine types supported */


#define	ELF32_MACHDEP_ID	EM_MIPS
#define	ELF64_MACHDEP_ID	EM_MIPS

#define ARCH_ELFSIZE		32	/* MD native binary size */

/* mips relocs.  */

#define R_MIPS_NONE		0
#define R_MIPS_16		1
#define R_MIPS_32		2
#define R_MIPS_REL32		3
#define R_MIPS_REL		R_MIPS_REL32
#define R_MIPS_26		4
#define R_MIPS_HI16		5	/* high 16 bits of symbol value */
#define R_MIPS_LO16		6	/* low 16 bits of symbol value */
#define R_MIPS_GPREL16		7  	/* GP-relative reference  */
#define R_MIPS_LITERAL		8 	/* Reference to literal section  */
#define R_MIPS_GOT16		9	/* Reference to global offset table */
#define R_MIPS_GOT		R_MIPS_GOT16
#define R_MIPS_PC16		10  	/* 16 bit PC relative reference */
#define R_MIPS_CALL16 		11  	/* 16 bit call thru glbl offset tbl */
#define R_MIPS_CALL		R_MIPS_CALL16
#define R_MIPS_GPREL32		12

/* 13, 14, 15 are not defined at this point. */
#define R_MIPS_UNUSED1		13
#define R_MIPS_UNUSED2		14
#define R_MIPS_UNUSED3		15

/*
 * The remaining relocs are apparently part of the 64-bit Irix ELF ABI.
 */
#define R_MIPS_SHIFT5		16
#define R_MIPS_SHIFT6		17

#define R_MIPS_64		18
#define R_MIPS_GOT_DISP		19
#define R_MIPS_GOT_PAGE		20
#define R_MIPS_GOT_OFST		21
#define R_MIPS_GOT_HI16		22
#define R_MIPS_GOT_LO16		23
#define R_MIPS_SUB 		24
#define R_MIPS_INSERT_A		25
#define R_MIPS_INSERT_B		26
#define R_MIPS_DELETE		27
#define R_MIPS_HIGHER		28
#define R_MIPS_HIGHEST		29
#define R_MIPS_CALL_HI16	30
#define R_MIPS_CALL_LO16	31
#define R_MIPS_SCN_DISP		32
#define R_MIPS_REL16		33
#define R_MIPS_ADD_IMMEDIATE	34
#define R_MIPS_PJUMP		35
#define R_MIPS_RELGOT		36

#define R_MIPS_TLS_DTPMOD32	38	/* Module number 32 bit */
#define R_MIPS_TLS_DTPREL32	39	/* Module-relative offset 32 bit */
#define R_MIPS_TLS_DTPMOD64	40	/* Module number 64 bit */
#define R_MIPS_TLS_DTPREL64	41	/* Module-relative offset 64 bit */
#define R_MIPS_TLS_GD		42	/* 16 bit GOT offset for GD */
#define R_MIPS_TLS_LDM		43	/* 16 bit GOT offset for LDM */
#define R_MIPS_TLS_DTPREL_HI16	44	/* Module-relative offset, high 16 bits */
#define R_MIPS_TLS_DTPREL_LO16	45	/* Module-relative offset, low 16 bits */
#define R_MIPS_TLS_GOTTPREL	46	/* 16 bit GOT offset for IE */
#define R_MIPS_TLS_TPREL32	47	/* TP-relative offset, 32 bit */
#define R_MIPS_TLS_TPREL64	48	/* TP-relative offset, 64 bit */
#define R_MIPS_TLS_TPREL_HI16	49	/* TP-relative offset, high 16 bits */
#define R_MIPS_TLS_TPREL_LO16	50	/* TP-relative offset, low 16 bits */

#define R_MIPS_max		51

#define R_TYPE(name)		__CONCAT(R_MIPS_,name)

/* mips dynamic tags */

#define DT_MIPS_RLD_VERSION	0x70000001
#define DT_MIPS_TIME_STAMP	0x70000002
#define DT_MIPS_ICHECKSUM	0x70000003
#define DT_MIPS_IVERSION	0x70000004
#define DT_MIPS_FLAGS		0x70000005
#define DT_MIPS_BASE_ADDRESS	0x70000006
#define DT_MIPS_CONFLICT	0x70000008
#define DT_MIPS_LIBLIST		0x70000009
#define DT_MIPS_CONFLICTNO	0x7000000b
#define	DT_MIPS_LOCAL_GOTNO	0x7000000a	/* number of local got ents */
#define DT_MIPS_LIBLISTNO	0x70000010
#define	DT_MIPS_SYMTABNO	0x70000011	/* number of .dynsym entries */
#define DT_MIPS_UNREFEXTNO	0x70000012
#define	DT_MIPS_GOTSYM		0x70000013	/* first dynamic sym in got */
#define DT_MIPS_HIPAGENO	0x70000014
#define	DT_MIPS_RLD_MAP		0x70000016	/* address of loader map */

#ifdef _KERNEL
#ifdef _KERNEL_OPT
#include "opt_compat_netbsd.h"
#endif
#ifdef COMPAT_16
/*
 * Up to 1.6, the ELF dynamic loader (ld.elf_so) was not relocatable.
 * Tell the kernel ELF exec code not to try relocating the interpreter
 * for dynamically-linked ELF binaries.
 */
#define ELF_INTERP_NON_RELOCATABLE
#endif /* COMPAT_16 */
#endif /* _KERNEL */
