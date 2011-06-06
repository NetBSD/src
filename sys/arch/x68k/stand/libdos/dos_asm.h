/*
 *	local definitions for libdos
 *
 *	written by ITOH Yasufumi
 *	public domain
 *
 *	$NetBSD: dos_asm.h,v 1.2.162.1 2011/06/06 09:07:03 jruoho Exp $
 */

#include <machine/asm.h>

#ifdef __ELF__
#define CERROR		__DOS_CERROR
#define PRCERROR	__DOS_PRCERROR
#else
#define CERROR		DOS_CERROR
#define PRCERROR	DOS_PRCERROR
#endif
