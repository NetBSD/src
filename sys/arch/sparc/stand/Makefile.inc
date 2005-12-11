#	$NetBSD: Makefile.inc,v 1.10 2005/12/11 12:19:08 christos Exp $

.ifndef __INCLUDED_STAND_MAKEFILE_INC
__INCLUDED_STAND_MAKEFILE_INC=

BINDIR=		/usr/mdec

CPPFLAGS+=	-DSTANDALONE -I.
CFLAGS+=	-msoft-float -ffreestanding

.if ${MACHINE} == "sparc64"
CPPFLAGS+=	-DSUN4U
.endif

.endif
