#	$NetBSD: Makefile,v 1.7 2002/09/19 03:09:32 lukem Exp $
#
# Makefile for uulog

NOMAN=		# defined

BINDIR=		$(bindir)

PROG=		uulog
SRCS=		uulog.c log.c
LDADD+=		$(LIBUNIX) $(LIBUUCONF) $(LIBUUCP)
DPADD+=		$(LIBUNIX) $(LIBUUCONF) $(LIBUUCP)
CPPFLAGS+=	-I$(.CURDIR)/../common_sources\
		-DVERSION=\"$(VERSION)\"

.include <bsd.prog.mk>
.PATH: $(.CURDIR)/../common_sources
