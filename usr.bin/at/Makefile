#	$NetBSD: Makefile,v 1.2.4.1 1998/10/24 00:21:49 cgd Exp $

PROG=	at
SRCS=	at.c panic.c parsetime.c perm.c
LINKS=	${BINDIR}/at ${BINDIR}/atq \
	${BINDIR}/at ${BINDIR}/atrm \
	${BINDIR}/at ${BINDIR}/batch
MLINKS=	at.1 batch.1 \
	at.1 atq.1 \
	at.1 atrm.1

BINOWN=	root
BINMODE= 4555

.include <bsd.prog.mk>
