#	$NetBSD: Makefile,v 1.6 2001/12/12 01:49:01 tv Exp $
#
# Makefile for uulog

BINDIR=		$(bindir)

PROG=		uulog
SRCS=		uulog.c log.c
LDADD+=		$(LIBUNIX) $(LIBUUCONF) $(LIBUUCP)
DPADD+=		$(LIBUNIX) $(LIBUUCONF) $(LIBUUCP)
CPPFLAGS+=	-I$(.CURDIR)/../common_sources\
		-DVERSION=\"$(VERSION)\"

NOMAN=		# defined

.include <bsd.prog.mk>
.PATH: $(.CURDIR)/../common_sources
