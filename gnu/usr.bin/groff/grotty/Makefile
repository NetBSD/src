#	$Id: Makefile,v 1.6 1993/08/02 17:43:18 mycroft Exp $

PROG=		grotty
SRCS=		tty.cc
CFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBDRIVER) $(LIBGROFF) -lm
DPADD+=		$(LIBDRIVER) $(LIBGROFF) $(LIBMATH)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
