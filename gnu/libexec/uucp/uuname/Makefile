#	$NetBSD: Makefile,v 1.5 1999/02/13 02:54:24 lukem Exp $
#
# Makefile for uuname

BINDIR=		$(bindir)
BINOWN=		$(owner)
BINMODE=	4555

PROG=		uuname
SRCS=		uuname.c log.c
LDADD+=		$(LIBUNIX) $(LIBUUCONF) $(LIBUUCP)
DPADD+=		$(LIBUNIX) $(LIBUUCONF) $(LIBUUCP)
CPPFLAGS+=	-I$(.CURDIR)/../common_sources\
		-DVERSION=\"$(VERSION)\"

MKMAN=		no

.include <bsd.prog.mk>
.PATH: $(.CURDIR)/../common_sources
