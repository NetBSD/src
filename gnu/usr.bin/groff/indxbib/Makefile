#	$NetBSD: Makefile,v 1.13 2002/01/01 00:27:35 thorpej Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/utils/indxbib
.PATH:		${DIST}

PROG=		indxbib
PROG_CXX=
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
