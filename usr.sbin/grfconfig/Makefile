#   $NetBSD: Makefile,v 1.7 2002/09/18 03:54:29 lukem Exp $

.include <bsd.own.mk>

.if ${MACHINE} == "amiga"
PROG=		grfconfig
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/arch -D${MACHINE}
.endif

MAN=		grfconfig.8
MANSUBDIR=	/amiga

.include <bsd.prog.mk>
