#	$NetBSD: Makefile,v 1.7 2005/08/20 17:21:29 rpaulo Exp $

.include <bsd.own.mk>

.if ${MACHINE} == "hp300"
PROG=	grfinfo
CPPFLAGS+= -I${NETBSDSRCDIR}/sys/arch/hp300
.endif

MAN=		grfinfo.1
MANSUBDIR=	/hp300

.include <bsd.prog.mk>
