/*
 *	local definitions for libdos
 *
 *	written by ITOH Yasufumi
 *	public domain
 *
 *	$NetBSD: dos_asm.h,v 1.2.164.1 2011/03/05 15:10:08 bouyer Exp $
 */

#include <machine/asm.h>

#ifdef __ELF__
#define CERROR		__DOS_CERROR
#define PRCERROR	__DOS_PRCERROR
#else
#define CERROR		DOS_CERROR
#define PRCERROR	DOS_PRCERROR
#endif
