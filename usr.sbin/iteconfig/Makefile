#   $NetBSD: Makefile,v 1.7 2002/09/18 03:54:32 lukem Exp $

.include <bsd.own.mk>

.if ${MACHINE} == "atari" || ${MACHINE} == "amiga"
PROG=iteconfig
CPPFLAGS+=-I${NETBSDSRCDIR}/sys/arch -D${MACHINE}
.endif

MAN=	iteconfig.8

.include <bsd.prog.mk>
