/* $NetBSD: podulebusreg.h,v 1.1.2.1 2002/05/30 15:32:00 gehenna Exp $ */

/*
 * 2000 Ben Harris
 *
 * This file is in the public domain.
 */

#ifndef _PODULEBUSREG_H_
#define _PODULEBUSREG_H_

#define MAX_PODULES 4
#define PODULE_GAP (0x4000 >> 2)

/* Podule CMOS bytes */
#define PODULE_CMOS_BASE	(112 + 0x40)
#define PODULE_CMOS_PERSLOT	4

/* Podule identity byte (ECId) flags */
#define ECID_IRQING	0x01
#define ECID_NOTPRESENT	0x02
#define ECID_FIQING	0x04
#define ECID_ID_MASK	0x78
#define ECID_ID_SHIFT	3
#define ECID_ID_EXTEND	0x00
#define ECID_NONCONF	0x80

/* Extended ECId bytes */
#define EXTECID_F1		1
#define EXTECID_F1_CD		0x01
#define EXTECID_F1_IS		0x02
#define EXTECID_F1_W_MASK	0xc0
#define EXTECID_F1_W_8BIT	0x00
#define EXTECID_F1_W_16BIT	0x40
#define EXTECID_F1_W_32BIT	0x80

#define EXTECID_PLO	3
#define EXTECID_PHI	4
#define EXTECID_MLO	5
#define EXTECID_MHI	6

#define EXTECID_SIZE	8

/* Extended interrupt pointers */
#define IRQPTR_SIZE	8

/* Chunk types */
#define CHUNK_RISCOS_LOADER	0x80
#define CHUNK_RISCOS_MODULE	0x81
#define CHUNK_RISCOS_BBCROM	0x82
#define CHUNK_RISCOS_SPRITE	0x83
#define CHUNK_OS1_LOADER	0x90
#define CHUNK_RISCIX_LOADER	0xa0
#define CHUNK_HELIOS		0xa3
#define CHUNK_DEV_LINK		0xf0
#define CHUNK_DEV_SERIAL	0xf1 /* ASCII string */
#define CHUNK_DEV_DATE		0xf2 /* ASCII string */
#define CHUNK_DEV_MODS		0xf3 /* ASCII string */
#define CHUNK_DEV_PLACE		0xf4 /* ASCII string */
#define CHUNK_DEV_DESCR		0xf5 /* ASCII string */
#define CHUNK_DEV_PARTNO	0xf6 /* ASCII string */
#define CHUNK_DEV_EADDR		0xf7 /* Six bytes */
#define CHUNK_DEV_HWREV		0xf8 /* Four-byte LE word */
#define CHUNK_DEV_ROMCRC	0xf9

#endif
