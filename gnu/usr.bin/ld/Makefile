#	$Id: Makefile,v 1.9 1993/12/05 00:54:41 cgd Exp $
#

PROG=	ld
SRCS=	ld.c symbol.c lib.c shlib.c warnings.c etc.c rrs.c xbits.c md.c
CFLAGS += -g -I$(.CURDIR) -I$(.CURDIR)/$(MACHINE_ARCH)

LDADD+=	-lgnumalloc
DPADD+= /usr/lib/libgnumalloc.a

SUBDIR=	ldconfig ldd rtld

.PATH: $(.CURDIR)/$(MACHINE_ARCH)

.include <bsd.prog.mk>
