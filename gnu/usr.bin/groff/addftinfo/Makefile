# Makefile for addftinfo

PROG=		addftinfo
SRCS=		addftinfo.cc guess.cc
CFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF)
DPADD+=		$(LIBGROFF)

.include <bsd.prog.mk>
.include "../../Makefile.inc"
.include "../Makefile.cfg"
