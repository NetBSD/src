#	$NetBSD: Makefile,v 1.10.10.1 2009/05/13 19:19:43 jym Exp $

USE_FORT?=	yes	# setuid

PROG=		at
SRCS=		at.c panic.c parsetime.c perm.c stime.c
LINKS=		${BINDIR}/at ${BINDIR}/atq \
		${BINDIR}/at ${BINDIR}/atrm \
		${BINDIR}/at ${BINDIR}/batch
MLINKS=		at.1 batch.1 \
		at.1 atq.1 \
		at.1 atrm.1

BINOWN=		root
BINMODE=	4555

DPADD+=		${LIBUTIL}
LDADD+=		-lutil

.include <bsd.prog.mk>
