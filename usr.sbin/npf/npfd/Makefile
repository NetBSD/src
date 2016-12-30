# $NetBSD: Makefile,v 1.3 2016/12/30 19:55:46 christos Exp $
#
# Public Domain
#

NOMAN=
PROG=		npfd

#DBG=-g
SRCS=		npfd.c npfd_log.c
CPPFLAGS+=	-I${.CURDIR}

LDADD+=		-lnpf -lpcap -lutil
DPADD+=		${LIBNPF} ${LIBPCAP} ${LIBUTIL}

WARNS=		5
NOLINT=		# disabled deliberately

.include <bsd.prog.mk>
