# Makefile for uuxqt
# $Id: Makefile,v 1.3 1994/10/24 22:30:26 jtc Exp $

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

MAN8=		uuxqt.0

.include <bsd.prog.mk>
.PATH: $(.CURDIR)/../common_sources
