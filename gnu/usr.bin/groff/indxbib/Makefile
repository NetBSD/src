#	$NetBSD: Makefile,v 1.11 1999/02/13 02:54:27 lukem Exp $

.include <bsd.own.mk>

PROG=		indxbib
SRCS=		indxbib.cc dirnamemax.c signal.c
CPPFLAGS+=	-I$(.CURDIR)/../include
LDADD+= 	$(LIBBIB) $(LIBGROFF) -lm
DPADD+= 	$(LIBBIB) $(LIBGROFF) $(LIBM)

COMMON_WORDS_FILE!=awk -F\" '/COMMON_WORDS_FILE/ { print $$2 }' ${.CURDIR}/../include/defs.h

.if ${MKSHARE} != "no"
FILES=eign
FILESNAME=${COMMON_WORDS_FILE:T}
FILESDIR=${COMMON_WORDS_FILE:H}
.endif

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
