#	$NetBSD: Makefile,v 1.8.2.1 1997/11/08 06:53:45 lukem Exp $

PROG=		indxbib
SRCS=		indxbib.cc dirnamemax.c signal.c
CPPFLAGS+=	-I$(.CURDIR)/../include
LDADD+= 	$(LIBBIB) $(LIBGROFF) -lm
DPADD+= 	$(LIBBIB) $(LIBGROFF) $(LIBM)

COMMON_WORDS_FILE!=awk -F\" '/COMMON_WORDS_FILE/ { print $$2 }' ${.CURDIR}/../include/defs.h

FILES=eign
FILESNAME=${COMMON_WORDS_FILE:T}
FILESDIR=${COMMON_WORDS_FILE:H}


.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
