#	$NetBSD: Makefile.inc,v 1.8 2003/07/30 15:58:38 mrg Exp $

.ifndef __INCLUDED_STAND_MAKEFILE_INC
__INCLUDED_STAND_MAKEFILE_INC=

BINDIR=		/usr/mdec

.if ${MACHINE} == "sparc64"
CPPFLAGS+=	-DSTANDALONE -DSUN4U
CPPFLAGS+=	-I.

CFLAGS+=	-msoft-float -ffreestanding
.endif

.endif
