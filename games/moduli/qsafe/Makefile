# $NetBSD: Makefile,v 1.1 2006/01/19 23:23:58 elad Exp $

NOMAN=yes
PROG=   qsafe
SRCS= qsafe.c qfile.c
QSIEVE=${.CURDIR}/../qsieve
.PATH: ${QSIEVE}
CPPFLAGS+=-I${QSIEVE}

.include <bsd.prog.mk>
