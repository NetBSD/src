#	$NetBSD: Makefile,v 1.6.2.1 1997/11/08 06:53:57 lukem Exp $

PROG=		refer
SRCS=		command.cc ref.cc refer.cc token.cc label.cc
CPPFLAGS+=	-I. -I$(.CURDIR)/../include
LDADD+=		$(LIBBIB) $(LIBGROFF) -lm
DPADD+=		$(LIBBIB) $(LIBGROFF) $(LIBM)

CLEANFILES+=	label.cc label.tab.h

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
