#	$Id: Makefile,v 1.3 1993/08/02 17:45:18 mycroft Exp $

PROG=		tfmtodit
SRCS=		tfmtodit.cc
CFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBMATH)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
