# $NetBSD: Makefile,v 1.7 2019/08/24 13:20:52 rmind Exp $
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
