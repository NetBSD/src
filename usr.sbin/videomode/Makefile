#	$NetBSD: Makefile,v 1.2 1997/06/23 04:51:33 mrg Exp $

.if ${MACHINE} == "amiga"
PROG=videomode
CFLAGS+=-I${.CURDIR}/../../sys/arch -D${MACHINE}
.endif

NOMAN=	noman

.include <bsd.prog.mk>
