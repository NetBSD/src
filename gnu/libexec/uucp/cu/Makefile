#	$NetBSD: Makefile,v 1.4 1997/10/18 05:47:50 lukem Exp $
#
# Makefile for cu

BINDIR=		$(bindir)
BINOWN=		$(owner)
BINMODE=	4555

PROG=		cu
SRCS=		cu.c prot.c log.c chat.c conn.c copy.c
LDADD+=		$(LIBUNIX) $(LIBUUCONF) $(LIBUUCP)
DPADD+=		$(LIBUNIX) $(LIBUUCONF) $(LIBUUCP)
CFLAGS+=	-I$(.CURDIR)/../common_sources\
		-DVERSION=\"$(VERSION)\"

.include <bsd.prog.mk>
.PATH: $(.CURDIR)/../common_sources
