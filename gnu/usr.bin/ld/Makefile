# $Id: Makefile,v 1.6 1993/10/18 23:34:29 pk Exp $
#

PROG=	ld
SRCS=	ld.c symbol.c lib.c shlib.c warnings.c etc.c rrs.c xbits.c md.c
CFLAGS += -g -I$(.CURDIR) -I$(.CURDIR)/$(MACHINE)

.PATH: $(.CURDIR)/$(MACHINE)

.include <bsd.prog.mk>
