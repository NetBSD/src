#   $NetBSD: Makefile,v 1.5 1997/10/25 06:58:00 lukem Exp $

.if ${MACHINE} == "amiga"
PROG=		grfconfig
CPPFLAGS+=	-I${.CURDIR}/../../sys/arch -D${MACHINE}
.endif

MAN=	grfconfig.8

.include <bsd.prog.mk>
