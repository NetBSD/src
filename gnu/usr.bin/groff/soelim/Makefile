#	$NetBSD: Makefile,v 1.5 1997/10/25 10:12:24 lukem Exp $

PROG=		soelim
SRCS=		soelim.cc
CPPFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF)
DPADD+=		$(LIBGROFF)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
