#	$NetBSD: Makefile.inc,v 1.6.32.1 2000/08/26 00:32:52 mrg Exp $

.ifndef __INCLUDED_STAND_MAKEFILE_INC
__INCLUDED_STAND_MAKEFILE_INC=

BINDIR=		/usr/mdec

.if ${MACHINE} == "sparc64"
CPPFLAGS+=	-DSTANDALONE -DSUN4U
CPPFLAGS+=	-I.

CFLAGS+=	-msoft-float
.endif

.endif
