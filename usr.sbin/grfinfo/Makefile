#	$NetBSD: Makefile,v 1.6 2002/09/18 03:54:29 lukem Exp $

.include <bsd.own.mk>

.if ${MACHINE} == "hp300"
PROG=	grfinfo
CPPFLAGS+= -I${NETBSDSRCDIR}/sys/arch/hp300
.endif

MAN=	grfinfo.1

.include <bsd.prog.mk>
