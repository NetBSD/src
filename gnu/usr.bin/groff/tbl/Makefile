#	$NetBSD: Makefile,v 1.7.2.1 1997/11/08 06:54:00 lukem Exp $

PROG=		tbl
SRCS=		main.cc table.cc
CPPFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBM)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
