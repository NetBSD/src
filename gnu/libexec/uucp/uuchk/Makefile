#	$NetBSD: Makefile,v 1.8 2002/09/19 03:09:32 lukem Exp $
#
# Makefile for uuchk

NOMAN=		# defined

BINDIR=		/usr/sbin
BINOWN=		$(owner)

PROG=		uuchk
SRCS=		uuchk.c
LDADD+=		$(LIBUNIX) $(LIBUUCONF) $(LIBUUCP)
DPADD+=		$(LIBUNIX) $(LIBUUCONF) $(LIBUUCP)
CPPFLAGS+=	-I$(.CURDIR)/../common_sources\
		-DVERSION=\"$(VERSION)\"

.include <bsd.prog.mk>
