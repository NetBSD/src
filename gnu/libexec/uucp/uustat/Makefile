#	$NetBSD: Makefile,v 1.6 1997/10/25 09:32:07 lukem Exp $
#
# Makefile for uustat

BINDIR=		$(bindir)
BINOWN=		$(owner)
BINMODE=	4555

PROG=		uustat
SRCS=		uustat.c util.c log.c copy.c
LDADD+=		$(LIBUNIX) $(LIBUUCONF) $(LIBUUCP)
DPADD+=		$(LIBUNIX) $(LIBUUCONF) $(LIBUUCP)
CPPFLAGS+=	-I$(.CURDIR)/../common_sources\
		-DOWNER=\"$(owner)\"\
		-DVERSION=\"$(VERSION)\"

.include <bsd.prog.mk>
.PATH: $(.CURDIR)/../common_sources
