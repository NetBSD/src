#	$NetBSD: Makefile,v 1.7 2001/12/12 01:49:00 tv Exp $
#
# Makefile for uuchk

BINDIR=		/usr/sbin
BINOWN=		$(owner)

PROG=		uuchk
SRCS=		uuchk.c
LDADD+=		$(LIBUNIX) $(LIBUUCONF) $(LIBUUCP)
DPADD+=		$(LIBUNIX) $(LIBUUCONF) $(LIBUUCP)
CPPFLAGS+=	-I$(.CURDIR)/../common_sources\
		-DVERSION=\"$(VERSION)\"

NOMAN=		# defined

.include <bsd.prog.mk>
