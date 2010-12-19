#	$NetBSD: Makefile,v 1.6 2010/12/19 22:52:08 pgoyette Exp $

PROG=	powerd
SRCS=	powerd.c

RUMPPRG=	powerd

.PATH: ${.CURDIR}/../../common/lib/libprop
CPPFLAGS+=	-DRUMP_ACTION
RUMPSRCS+=	prop_kern.c

DPADD+=	${LIBPROP} ${LIBUTIL}
LDADD+=	-lprop -lutil

MAN=	powerd.8

.include <bsd.prog.mk>
