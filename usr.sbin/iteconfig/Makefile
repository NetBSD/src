#   $NetBSD: Makefile,v 1.4 1997/06/23 04:51:30 mrg Exp $

.if ${MACHINE} == "atari" || ${MACHINE} == "amiga"
PROG=iteconfig
CFLAGS+=-I${.CURDIR}/../../sys/arch -D${MACHINE}
.endif

MAN=	iteconfig.8 iteconfig_atari.8 iteconfig_amiga.8

.include <bsd.prog.mk>
