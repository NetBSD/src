#	$NetBSD: Makefile,v 1.1 1996/12/16 19:59:55 cgd Exp $

PROG=	ldd
SRCS=	ldd.c \
	xmalloc.c debug.c map_object.c load.c search.c headers.c paths.c
NOMAN=

CFLAGS+= -Wall -DELFSIZE=64 -I${.CURDIR}/../../libexec/ld.elf_so
CFLAGS+= -DLIBDIR=\"${LIBDIR}\"
#CFLAGS+= -DDEBUG

LDFLAGS+= -static

.PATH: ${.CURDIR}/../../libexec/ld.elf_so

.include <bsd.prog.mk>
