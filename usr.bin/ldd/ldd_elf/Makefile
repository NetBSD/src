#	$NetBSD: Makefile,v 1.5 1997/10/11 19:13:39 christos Exp $

WARNS ?= 1
PROG=	ldd
SRCS=	ldd.c \
	xmalloc.c debug.c map_object.c load.c search.c headers.c paths.c
NOMAN=

.if (${MACHINE_ARCH} == "alpha")
CPPFLAGS+= -DELFSIZE=64
.else
CPPFLAGS+= -DELFSIZE=32
.endif

LDELFSO=${.CURDIR}/../../libexec/ld.elf_so
CPPFLAGS+= -I${LDELFSO} -DLIBDIR=\"${LIBDIR}\"	# -DDEBUG
.PATH: ${LDELFSO}

LDFLAGS+= -static


.include <bsd.prog.mk>
