#	$Id: Makefile,v 1.3 1993/08/02 17:41:44 mycroft Exp $

PROG=		addftinfo
SRCS=		addftinfo.cc guess.cc
CFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF)
DPADD+=		$(LIBGROFF)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
