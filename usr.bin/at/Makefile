#	$NetBSD: Makefile,v 1.6 1998/08/25 15:14:02 tv Exp $

PROG=		at
SRCS=		at.c panic.c parsetime.c perm.c
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
