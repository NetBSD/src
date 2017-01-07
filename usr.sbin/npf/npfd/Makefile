# $NetBSD: Makefile,v 1.4.2.2 2017/01/07 08:57:00 pgoyette Exp $
#
# Public Domain
#

NOMAN=
PROG=		npfd

DBG=-g
SRCS=		npfd.c npfd_log.c
CPPFLAGS+=	-I${.CURDIR}

LDADD+=		-lnpf -lpcap -lutil
DPADD+=		${LIBNPF} ${LIBPCAP} ${LIBUTIL}

WARNS=		5
NOLINT=		# disabled deliberately

.include <bsd.prog.mk>
