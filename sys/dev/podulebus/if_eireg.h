/* $NetBSD: if_eireg.h,v 1.1.2.2 2001/03/19 23:58:13 bjh21 Exp $ */

/*
 * 2000 Ben Harris
 *
 * This file is in the public domain.
 */

/*
 * if_eireg.h - register definitions etc for the Acorn Ether1 card
 */

#ifndef _IF_EIREG_H_
#define _IF_EIREG_H_

/*
 * My understanding of this card is as follows.  Note that this is
 * mostly derived from reading other people's code, so it may be
 * hideously inaccurate.
 *
 * The card has three address spaces.  The ROM is mapped into the
 * bottom n (16?) bytes of SYNC address space, and contains the
 * expansion card ID information and the Ethernet address.  There is a
 * (write only?) set of registers at the start of the FAST address
 * space.  One of these performs miscellaneous control functions, and
 * the other acts as a page selector for the board memory.  The board
 * has 64k of RAM, and 4k pages of this can be mapped at offset 0x2000
 * in the FAST space by writing the page number to the page register.
 * The 82586 has access to the whole of this memory and (I believe)
 * sees it as the top 64k of its address space.
 */

/* Registers in the board's control space */
#define EI_PAGE		0
#define EI_CONTROL	1
#define EI_CTL_RESET	0x01
#define EI_CTL_LOOP	0x02
#define EI_CTL_ATTN	0x04
#define EI_CTL_CLI	0x08

/* Offset of base of memory in bus_addr_t units */
#define EI_MEMOFF	0x2000

/*
 * All addresses within board RAM are in bytes of actual RAM.  RAM is
 * 16 bis wide, and can only be accessed by word transfers
 * (bus_space_xxx_2).
 */
#define EI_MEMSIZE	0x10000
#define EI_MEMBASE	(0x1000000 - EI_MEMSIZE)
#define EI_PAGESIZE	0x1000
#define EI_NPAGES	(EI_MEMSIZE / EI_PAGESIZE)
#define ei_atop(a)	(((a) % EI_MEMSIZE) / EI_PAGESIZE)
#define ei_atopo(a)	((a) % EI_PAGESIZE)

#define EI_SCP_ADDR	IE_SCP_ADDR % EI_MEMSIZE

#define EI_ROMSIZE	16
#define EI_ROM_HWREV	8
#define EI_ROM_EADDR	9

#endif
