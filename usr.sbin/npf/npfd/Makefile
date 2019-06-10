# $NetBSD: Makefile,v 1.5.14.1 2019/06/10 22:10:35 christos Exp $
#
# Public Domain
#

PROG=		npfd
MAN=		npfd.8

SRCS=		npfd.c npfd_log.c
CPPFLAGS+=	-I${.CURDIR}

PROGDPLIBS+=	nv ${NETBSDSRCDIR}/external/bsd/libnv/lib
CPPFLAGS+=	-I ${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

LDADD+=		-lnpf -lpcap -lutil
DPADD+=		${LIBNPF} ${LIBPCAP} ${LIBUTIL}

WARNS=		5

.include <bsd.prog.mk>
