#	$Id: Makefile,v 1.5 1993/08/02 17:43:00 mycroft Exp $

PROG=		groff
SRCS=		groff.cc pipeline.c
CFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBMATH)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
