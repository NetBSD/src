|	Writes Human68k DOS process error number to  dos_errno.
|	Called on errors of DOS calls for processes.
|
|	written by Yasha (ITOH Yasufumi)
|	public domain
|
|	$NetBSD: dos_procerr.s,v 1.1 1998/09/01 19:53:26 itohy Exp $

	.globl	DOS_PRCERROR

DOS_PRCERROR:
	movel	d0,sp@-
	cmpil	#0xffff0100,d0
	jcs	Lnoterrcode
	negl	d0
Lwerr:
	movel	d0,_dos_errno
	movel	sp@+,d0
	rts

Lnoterrcode:
	swap	d0
	addqw	#1,d0
	jeq	Lillid

	moveq	#37,d0
	jra	Lwerr

Lillid:
	moveq	#40,d0
	jra	Lwerr
