#	$NetBSD: Makefile,v 1.14 2002/01/01 01:38:28 thorpej Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/utils/indxbib
.PATH:		${DIST}

PROG_CXX=	indxbib
SRCS=		indxbib.cc dirnamemax.c signal.c
LDADD+= 	$(LIBBIB) $(LIBGROFF) -lm
DPADD+= 	$(LIBBIB) $(LIBGROFF) $(LIBM)

COMMON_WORDS_FILE!=awk -F\" '/COMMON_WORDS_FILE/ { print $$2 }' ${.CURDIR}/../include/defs.h

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=eign
FILESNAME=${COMMON_WORDS_FILE:T}
FILESDIR=${COMMON_WORDS_FILE:H}
.endif

.include <bsd.prog.mk>
