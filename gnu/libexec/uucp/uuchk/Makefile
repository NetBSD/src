#	$NetBSD: Makefile,v 1.6 2000/12/11 15:49:08 is Exp $
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

MKMAN=		no

.include <bsd.prog.mk>
