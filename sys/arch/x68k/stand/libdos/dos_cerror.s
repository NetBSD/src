|	Writes Human68k DOS error number to  dos_errno.
|	Called on errors of DOS calls.
|
|	written by Yasha (ITOH Yasufumi)
|	public domain
|
|	$NetBSD: dos_cerror.s,v 1.1 1998/09/01 19:53:25 itohy Exp $

	.globl	DOS_CERROR

DOS_CERROR:
	movel	d0,sp@-
	negl	d0
	cmpil	#80,d0
	bnes	Lno_eexisists
	moveq	#36,d0
Lno_eexisists:
	movel	d0,_dos_errno
	movel	sp@+,d0
	rts
