#	$Id: Makefile,v 1.3 1993/08/02 17:45:08 mycroft Exp $

PROG=		soelim
SRCS=		soelim.cc
CFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF)
DPADD+=		$(LIBGROFF)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
