#	$Id: Makefile,v 1.5 1995/10/26 00:59:02 jtc Exp $

PROG=		indxbib
SRCS=		indxbib.cc dirnamemax.c signal.c
CFLAGS+=	-I$(.CURDIR)/../include
LDADD+= 	$(LIBBIB) $(LIBGROFF) -lm
DPADD+= 	$(LIBBIB) $(LIBGROFF) $(LIBM)

# XXX -- this depends on ../include/defs.h
COMMON_WORDS_FILE=	"/usr/share/dict/eign"

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 $(.CURDIR)/eign \
		$(DESTDIR)$(COMMON_WORDS_FILE)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
