# $NetBSD: Makefile,v 1.6 2008/10/22 17:54:23 hannken Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/utils/indxbib/Makefile,v 1.7 2003/05/01 13:22:21 ru Exp $

DIST_SUBDIR=	src/utils/indxbib

PROG=		indxbib
SRCS=		indxbib.cpp signal.c
DPADD+=		${LIBBIB} ${LIBGROFF} ${LIBM} ${LIBSUPCXX}
LDADD+=		${LIBBIB} ${LIBGROFF} -lm -lsupc++
CLEANFILES=	${MAN}

.include <bsd.own.mk>

.if make(install)
COMMON_WORDS_FILE!= ${TOOL_AWK} -F\" '/COMMON_WORDS_FILE/ { print $$2 }' \
		    ${.CURDIR}/../../include/defs.h

.if ${MKSHARE} != "no"
FILES=eign
FILESNAME=${COMMON_WORDS_FILE:T}
FILESDIR=${COMMON_WORDS_FILE:H}
.endif
.endif

.include <bsd.prog.mk>
