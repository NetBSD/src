#	$Id: Makefile,v 1.3 1993/12/02 07:21:19 mycroft Exp $

PROG=	ldconfig
SRCS=	ldconfig.c shlib.c etc.c
LDDIR?= $(.CURDIR)/..
LDFLAGS += -static
CFLAGS += -I$(LDDIR) -I$(.CURDIR) -I$(LDDIR)/$(MACHINE) -O
BINDIR= /sbin
MAN8 = ldconfig.0

.PATH: $(LDDIR) $(LDDIR)/$(MACHINE)

.include <bsd.prog.mk>
