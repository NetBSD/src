#	$NetBSD: Makefile,v 1.4 1997/10/25 06:58:53 lukem Exp $

.if ${MACHINE} == "amiga"
PROG=videomode
CPPFLAGS+=-I${.CURDIR}/../../sys/arch -D${MACHINE}
.else
NOOBJ=	noobj
.endif
NOMAN=	noman

.include <bsd.prog.mk>
