#	$NetBSD: Makefile,v 1.3 1997/10/18 05:48:01 lukem Exp $
#
# Makefile for uuconv

BINDIR=		$(sbindir)
BINOWN=		$(owner)

PROG=		uuconv
SRCS=		uuconv.c 
LDADD+=		$(LIBUNIX) $(LIBUUCONF) $(LIBUUCP)
DPADD+=		$(LIBUNIX) $(LIBUUCONF) $(LIBUUCP)
CFLAGS+=	-I$(.CURDIR)/../common_sources\
		-I$(.CURDIR)/../libuuconf\
		-DVERSION=\"$(VERSION)\"

NOMAN=		noman

.include <bsd.prog.mk>
