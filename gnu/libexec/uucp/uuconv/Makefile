#	$NetBSD: Makefile,v 1.3.2.1 1997/11/08 06:52:29 lukem Exp $
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

NOMAN=		noman

.include <bsd.prog.mk>
