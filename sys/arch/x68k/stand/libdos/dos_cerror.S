|	Writes Human68k DOS error number to  dos_errno.
|	Called on errors of DOS calls.
|
|	written by Yasha (ITOH Yasufumi)
|	public domain
|
|	$NetBSD: dos_cerror.S,v 1.1 1999/11/11 08:14:43 itohy Exp $

#include "dos_asm.h"
#include "dos_errno.h"

ASENTRY_NOPROFILE(CERROR)
	movel	%d0,%sp@-
	negl	%d0
	cmpil	#_DOS_EEXIST,%d0
	bnes	Lno_eexisists
	moveq	#DOS_EEXIST,%d0
Lno_eexisists:
	movel	%d0,_C_LABEL(dos_errno)
	movel	%sp@+,%d0
	rts
