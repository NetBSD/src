#	$NetBSD: Makefile,v 1.3.2.1 1997/11/08 06:52:25 lukem Exp $
#
# Makefile for uuchk

BINDIR=		$(sbindir)
BINOWN=		$(owner)

PROG=		uuchk
SRCS=		uuchk.c
LDADD+=		$(LIBUNIX) $(LIBUUCONF) $(LIBUUCP)
DPADD+=		$(LIBUNIX) $(LIBUUCONF) $(LIBUUCP)
CPPFLAGS+=	-I$(.CURDIR)/../common_sources\
		-DVERSION=\"$(VERSION)\"

NOMAN=		noman

.include <bsd.prog.mk>
