#	$NetBSD: Makefile,v 1.5 1999/02/13 02:54:24 lukem Exp $
#
# Makefile for uuconv

BINDIR=		$(sbindir)
BINOWN=		$(owner)

PROG=		uuconv
SRCS=		uuconv.c 
LDADD+=		$(LIBUNIX) $(LIBUUCONF) $(LIBUUCP)
DPADD+=		$(LIBUNIX) $(LIBUUCONF) $(LIBUUCP)
CPPFLAGS+=	-I$(.CURDIR)/../common_sources\
		-I$(.CURDIR)/../libuuconf\
		-DVERSION=\"$(VERSION)\"

MKMAN=		no

.include <bsd.prog.mk>
