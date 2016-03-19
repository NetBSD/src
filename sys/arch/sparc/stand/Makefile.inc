#	$NetBSD: Makefile.inc,v 1.10.142.1 2016/03/19 11:30:05 skrll Exp $

.ifndef __INCLUDED_STAND_MAKEFILE_INC
__INCLUDED_STAND_MAKEFILE_INC=

.include <bsd.own.mk>

BINDIR=		/usr/mdec

CPPFLAGS+=	-DSTANDALONE -I.
CFLAGS+=	-ffreestanding ${${ACTIVE_CC} == "gcc":? -msoft-float :}

.if ${MACHINE} == "sparc64"
CPPFLAGS+=	-DSUN4U
.endif

.endif
