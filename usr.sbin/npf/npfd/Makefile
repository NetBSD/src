# $NetBSD: Makefile,v 1.1 2016/12/27 22:20:00 rmind Exp $
#
# Public Domain
#

PROG=		npfd

SRCS=		npfd.c
CPPFLAGS+=	-I${.CURDIR}

LDADD+=		-lnpf -lpcap
DPADD+=		${LIBNPF} ${LIBPCAP}

WARNS=		5
NOLINT=		# disabled deliberately

.include <bsd.prog.mk>
