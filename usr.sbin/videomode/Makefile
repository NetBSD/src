#	$NetBSD: Makefile,v 1.3.2.1 1997/11/08 15:12:53 lukem Exp $

.if ${MACHINE} == "amiga"
PROG=videomode
CPPFLAGS+=-I${.CURDIR}/../../sys/arch -D${MACHINE}
.else
NOOBJ=	noobj
.endif
NOMAN=	noman

.include <bsd.prog.mk>
