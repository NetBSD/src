# Makefile for cu
# $Id: Makefile,v 1.3 1995/08/24 05:19:28 jtc Exp $

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
