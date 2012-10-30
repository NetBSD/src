# $NetBSD: Makefile,v 1.12.6.1 2012/10/30 18:59:22 yamt Exp $

.include <bsd.own.mk>

PROG=	identd
SRCS=	identd.c
MAN=	identd.8

# Build with IP Filter support?
.if (${MKIPFILTER} != "no")
SRCS+=	ipf.c
CPPFLAGS+=-DWITH_IPF
.endif

# Build with pf support?
.if (${MKPF} != "no")
SRCS+=	pf.c
CPPFLAGS+=-DWITH_PF
.endif

.include <bsd.prog.mk>
