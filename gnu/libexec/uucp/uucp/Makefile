#	$NetBSD: Makefile,v 1.3.2.1 1997/11/08 06:52:31 lukem Exp $
#
# Makefile for uucp

BINDIR=		$(bindir)
BINOWN=		$(owner)
BINMODE=	4555

PROG=		uucp
SRCS=		uucp.c util.c log.c copy.c
LDADD+=		$(LIBUNIX) $(LIBUUCONF) $(LIBUUCP)
DPADD+=		$(LIBUNIX) $(LIBUUCONF) $(LIBUUCP)
CPPFLAGS+=	-I$(.CURDIR)/../common_sources\
		-DVERSION=\"$(VERSION)\"

.include <bsd.prog.mk>
.PATH: $(.CURDIR)/../common_sources
