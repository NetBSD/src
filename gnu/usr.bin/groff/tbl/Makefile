#	$NetBSD: Makefile,v 1.8 1997/10/25 10:12:27 lukem Exp $

PROG=		tbl
SRCS=		main.cc table.cc
CPPFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBM)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
