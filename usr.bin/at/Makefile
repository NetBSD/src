#	$NetBSD: Makefile,v 1.6.10.1 2001/02/03 21:03:31 he Exp $

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
