#	$Id: Makefile,v 1.6 1993/08/02 17:43:13 mycroft Exp $

PROG=		grops
SRCS=		ps.cc psrm.cc
CFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBDRIVER) $(LIBGROFF) -lm
DPADD+=		$(LIBDRIVER) $(LIBGROFF) $(LIBMATH)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
