#	$Id: Makefile,v 1.4 1993/12/05 00:55:03 cgd Exp $

PROG=	ldconfig
SRCS=	ldconfig.c shlib.c etc.c
LDDIR?= $(.CURDIR)/..
LDFLAGS += -static
CFLAGS += -I$(LDDIR) -I$(.CURDIR) -I$(LDDIR)/$(MACHINE_ARCH) -O
BINDIR= /sbin
MAN8 = ldconfig.0

.PATH: $(LDDIR) $(LDDIR)/$(MACHINE_ARCH)

.include <bsd.prog.mk>
