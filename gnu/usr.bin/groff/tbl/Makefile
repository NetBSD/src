#	$Id: Makefile,v 1.5 1993/08/02 17:45:13 mycroft Exp $

PROG=		tbl
SRCS=		main.cc table.cc
CFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBMATH)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
