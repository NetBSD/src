#	$NetBSD: Makefile,v 1.7 2002/09/19 03:09:32 lukem Exp $
#
# Makefile for uuconv

NOMAN=		# defined

BINDIR=		$(sbindir)
BINOWN=		$(owner)

PROG=		uuconv
SRCS=		uuconv.c 
LDADD+=		$(LIBUNIX) $(LIBUUCONF) $(LIBUUCP)
DPADD+=		$(LIBUNIX) $(LIBUUCONF) $(LIBUUCP)
CPPFLAGS+=	-I$(.CURDIR)/../common_sources\
		-I$(.CURDIR)/../libuuconf\
		-DVERSION=\"$(VERSION)\"

.include <bsd.prog.mk>
