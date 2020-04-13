# $NetBSD: Makefile,v 1.5.14.2 2020/04/13 08:05:55 martin Exp $
#
# Public Domain
#

PROG=		npfd
MAN=		npfd.8

SRCS=		npfd.c npfd_log.c
CPPFLAGS+=	-I${.CURDIR}

LDADD+=		-lnpf -lpcap -lutil
DPADD+=		${LIBNPF} ${LIBPCAP} ${LIBUTIL}

WARNS=		5

.include <bsd.prog.mk>
