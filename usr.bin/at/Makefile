#	$NetBSD: Makefile,v 1.4 1998/08/21 19:39:41 tv Exp $

PROG=	at
SRCS=	at.c panic.c parsetime.c perm.c
CFLAGS+=-Wno-format-y2k
LINKS=	${BINDIR}/at ${BINDIR}/atq \
	${BINDIR}/at ${BINDIR}/atrm \
	${BINDIR}/at ${BINDIR}/batch
MLINKS=	at.1 batch.1 \
	at.1 atq.1 \
	at.1 atrm.1

BINOWN=	root
BINMODE= 4555

.include <bsd.prog.mk>
