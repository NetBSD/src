#	$NetBSD: Makefile.inc,v 1.13 2016/05/19 15:41:18 christos Exp $

.ifndef __INCLUDED_STAND_MAKEFILE_INC
__INCLUDED_STAND_MAKEFILE_INC=

.include <bsd.own.mk>

BINDIR=		/usr/mdec
NOPIE=		yes

CPPFLAGS+=	-DSTANDALONE -I.
CFLAGS+=	-ffreestanding ${${ACTIVE_CC} == "gcc":? -msoft-float :}

.if ${MACHINE} == "sparc64"
CPPFLAGS+=	-DSUN4U
.endif

.endif
