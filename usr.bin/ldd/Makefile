#	$NetBSD: Makefile,v 1.4 1997/05/08 21:11:41 gwr Exp $

PROG=	ldd
SRCS=	ldd.c \
	xmalloc.c debug.c map_object.c load.c search.c headers.c paths.c
NOMAN=

.if (${MACHINE_ARCH} == "alpha")
CFLAGS+= -DELFSIZE=64
.else
CFLAGS+= -DELFSIZE=32
.endif

CFLAGS+= -Wall  -I${.CURDIR}/../../libexec/ld.elf_so
CFLAGS+= -DLIBDIR=\"${LIBDIR}\"
#CFLAGS+= -DDEBUG



LDFLAGS+= -static

.PATH: ${.CURDIR}/../../libexec/ld.elf_so

.include <bsd.prog.mk>
