/*
 *	local definitions for libdos
 *
 *	written by ITOH Yasufumi
 *	public domain
 *
 *	$NetBSD: dos_asm.h,v 1.3 2011/02/21 02:31:59 itohy Exp $
 */

#include <machine/asm.h>

#ifdef __ELF__
#define CERROR		__DOS_CERROR
#define PRCERROR	__DOS_PRCERROR
#else
#define CERROR		DOS_CERROR
#define PRCERROR	DOS_PRCERROR
#endif
