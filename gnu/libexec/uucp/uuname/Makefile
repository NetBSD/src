#	$NetBSD: Makefile,v 1.6 2001/12/12 01:49:01 tv Exp $
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

NOMAN=		# defined

.include <bsd.prog.mk>
.PATH: $(.CURDIR)/../common_sources
