#	$NetBSD: Makefile,v 1.4.2.1 1997/11/08 06:53:58 lukem Exp $

PROG=		soelim
SRCS=		soelim.cc
CPPFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF)
DPADD+=		$(LIBGROFF)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
