/*
 *	local definitions for libdos
 *
 *	written by Yasha (ITOH Yasufumi)
 *	public domain
 *
 *	$NetBSD: dos_asm.h,v 1.1.2.2 1999/11/15 00:39:55 fvdl Exp $
 */

#include <machine/asm.h>

#ifdef __ELF__
#define CERROR		__DOS_PRCERROR
#define PRCERROR	__DOS_CERROR
#else
#define CERROR		DOS_CERROR
#define PRCERROR	DOS_PRCERROR
#endif
