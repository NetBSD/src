#   $NetBSD: Makefile,v 1.6 2002/07/09 22:18:37 jdolecek Exp $

.if ${MACHINE} == "amiga"
PROG=		grfconfig
CPPFLAGS+=	-I${.CURDIR}/../../sys/arch -D${MACHINE}
.endif

MAN=		grfconfig.8
MANSUBDIR=	/amiga

.include <bsd.prog.mk>
