#	$NetBSD: Makefile,v 1.4 1997/10/25 09:31:54 lukem Exp $
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
