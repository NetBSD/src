#	$NetBSD: Makefile,v 1.5 1999/02/13 02:55:01 lukem Exp $

.if ${MACHINE} == "amiga"
PROG=videomode
CPPFLAGS+=-I${.CURDIR}/../../sys/arch -D${MACHINE}
.else
MKOBJ=	no
.endif
MKMAN=	no

.include <bsd.prog.mk>
