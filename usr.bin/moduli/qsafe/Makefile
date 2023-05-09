# $NetBSD: Makefile,v 1.2 2023/05/09 20:02:59 christos Exp $

NOMAN=yes
PROG=   qsafe
SRCS= qsafe.c qfile.c

QSIEVE=${.CURDIR}/../qsieve
.PATH: ${QSIEVE}
CPPFLAGS+=-I${QSIEVE}

COPTS.qsafe.c+= -Wno-error=deprecated-declarations

.include <bsd.prog.mk>
