#	$NetBSD: Makefile,v 1.3.2.1 1997/11/08 06:52:40 lukem Exp $
#
# Makefile for uux

BINDIR=		$(bindir)
BINOWN=		$(owner)
BINMODE=	4555

PROG=		uux
SRCS=		uux.c util.c log.c copy.c
LDADD+=		$(LIBUNIX) $(LIBUUCONF) $(LIBUUCP)
DPADD+=		$(LIBUNIX) $(LIBUUCONF) $(LIBUUCP)
CPPFLAGS+=	-I$(.CURDIR)/../common_sources\
		-DVERSION=\"$(VERSION)\"

.include <bsd.prog.mk>
.PATH: $(.CURDIR)/../common_sources
