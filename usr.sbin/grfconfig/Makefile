#   $NetBSD: Makefile,v 1.4.2.1 1997/11/08 15:11:36 lukem Exp $

.if ${MACHINE} == "amiga"
PROG=		grfconfig
CPPFLAGS+=	-I${.CURDIR}/../../sys/arch -D${MACHINE}
.endif

MAN=	grfconfig.8

.include <bsd.prog.mk>
