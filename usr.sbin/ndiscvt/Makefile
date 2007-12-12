# $FreeBSD: src/usr.sbin/ndiscvt/Makefile,v 1.5 2004/02/23 20:21:21 johan Exp $

WARNS?=	4

.PATH:	${.CURDIR}/../../sys/compat/ndis

# .if ${MACHINE} == "i386"
PROG=	ndiscvt
# .endif

MAN=	ndiscvt.8
SRCS=	ndiscvt.c
SRCS+=	subr_pe.c
SRCS+=	inf.c inf-token.l inf-parse.y

CFLAGS+=-I. -I${.CURDIR} -I${.CURDIR}/../../sys

YHEADER=yes
YPREFIX=ndiscvt_
LPREFIX=${YPREFIX}

.include <bsd.prog.mk>
