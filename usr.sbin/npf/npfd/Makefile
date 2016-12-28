# $NetBSD: Makefile,v 1.2 2016/12/28 01:25:48 christos Exp $
#
# Public Domain
#

NOMAN=
PROG=		npfd

SRCS=		npfd.c npfd_log.c
CPPFLAGS+=	-I${.CURDIR}

LDADD+=		-lnpf -lpcap
DPADD+=		${LIBNPF} ${LIBPCAP}

WARNS=		5
NOLINT=		# disabled deliberately

.include <bsd.prog.mk>
