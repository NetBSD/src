#	$NetBSD: Makefile,v 1.3 1997/10/18 05:48:09 lukem Exp $
#
# Makefile for uupick

BINDIR=		$(bindir)

PROG=		uupick
SRCS=		uupick.c log.c copy.c
LDADD+=		$(LIBUNIX) $(LIBUUCONF) $(LIBUUCP)
DPADD+=		$(LIBUNIX) $(LIBUUCONF) $(LIBUUCP)
CFLAGS+=	-I$(.CURDIR)/../common_sources\
		-DVERSION=\"$(VERSION)\"

NOMAN=		noman

.include <bsd.prog.mk>
.PATH: $(.CURDIR)/../common_sources
