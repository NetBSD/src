# $Id: Makefile,v 1.11.2.1 1994/08/19 11:26:36 mycroft Exp $
#

PROG=	ld
SRCS=	ld.c symbol.c lib.c shlib.c warnings.c etc.c rrs.c xbits.c md.c
CFLAGS += -g -I$(.CURDIR) -I$(.CURDIR)/$(MACHINE_ARCH)

LDADD+=	-lgnumalloc
DPADD+= /usr/lib/libgnumalloc.a

SUBDIR=	ldconfig ldd

.PATH: $(.CURDIR)/$(MACHINE_ARCH)

.include <bsd.prog.mk>
.if !defined(NOPIC)
SUBDIR+= rtld
.endif
