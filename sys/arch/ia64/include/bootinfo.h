/*	$NetBSD: bootinfo.h,v 1.1.166.1 2020/04/08 14:07:41 martin Exp $	*/

/*-
 * Copyright (c) 1994, 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

struct bootinfo {
	uint64_t	bi_magic;		/* BOOTINFO_MAGIC */
#define	BOOTINFO_MAGIC		0xdeadbeeffeedface
	uint64_t	bi_version;		/* version 1 */
	uint64_t	bi_spare[5];		/* was: name of booted kernel */
	uint64_t	bi_hcdp;		/* DIG64 HCDP table */
	uint64_t	bi_fpswa;		/* FPSWA interface */
	uint64_t	bi_boothowto;		/* value for boothowto */
	uint64_t	bi_systab;		/* pa of EFI system table */
	uint64_t	bi_memmap;		/* pa of EFI memory map */
	uint64_t	bi_memmap_size;		/* size of EFI memory map */
	uint64_t	bi_memdesc_size;	/* sizeof EFI memory desc */
	uint32_t	bi_memdesc_version;	/* EFI memory desc version */
	uint32_t	bi_spare2;
	uint64_t	bi_unwindtab;		/* start of ia64 unwind table */
	uint64_t	bi_unwindtablen;	/* length of the table, in bytes */
	uint64_t	bi_symtab;		/* start of kernel sym table */
	uint64_t	bi_esymtab;		/* end of kernel sym table */
	uint64_t	bi_kernend;		/* end of kernel space */
	uint64_t	bi_envp;		/* environment */
};

extern struct bootinfo bootinfo;
