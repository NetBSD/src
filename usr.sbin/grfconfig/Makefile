#   $NetBSD: Makefile,v 1.2 1997/06/23 04:51:24 mrg Exp $

.if ${MACHINE} == "amiga"
PROG=	grfconfig
CFLAGS+=-I${.CURDIR}/../../sys/arch -D${MACHINE}
.endif

MAN=	grfconfig.8

.include <bsd.prog.mk>
