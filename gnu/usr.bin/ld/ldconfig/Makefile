#	$Id: Makefile,v 1.1 1993/10/23 00:16:57 pk Exp $

PROG=	ldconfig
SRCS=	ldconfig.c shlib.c etc.c
LDDIR?= $(.CURDIR)/..
CFLAGS += -I$(LDDIR) -I$(.CURDIR) -I$(LDDIR)/$(MACHINE) -O
BINDIR= ${DESTDIR}/sbin
MAN8 = ldconfig.0

.PATH: $(LDDIR) $(LDDIR)/$(MACHINE)

.include <bsd.prog.mk>
