# Makefile for indxbib

PROG=		indxbib
SRCS=		indxbib.cc dirnamemax.c signal.c
CFLAGS+=	-I$(.CURDIR)/../include
LDADD+= 	$(LIBBIB) $(LIBGROFF) -lm
DPADD+= 	$(LIBBIB) $(LIBGROFF) $(LIBMATH)

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 $(.CURDIR)/eign \
		$(DESTDIR)$(common_words_file)

.include <bsd.prog.mk>
.include "../../Makefile.inc"
.include "../Makefile.cfg"
