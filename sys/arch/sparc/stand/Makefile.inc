#	$NetBSD: Makefile.inc,v 1.7.24.4 2005/11/10 13:59:17 skrll Exp $

.ifndef __INCLUDED_STAND_MAKEFILE_INC
__INCLUDED_STAND_MAKEFILE_INC=

BINDIR=		/usr/mdec

CPPFLAGS+=	-DSTANDALONE -I.
CFLAGS+=	-msoft-float -ffreestanding

.if ${MACHINE} == "sparc64"
CPPFLAGS+=	-DSUN4U
.endif

.endif
