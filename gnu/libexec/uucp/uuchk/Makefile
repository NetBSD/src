#	$NetBSD: Makefile,v 1.5 1999/02/13 02:54:24 lukem Exp $
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

MKMAN=		no

.include <bsd.prog.mk>
