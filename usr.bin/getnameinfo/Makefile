# $NetBSD: Makefile,v 1.2 2025/02/06 20:59:00 christos Exp $

.include <bsd.own.mk>

WARNS= 5

GETADDRINFO:=${.PARSEDIR}/../getaddrinfo

.include "${GETADDRINFO}/Makefile.inc"

.PATH.c: ${GETADDRINFO}

PROG=		getnameinfo
SRCS=		getnameinfo.c support.c
CPPFLAGS+=	-I${GETADDRINFO}

.include <bsd.prog.mk>
