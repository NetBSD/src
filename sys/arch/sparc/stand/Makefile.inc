#	$NetBSD: Makefile.inc,v 1.6.22.1 2000/11/20 20:25:49 bouyer Exp $

.ifndef __INCLUDED_STAND_MAKEFILE_INC
__INCLUDED_STAND_MAKEFILE_INC=

BINDIR=		/usr/mdec

.if ${MACHINE} == "sparc64"
CPPFLAGS+=	-DSTANDALONE -DSUN4U
CPPFLAGS+=	-I.

CFLAGS+=	-msoft-float
.endif

.endif
