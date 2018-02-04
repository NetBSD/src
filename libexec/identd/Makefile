# $NetBSD: Makefile,v 1.16 2018/02/04 09:03:23 mrg Exp $

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
LDADD+=-lnpf -lprop
DPADD+=${LIBNPF} ${LIBPROP}
.endif

# XXX
COPTS+=	-fno-strict-aliasing

.include <bsd.prog.mk>
