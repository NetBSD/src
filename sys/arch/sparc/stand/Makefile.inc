#	$NetBSD: Makefile.inc,v 1.11 2016/02/27 19:26:14 joerg Exp $

.ifndef __INCLUDED_STAND_MAKEFILE_INC
__INCLUDED_STAND_MAKEFILE_INC=

BINDIR=		/usr/mdec

CPPFLAGS+=	-DSTANDALONE -I.
CFLAGS+=	-ffreestanding ${${ACTIVE_CC} == "gcc":? -msoft-float :}

.if ${MACHINE} == "sparc64"
CPPFLAGS+=	-DSUN4U
.endif

.endif
