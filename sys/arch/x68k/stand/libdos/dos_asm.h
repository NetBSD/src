/*
 *	local definitions for libdos
 *
 *	written by Yasha (ITOH Yasufumi)
 *	public domain
 *
 *	$NetBSD: dos_asm.h,v 1.1 1999/11/11 08:14:43 itohy Exp $
 */

#include <machine/asm.h>

#ifdef __ELF__
#define CERROR		__DOS_PRCERROR
#define PRCERROR	__DOS_CERROR
#else
#define CERROR		DOS_CERROR
#define PRCERROR	DOS_PRCERROR
#endif
