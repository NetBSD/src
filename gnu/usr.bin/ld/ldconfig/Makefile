#	$Id: Makefile,v 1.7 1993/12/10 05:10:22 mycroft Exp $

PROG=	ldconfig
SRCS=	ldconfig.c shlib.c etc.c
LDDIR?= $(.CURDIR)/..
CFLAGS+=-I$(LDDIR) -I$(.CURDIR) -I$(LDDIR)/$(MACHINE_ARCH)
LDSTATIC=-static
BINDIR=	/sbin
MAN8=	ldconfig.0

.PATH: $(LDDIR) $(LDDIR)/$(MACHINE_ARCH)

.include <bsd.prog.mk>
