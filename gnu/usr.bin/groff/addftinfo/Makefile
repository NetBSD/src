#	$NetBSD: Makefile,v 1.4.2.1 1997/11/08 06:53:29 lukem Exp $

PROG=		addftinfo
SRCS=		addftinfo.cc guess.cc
CPPFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF)
DPADD+=		$(LIBGROFF)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
