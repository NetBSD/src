#   $NetBSD: Makefile,v 1.4 1997/10/17 05:58:23 lukem Exp $

.if ${MACHINE} == "amiga"
PROG=		grfconfig
CFLAGS+=	-I${.CURDIR}/../../sys/arch -D${MACHINE}
.endif

MAN=	grfconfig.8

.include <bsd.prog.mk>
