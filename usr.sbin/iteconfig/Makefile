#   $NetBSD: Makefile,v 1.5 1997/06/23 14:15:33 mrg Exp $

.if ${MACHINE} == "atari" || ${MACHINE} == "amiga"
PROG=iteconfig
CFLAGS+=-I${.CURDIR}/../../sys/arch -D${MACHINE}
.endif

MAN=	iteconfig.8

.include <bsd.prog.mk>
