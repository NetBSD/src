|	XC compatible RENAME() function
|	int DOS_RENAME __P((const char *path, const char *newname));
|
|	written by Yasha (ITOH Yasufumi)
|	public domain
|
|	$NetBSD: dos_rename.S,v 1.1.4.1 1999/12/27 18:34:18 wrstuden Exp $

#include "dos_asm.h"

Lbufsz	=	92	| sizeof(struct dos_nameckbuf)
Lnamoff	=	67	| offsetof(struct dos_nameckbuf, name)
Lextoff	=	86	| offsetof(struct dos_nameckbuf, ext)

ENTRY_NOPROFILE(DOS_RENAME)
	link	%a6,#-Lbufsz*2			| allocate two  dos_nameckbuf

	moveal	%sp,%a1				| dos_nameckbuf for path

	pea	%sp@(Lbufsz)
	movel	%a6@(4 + 8),%sp@-		| newname
	.word	0xff37				| DOS_NAMECK
	tstl	%d0
	bmis	Lerr

	addql	#4,%sp

	movel	%a1,%sp@
	movel	%a6@(4 + 4),%sp@-		| (old) path
	.word	0xff37				| DOS_NAMECK
	tstl	%d0
	bmis	Lerr

	| we don't pop arguments since the same args are used by DOS_MOVE below

	| search end of dirname of (old) path
Lpath:	tstb	%a1@+
	bnes	Lpath
	subql	#1,%a1				| remove nul char

	| add new name
	lea	%sp@(8 + Lbufsz+Lnamoff),%a0	| new name (8: args on stack)
Lname:	moveb	%a0@+,%a1@+
	bnes	Lname
	subql	#1,%a1				| remove nul char

	| add new extention
	lea	%sp@(8 + Lbufsz+Lextoff),%a0	| new ext (8: args on stack)
Lext:	moveb	%a0@+,%a1@+
	bnes	Lext

	| we already have the arguments on stack
	bsr	_C_LABEL(DOS_MOVE)

	| version check of Human68k and error handlings are done in DOS_MOVE()

	unlk	%a6				| restore stack
	rts

Lerr:	unlk	%a6
	bra	CERROR
