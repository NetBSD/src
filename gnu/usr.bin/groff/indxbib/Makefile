#	$NetBSD: Makefile,v 1.8 1997/10/18 05:59:04 lukem Exp $

PROG=		indxbib
SRCS=		indxbib.cc dirnamemax.c signal.c
CFLAGS+=	-I$(.CURDIR)/../include
LDADD+= 	$(LIBBIB) $(LIBGROFF) -lm
DPADD+= 	$(LIBBIB) $(LIBGROFF) $(LIBM)

COMMON_WORDS_FILE!=awk -F\" '/COMMON_WORDS_FILE/ { print $$2 }' ${.CURDIR}/../include/defs.h

FILES=eign
FILESNAME=${COMMON_WORDS_FILE:T}
FILESDIR=${COMMON_WORDS_FILE:H}


.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
