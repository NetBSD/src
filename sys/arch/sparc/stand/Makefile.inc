#	$NetBSD: Makefile.inc,v 1.7 2000/08/20 14:57:16 mrg Exp $

.ifndef __INCLUDED_STAND_MAKEFILE_INC
__INCLUDED_STAND_MAKEFILE_INC=

BINDIR=		/usr/mdec

.if ${MACHINE} == "sparc64"
CPPFLAGS+=	-DSTANDALONE -DSUN4U
CPPFLAGS+=	-I.

CFLAGS+=	-msoft-float
.endif

.endif
