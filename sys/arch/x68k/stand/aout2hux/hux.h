/*
 *	Human68k .x file structure definitions
 *
 *	written by Yasha (ITOH Yasufumi)
 *	public domain
 *
 *	$NetBSD: hux.h,v 1.2 1999/02/02 10:00:18 itohy Exp $
 */
/*
 * Human68k ".x" executable format
 *
 *	----------------------------
 *	|  file header (64 bytes)  |
 *	|--------------------------|
 *	|  text                    |
 *	|--------------------------|
 *	|  data                    |
 *	|--------------------------|
 *	|  relocation table        |
 *	|--------------------------|
 *	|  symbol table            |
 *	|--------------------------|
 *	|  debugging information   |
 *	----------------------------
 *
 * text and data sections are loaded contiguous
 */

/* file header */

#define HUXMAGIC	0x4855		/* "HU" */

struct huxhdr {
	be_uint16_t	x_magic;	/* HUXMAGIC */
	u_int8_t	x_reserved1;	/* 0 */
	u_int8_t	x_loadmode;	/* 0: normal, 1: minimal memory,
					   2: high address */
	be_uint32_t	x_base;		/* base address (normally 0) */
	be_uint32_t	x_entry;	/* execution entry */
	be_uint32_t	x_text;		/* size of text section */
	be_uint32_t	x_data;		/* size of data section */
	be_uint32_t	x_bss;		/* size of bss */
	be_uint32_t	x_rsize;	/* size of relocation table */
	be_uint32_t	x_syms;		/* size of symbol info */
	be_uint32_t	x_db_line;	/* size of debugging info (line #) */
	be_uint32_t	x_db_syms;	/* size of debugging info (symbol) */
	be_uint32_t	x_db_str;	/* size of debugging info (string) */
	be_uint32_t	x_reserved2[4];	/* 0 */
	be_uint32_t	x_bindlist;	/* bind list offset */
};

/*
 * relocation information
 */

/* short format */
struct relinf_s {
	be_uint16_t	locoff_s;	/* offset */
};

/* long format */
#define HUXLRELMAGIC	0x0001
struct relinf_l {
	be_uint16_t	lrelmag;	/* HUXLRELMAGIC */
	be_uint16_t	locoff_l[2];	/* this would be  be_uint32_t
					 * if there were no alignment problems
					 */
};

#define HUX_MINLREL	0x10000		/* minimal value for long format */
