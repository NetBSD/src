# $NetBSD: Makefile,v 1.1.96.1 2023/08/11 13:43:43 martin Exp $

NOMAN=yes
PROG=   qsafe
SRCS= qsafe.c qfile.c

QSIEVE=${.CURDIR}/../qsieve
.PATH: ${QSIEVE}
CPPFLAGS+=-I${QSIEVE}

COPTS.qsafe.c+= -Wno-error=deprecated-declarations

.include <bsd.prog.mk>
