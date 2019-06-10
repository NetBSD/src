# $NetBSD: Makefile,v 1.16.4.1 2019/06/10 22:05:29 christos Exp $

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

PROGDPLIBS+= nv ${NETBSDSRCDIR}/external/bsd/libnv/lib
CPPFLAGS+= -I ${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

LDADD+=-lnpf
DPADD+=${LIBNPF}
.endif

# XXX
COPTS+=	-fno-strict-aliasing

.include <bsd.prog.mk>
