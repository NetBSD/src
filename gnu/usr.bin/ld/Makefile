#	$NetBSD: Makefile,v 1.14 1997/03/24 19:54:35 perry Exp $

PROG=	ld
SRCS=	ld.c symbol.c lib.c shlib.c warnings.c etc.c rrs.c xbits.c md.c
CFLAGS += -g -I$(.CURDIR) -I$(.CURDIR)/$(MACHINE_ARCH)

LDADD+=	-lgnumalloc
DPADD+= /usr/lib/libgnumalloc.a

SUBDIR=	ldconfig ldd

.PATH: $(.CURDIR)/$(MACHINE_ARCH)

.include <bsd.own.mk>	# for NOPIC definition

.if !defined(NOPIC)
SUBDIR+= rtld
.endif

.include <bsd.prog.mk>
