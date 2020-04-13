# $NetBSD: Makefile,v 1.16.4.2 2020/04/13 08:03:17 martin Exp $

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

# Build with npf support?
.if (${MKNPF} != "no")
SRCS+=	npf.c
CPPFLAGS+=-DWITH_NPF
LDADD+=-lnpf
DPADD+=${LIBNPF}
.endif

# XXX
COPTS+=	-fno-strict-aliasing

.include <bsd.prog.mk>
