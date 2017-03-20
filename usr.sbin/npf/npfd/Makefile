# $NetBSD: Makefile,v 1.4.2.3 2017/03/20 06:58:08 pgoyette Exp $
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
