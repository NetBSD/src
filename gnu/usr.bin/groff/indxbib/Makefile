#	$NetBSD: Makefile,v 1.12 2001/04/19 14:41:23 wiz Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/utils/indxbib
.PATH:		${DIST}

PROG=		indxbib
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
