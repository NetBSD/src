#	$NetBSD: Makefile,v 1.3 1997/06/24 08:01:39 mikel Exp $

.if ${MACHINE} == "amiga"
PROG=videomode
CFLAGS+=-I${.CURDIR}/../../sys/arch -D${MACHINE}
.else
NOOBJ=	noobj
.endif
NOMAN=	noman

.include <bsd.prog.mk>
