#	$NetBSD: Makefile,v 1.7 1998/12/17 22:34:55 thorpej Exp $

PROG=	ldd
SRCS=	ldd.c \
	xmalloc.c debug.c map_object.c load.c search.c headers.c paths.c
MAN=	ldd.1

.PATH:	${.CURDIR}/..

.if (${MACHINE_ARCH} == "alpha")
CPPFLAGS+= -DELFSIZE=64
.else
CPPFLAGS+= -DELFSIZE=32
.endif

LDELFSO=${.CURDIR}/../../../libexec/ld.elf_so
CPPFLAGS+= -I${LDELFSO} -DLIBDIR=\"${LIBDIR}\"	# -DDEBUG
.PATH: ${LDELFSO}

LDFLAGS+= -static


.include <bsd.prog.mk>
