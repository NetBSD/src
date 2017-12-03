/*	$NetBSD: elf_machdep.h,v 1.10.22.1 2017/12/03 11:36:42 jdolecek Exp $	*/

#include <machine/endian_machdep.h>

#if _BYTE_ORDER == _LITTLE_ENDIAN
#define	ELF32_MACHDEP_ENDIANNESS	ELFDATA2LSB
#else
#define	ELF32_MACHDEP_ENDIANNESS	ELFDATA2MSB
#endif
#define	ELF32_MACHDEP_ID_CASES						\
		case EM_SH:						\
			break;

#define	ELF64_MACHDEP_ENDIANNESS	XXX	/* break compilation */
#define	ELF64_MACHDEP_ID_CASES						\
		/* no 64-bit ELF machine types supported */

#define	ELF32_MACHDEP_ID	EM_SH

#define	KERN_ELFSIZE		32
#define	ARCH_ELFSIZE		32	/* MD native binary size */

/*
 * SuperH ELF header flags.
 */
#define	EF_SH_MACH_MASK		0x1f

#define	EF_SH_UNKNOWN		0x00
#define	EF_SH_SH1		0x01
#define	EF_SH_SH2		0x02
#define	EF_SH_SH3		0x03
#define	EF_SH_DSP		0x04
#define	EF_SH_SH3_DSP		0x05
#define	EF_SH_SH3E		0x08
#define	EF_SH_SH4		0x09

#define	EF_SH_HAS_DSP(x)	((x) & EF_SH_DSP)
#define	EF_SH_HAS_FP(x)		((x) & EF_SH_SH3E)


#define	R_SH_NONE		0
#define	R_SH_DIR32		1
#define	R_SH_REL32		2
#define	R_SH_DIR8WPN		3
#define	R_SH_IND12W		4
#define	R_SH_DIR8WPL		5
#define	R_SH_DIR8WPZ		6
#define	R_SH_DIR8BP		7
#define	R_SH_DIR8W		8
#define	R_SH_DIR8L		9
#define	R_SH_LOOP_START		10
#define	R_SH_LOOP_END		11
#define	R_SH_GNU_VTINHERIT	22
#define	R_SH_GNU_VTENTRY	23
#define	R_SH_SWITCH8		24
#define	R_SH_SWITCH16		25
#define	R_SH_SWITCH32		26
#define	R_SH_USES		27
#define	R_SH_COUNT		28
#define	R_SH_ALIGN		29
#define	R_SH_CODE		30
#define	R_SH_DATA		31
#define	R_SH_LABEL		32

/* TLS relocations */
#define	R_SH_TLS_GD_32		144
#define	R_SH_TLS_LD_32		145
#define	R_SH_TLS_LDO_32		146
#define	R_SH_TLS_IE_32		147
#define	R_SH_TLS_LE_32		148
#define	R_SH_TLS_DTPMOD32	149
#define	R_SH_TLS_DTPOFF32	150
#define	R_SH_TLS_TPOFF32	151

#define	R_SH_GOT32		160
#define	R_SH_PLT32		161
#define	R_SH_COPY		162
#define	R_SH_GLOB_DAT		163
#define	R_SH_JMP_SLOT		164
#define	R_SH_RELATIVE		165
#define	R_SH_GOTOFF		166
#define	R_SH_GOTPC		167

#define	R_TYPE(name)	__CONCAT(R_SH_,name)
