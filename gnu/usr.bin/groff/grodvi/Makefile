# Makefile for grodvi

PROG=		grodvi
SRCS=		dvi.cc
CFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBDRIVER) $(LIBGROFF) -lm
DPADD+=		$(LIBDRIVER) $(LIBGROFF) $(LIBMATH)

.include <bsd.prog.mk>
.include "../../Makefile.inc"
.include "../Makefile.cfg"
