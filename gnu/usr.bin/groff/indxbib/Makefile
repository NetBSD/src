#	$NetBSD: Makefile,v 1.10 1998/09/27 17:22:04 lukem Exp $

PROG=		indxbib
SRCS=		indxbib.cc dirnamemax.c signal.c
CPPFLAGS+=	-I$(.CURDIR)/../include
LDADD+= 	$(LIBBIB) $(LIBGROFF) -lm
DPADD+= 	$(LIBBIB) $(LIBGROFF) $(LIBM)

COMMON_WORDS_FILE!=awk -F\" '/COMMON_WORDS_FILE/ { print $$2 }' ${.CURDIR}/../include/defs.h

.if !defined(NOSHARE)
FILES=eign
FILESNAME=${COMMON_WORDS_FILE:T}
FILESDIR=${COMMON_WORDS_FILE:H}
.endif

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
