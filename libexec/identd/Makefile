# $NetBSD: Makefile,v 1.11 2005/04/03 22:15:32 peter Exp $

.include <bsd.own.mk>

WARNS=	4
PROG=	identd
SRCS=	identd.c
MAN=	identd.8

# Build with IP Filter support?
.if (${MKIPFILTER} != "no")
SRCS+=	ipf.c
CPPFLAGS+=-I${NETBSDSRCDIR}/sys/dist/ipf -DWITH_IPF
.endif

# Build with pf support?
.if (${MKPF} != "no")
SRCS+=	pf.c
CPPFLAGS+=-DWITH_PF
.endif

.include <bsd.prog.mk>
