#	$NetBSD: Makefile,v 1.3.2.1 1997/11/08 06:52:36 lukem Exp $
#
# Makefile for uupick

BINDIR=		$(bindir)

PROG=		uupick
SRCS=		uupick.c log.c copy.c
LDADD+=		$(LIBUNIX) $(LIBUUCONF) $(LIBUUCP)
DPADD+=		$(LIBUNIX) $(LIBUUCONF) $(LIBUUCP)
CPPFLAGS+=	-I$(.CURDIR)/../common_sources\
		-DVERSION=\"$(VERSION)\"

NOMAN=		noman

.include <bsd.prog.mk>
.PATH: $(.CURDIR)/../common_sources
