#   $NetBSD: Makefile,v 1.6 1997/10/25 06:58:18 lukem Exp $

.if ${MACHINE} == "atari" || ${MACHINE} == "amiga"
PROG=iteconfig
CPPFLAGS+=-I${.CURDIR}/../../sys/arch -D${MACHINE}
.endif

MAN=	iteconfig.8

.include <bsd.prog.mk>
