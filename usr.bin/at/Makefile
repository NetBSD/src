#	$NetBSD: Makefile,v 1.7 2000/10/15 14:51:14 kleink Exp $

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

CWARNFLAGS+=	-Wno-format-y2k

.include <bsd.prog.mk>
