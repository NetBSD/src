# Makefile for uuxqt
# $Id: Makefile,v 1.4 1994/12/22 09:32:33 cgd Exp $

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
