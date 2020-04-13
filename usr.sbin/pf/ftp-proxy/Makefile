#	$NetBSD: Makefile,v 1.8.30.1 2020/04/13 08:05:56 martin Exp $
#	$OpenBSD: Makefile,v 1.3 2006/11/26 11:31:13 deraadt Exp $

PROG=	ftp-proxy
SRCS=	ftp-proxy.c filter.c
MAN=	ftp-proxy.8

CFLAGS+= -I${.CURDIR}
LDADD+=	-levent
DPADD+= ${LIBEVENT}

CPPFLAGS+=-I${NETBSDSRCDIR}/sys

.include <bsd.own.mk>

# NPF support
.if (${MKNPF} != "no")
#SRCS+=		npf.c
#CPPFLAGS+=	-DWITH_NPF
#PROGDPLIBS+=	nv ${NETBSDSRCDIR}/external/bsd/libnv/lib
#LDADD+=		-lnpf
.endif

# IP Filter support
.if (${MKIPFILTER} != "no")
SRCS+=	ipf.c
CPPFLAGS+=-DWITH_IPF
.endif

BINDIR= /usr/sbin

.include <bsd.prog.mk>

.PATH: ${NETBSDSRCDIR}/dist/pf/usr.sbin/ftp-proxy
