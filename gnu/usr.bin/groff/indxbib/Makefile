#	$Id: Makefile,v 1.4 1993/08/02 17:43:45 mycroft Exp $

PROG=		indxbib
SRCS=		indxbib.cc dirnamemax.c signal.c
CFLAGS+=	-I$(.CURDIR)/../include
LDADD+= 	$(LIBBIB) $(LIBGROFF) -lm
DPADD+= 	$(LIBBIB) $(LIBGROFF) $(LIBMATH)

# XXX -- this depends on ../include/defs.h
COMMON_WORDS_FILE=	"/usr/share/dict/eign"

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 $(.CURDIR)/eign \
		$(DESTDIR)$(COMMON_WORDS_FILE)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
