# $Id: Makefile,v 1.5 1993/10/16 21:52:27 pk Exp $
#

PROG=	ld
SRCS=	ld.c symbol.c lib.c shlib.c warnings.c etc.c rrs.c xbits.c md.c
NOMAN=	noman
CFLAGS += -g -I$(.CURDIR) -I$(.CURDIR)/$(MACHINE)

.PATH: $(.CURDIR)/$(MACHINE)

.include <bsd.prog.mk>
