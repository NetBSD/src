# $FreeBSD: src/gnu/usr.bin/groff/src/utils/indxbib/Makefile,v 1.7 2003/05/01 13:22:21 ru Exp $

DIST_SUBDIR=	src/utils/indxbib

PROG_CXX=	indxbib
SRCS=		indxbib.cpp signal.c
DPADD+=		${LIBBIB} ${LIBGROFF} ${LIBM}
LDADD+=		${LIBBIB} ${LIBGROFF} -lm
CLEANFILES=	${MAN}

COMMON_WORDS_FILE!=awk -F\" '/COMMON_WORDS_FILE/ { print $$2 }' ${.CURDIR}/../../include/defs.h

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=eign
FILESNAME=${COMMON_WORDS_FILE:T}
FILESDIR=${COMMON_WORDS_FILE:H}
.endif

.include <bsd.prog.mk>
