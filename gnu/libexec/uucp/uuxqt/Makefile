#	$NetBSD: Makefile,v 1.5 1997/10/18 05:48:18 lukem Exp $
#
# Makefile for uuxqt

BINDIR=		$(sbindir)
BINOWN=		$(owner)
BINMODE=	4555

PROG=		uuxqt
SRCS=		uuxqt.c util.c log.c copy.c
LDADD+=		$(LIBUNIX) $(LIBUUCONF) $(LIBUUCP)
DPADD+=		$(LIBUNIX) $(LIBUUCONF) $(LIBUUCP)
CFLAGS+=	-I$(.CURDIR)/../common_sources \
		-DOWNER=\"$(owner)\" \
		-DVERSION=\"$(VERSION)\"

MAN=		uuxqt.8

.include <bsd.prog.mk>
.PATH: $(.CURDIR)/../common_sources
