#	$NetBSD: Makefile,v 1.5.2.1 1997/11/08 06:52:42 lukem Exp $
#
# Makefile for uuxqt

BINDIR=		$(sbindir)
BINOWN=		$(owner)
BINMODE=	4555

PROG=		uuxqt
SRCS=		uuxqt.c util.c log.c copy.c
LDADD+=		$(LIBUNIX) $(LIBUUCONF) $(LIBUUCP)
DPADD+=		$(LIBUNIX) $(LIBUUCONF) $(LIBUUCP)
CPPFLAGS+=	-I$(.CURDIR)/../common_sources \
		-DOWNER=\"$(owner)\" \
		-DVERSION=\"$(VERSION)\"

MAN=		uuxqt.8

.include <bsd.prog.mk>
.PATH: $(.CURDIR)/../common_sources
