/*	$NetBSD: elf_machdep.h,v 1.3 2003/03/13 13:44:18 scw Exp $	*/

#ifndef _BYTE_ORDER
#error Define _BYTE_ORDER!
#endif

#if _BYTE_ORDER == _LITTLE_ENDIAN
#define	ELF32_MACHDEP_ENDIANNESS	ELFDATA2LSB
#define	ELF64_MACHDEP_ENDIANNESS	ELFDATA2LSB
#else
#define	ELF32_MACHDEP_ENDIANNESS	ELFDATA2MSB
#define	ELF64_MACHDEP_ENDIANNESS	ELFDATA2MSB
#endif
#define	ELF32_MACHDEP_ID_CASES						\
		case EM_SH:						\
			break;

#define	ELF64_MACHDEP_ID_CASES						\
		case EM_SH:						\
			break;

#define	ELF32_MACHDEP_ID	EM_SH
#define	ELF64_MACHDEP_ID	EM_SH

#ifndef _LP64
#define	ARCH_ELFSIZE		32	/* MD native binary size */
#else
#define	ARCH_ELFSIZE		64	/* MD native binary size */
#endif

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
#define	EF_SH_SH5		0x0a

#define	EF_SH_HAS_DSP(x)	((x)&4)
#define	EF_SH_HAS_FP(x)		((x)&8)

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
#define	R_SH_SWITCH16		25
#define	R_SH_SWITCH32		26
#define	R_SH_USES		27
#define	R_SH_COUNT		28
#define	R_SH_ALIGN		29
#define	R_SH_CODE		30
#define	R_SH_DATA		31
#define	R_SH_LABEL		32
#define	R_SH_SWITCH8		33
#define	R_SH_GNU_VTINHERIT	34
#define	R_SH_GNU_VTENTRY	35
#define	R_SH_LOOP_START		36
#define	R_SH_LOOP_END		37
#define R_SH_DIR5U		45
#define R_SH_DIR6U		46
#define R_SH_DIR6S		47
#define R_SH_DIR10S		48
#define R_SH_DIR10SW		49
#define R_SH_DIR10SL		50
#define R_SH_DIR10SQ		51
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
#define	R_SH_GOTPLT32		168
#define	R_SH_GOT_LOW16		169
#define	R_SH_GOT_MEDLOW16	170
#define	R_SH_GOT_MEDHI16	171
#define	R_SH_GOT_HI16		172
#define	R_SH_GOTPLT_LOW16	173
#define	R_SH_GOTPLT_MEDLOW16	174
#define	R_SH_GOTPLT_MEDHI16	175
#define	R_SH_GOTPLT_HI16	176
#define	R_SH_PLT_LOW16		177
#define	R_SH_PLT_MEDLOW16	178
#define	R_SH_PLT_MEDHI16	179
#define	R_SH_PLT_HI16		180
#define	R_SH_GOTOFF_LOW16	181
#define	R_SH_GOTOFF_MEDLOW16	182
#define	R_SH_GOTOFF_MEDHI16	183
#define	R_SH_GOTOFF_HI16	184
#define	R_SH_GOTPC_LOW16	185
#define	R_SH_GOTPC_MEDLOW16	186
#define	R_SH_GOTPC_MEDHI16	187
#define	R_SH_GOTPC_HI16		188
#define	R_SH_GOT10BY4		189
#define	R_SH_GOTPLT10BY4	190
#define	R_SH_GOT10BY8		191
#define	R_SH_GOTPLT10BY8	192
#define	R_SH_COPY64		193
#define	R_SH_GLOB_DAT64		194
#define	R_SH_JMP_SLOT64		195
#define	R_SH_RELATIVE64		196
#define	R_SH_SHMEDIA_CODE	242
#define	R_SH_PT_16		243
#define	R_SH_IMMS16		244
#define	R_SH_IMMU16		245
#define	R_SH_IMM_LOW16		246
#define	R_SH_IMM_LOW16_PCREL	247
#define	R_SH_IMM_MEDLOW16	248
#define	R_SH_IMM_MEDLOW16_PCREL	249
#define	R_SH_IMM_MEDHI16	250
#define	R_SH_IMM_MEDHI16_PCREL	251
#define	R_SH_IMM_HI16		252
#define	R_SH_IMM_HI16_PCREL	253
#define	R_SH_64			254
#define	R_SH_64_PCREL		255

#define	R_TYPE(name)	__CONCAT(R_SH_,name)
