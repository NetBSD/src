# $Id: Makefile,v 1.7 1993/10/26 06:56:08 cgd Exp $
#

PROG=	ld
SRCS=	ld.c symbol.c lib.c shlib.c warnings.c etc.c rrs.c xbits.c md.c
CFLAGS += -g -I$(.CURDIR) -I$(.CURDIR)/$(MACHINE)

LDADD+=	-lgnumalloc
DPADD+= /usr/lib/libgnumalloc.a

.PATH: $(.CURDIR)/$(MACHINE)

.include <bsd.prog.mk>
