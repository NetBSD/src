# $NetBSD: Makefile,v 1.1.4.2 2013/01/16 05:32:23 yamt Exp $

.include <bsd.own.mk>

NOMAN=		# defined

LIB=		bpfjit

# Private because it depends on the private sljit library.
LIBISPRIVATE=	yes

.PATH: ${NETBSDSRCDIR}/sys/net

SRCS=		bpfjit.c

SLJITDIST=	${NETBSDSRCDIR}/sys/external/bsd/sljit/dist
CPPFLAGS+=	-I${SLJITDIST}/sljit_src

WARNS=	4

.include <bsd.lib.mk>
