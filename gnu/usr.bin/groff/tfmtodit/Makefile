# Makefile for tfmtodit

PROG=		tfmtodit
SRCS=		tfmtodit.cc
CFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBMATH)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
