#	$Id: Makefile,v 1.5 1995/10/26 00:59:12 jtc Exp $

PROG=		refer
SRCS=		command.cc ref.cc refer.cc token.cc label.cc
CFLAGS+=	-I. -I$(.CURDIR)/../include
LDADD+=		$(LIBBIB) $(LIBGROFF) -lm
DPADD+=		$(LIBBIB) $(LIBGROFF) $(LIBM)

CLEANFILES+=	label.cc label.tab.h

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
