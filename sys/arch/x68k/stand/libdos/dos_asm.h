/*
 *	local definitions for libdos
 *
 *	written by Yasha (ITOH Yasufumi)
 *	public domain
 *
 *	$NetBSD: dos_asm.h,v 1.2 2000/06/19 03:42:31 itohy Exp $
 */

#include <machine/asm.h>

#ifdef __ELF__
#define CERROR		__DOS_CERROR
#define PRCERROR	__DOS_PRCERROR
#else
#define CERROR		DOS_CERROR
#define PRCERROR	DOS_PRCERROR
#endif
