#	$NetBSD: Makefile.inc,v 1.7.24.3 2004/09/21 13:22:39 skrll Exp $

.ifndef __INCLUDED_STAND_MAKEFILE_INC
__INCLUDED_STAND_MAKEFILE_INC=

BINDIR=		/usr/mdec

.if ${MACHINE} == "sparc64"
CPPFLAGS+=	-DSTANDALONE -DSUN4U
CPPFLAGS+=	-I.

CFLAGS+=	-msoft-float -ffreestanding
.endif

.endif
