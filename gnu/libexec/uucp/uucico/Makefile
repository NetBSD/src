# Makefile for uucico
# $Id: Makefile,v 1.4 1994/12/22 09:32:23 cgd Exp $

BINDIR=		$(sbindir)
BINOWN=		$(owner)
BINMODE=	4555

PROG=		uucico
SRCS=		uucico.c trans.c send.c rec.c xcmd.c prot.c protg.c protf.c \
		prott.c prote.c proti.c protj.c protz.c time.c chat.c \
		conn.c copy.c log.c tcp.c tli.c util.c
LDADD+=		$(LIBUNIX) $(LIBUUCONF) $(LIBUUCP)
DPADD+=		$(LIBUNIX) $(LIBUUCONF) $(LIBUUCP)
CFLAGS+=	-I$(.CURDIR)/../common_sources\
		-DOWNER=\"$(owner)\" \
		-DVERSION=\"$(VERSION)\"

MAN=		uucico.8

.include <bsd.prog.mk>
.PATH: $(.CURDIR)/../common_sources
