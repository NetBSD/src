/*
 *	a.out file structure definitions
 *
 *	written by Yasha (ITOH Yasufumi)
 *	public domain
 *
 *	$NetBSD: aout68k.h,v 1.1 1998/09/01 19:51:08 itohy Exp $
 */
/*
 * NetBSD/m68k a.out format (OMAGIC, NMAGIC)
 *
 *	----------------------------
 *	|  file header (32 bytes)  |
 *	|--------------------------|
 *	|  text                    |
 *	|--------------------------|
 *	|  data                    |
 *	|--------------------------|
 *	|  text relocation table   |
 *	|--------------------------|
 *	|  data relocation table   |
 *	|--------------------------|
 *	|  symbol table            |
 *	|--------------------------|
 *	|  string table            |
 *	----------------------------
 *
 * OMAGIC: text and data segments are loaded contiguous
 * NMAGIC: data segment is loaded at the next page of the text
 */

struct aout_m68k {
	be_uint32_t	a_magic;	/* encoded magic number */
	be_uint32_t	a_text;		/* size of text section */
	be_uint32_t	a_data;		/* size of data section */
	be_uint32_t	a_bss;		/* size of bss */
	be_uint32_t	a_syms;		/* size of symbol table */
	be_uint32_t	a_entry;	/* entry point address */
	be_uint32_t	a_trsize;	/* size of text relocation */
	be_uint32_t	a_drsize;	/* size of data relocation */
};

#define AOUT_GET_MAGIC(e)	(((e)->a_magic.val[2]<<8) | (e)->a_magic.val[3])
#define	AOUT_OMAGIC	0407
#define	AOUT_NMAGIC	0410
#define	AOUT_ZMAGIC	0413	/* demand paging --- header is in the text */

#define AOUT_GET_FLAGS(e)	((e)->a_magic.val[0] >> 2)
#define AOUT_FLAG_PIC		0x10
#define AOUT_FLAG_DYNAMIC	0x20

/* machine ID */
#define AOUT_GET_MID(e)		((((e)->a_magic.val[0] & 0x03) << 8) |	\
					(e)->a_magic.val[1])
#define	AOUT_MID_M68K	135	/* m68k BSD binary, 8KB page */
#define	AOUT_MID_M68K4K	136	/* m68k BSD binary, 4KB page */

#define AOUT_PAGESIZE(e)	(AOUT_GET_MAGIC(e) == AOUT_OMAGIC ? 1 :	\
				AOUT_GET_MID(e) == AOUT_MID_M68K ? 8192 : 4096)
