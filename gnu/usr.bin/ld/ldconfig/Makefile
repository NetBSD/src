#	$Id: Makefile,v 1.8 1994/06/10 15:16:53 pk Exp $

PROG=	ldconfig
SRCS=	ldconfig.c shlib.c etc.c
LDDIR?= $(.CURDIR)/..
CFLAGS+=-I$(LDDIR) -I$(.CURDIR) -I$(LDDIR)/$(MACHINE_ARCH)
LDSTATIC=-static
BINDIR=	/sbin
MAN8=	ldconfig.0

.PATH: $(LDDIR) $(LDDIR)/$(MACHINE_ARCH)

.if ($(MACHINE_ARCH) == i386)
CFLAGS += -DSTANDARD_SEARCH_DIRS="\"/usr/lib\", \"/usr/X386/lib\""
.endif

.include <bsd.prog.mk>
