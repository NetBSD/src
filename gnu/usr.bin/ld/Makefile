# $Id: Makefile,v 1.11 1993/12/08 11:47:17 mycroft Exp $
#

PROG=	ld
SRCS=	ld.c symbol.c lib.c shlib.c warnings.c etc.c rrs.c xbits.c md.c
CFLAGS += -g -I$(.CURDIR) -I$(.CURDIR)/$(MACHINE_ARCH)

LDADD+=	-lgnumalloc
DPADD+= /usr/lib/libgnumalloc.a

SUBDIR=	ldconfig ldd
.if !defined(NOPIC)
SUBDIR+= rtld
.endif

.PATH: $(.CURDIR)/$(MACHINE_ARCH)

.include <bsd.prog.mk>
