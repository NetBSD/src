#	$Id: Makefile,v 1.9 1994/12/22 09:35:29 cgd Exp $

PROG=	ldconfig
SRCS=	ldconfig.c shlib.c etc.c
LDDIR?= $(.CURDIR)/..
CFLAGS+=-I$(LDDIR) -I$(.CURDIR) -I$(LDDIR)/$(MACHINE_ARCH)
LDSTATIC=-static
BINDIR=	/sbin
MAN=	ldconfig.8

.PATH: $(LDDIR) $(LDDIR)/$(MACHINE_ARCH)

.if ($(MACHINE_ARCH) == i386)
CFLAGS += -DSTANDARD_SEARCH_DIRS="\"/usr/lib\", \"/usr/X386/lib\""
.endif

.include <bsd.prog.mk>
