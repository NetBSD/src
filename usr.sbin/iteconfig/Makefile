#   $NetBSD: Makefile,v 1.5.2.1 1997/11/08 15:11:53 lukem Exp $

.if ${MACHINE} == "atari" || ${MACHINE} == "amiga"
PROG=iteconfig
CPPFLAGS+=-I${.CURDIR}/../../sys/arch -D${MACHINE}
.endif

MAN=	iteconfig.8

.include <bsd.prog.mk>
