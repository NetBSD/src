#	$NetBSD: Makefile,v 1.10.4.1 2000/07/19 14:08:19 mrg Exp $

PROG=	ldd
SRCS=	ldd.c \
	xmalloc.c debug.c map_object.c load.c search.c headers.c paths.c
MAN=	ldd.1

.PATH:	${.CURDIR}/..

.if (${MACHINE_ARCH} == "alpha") || (${MACHINE_ARCH} == "sparc64")
CPPFLAGS+= -DELFSIZE=64
.else
CPPFLAGS+= -DELFSIZE=32
.endif
.if (${MACHINE_ARCH} == "sparc") || (${MACHINE_ARCH} == "sparc64")
CPPFLAGS+= -DVARPSZ
.endif

LDELFSO=${.CURDIR}/../../../libexec/ld.elf_so
CPPFLAGS+= -I${LDELFSO} -DLIBDIR=\"${LIBDIR}\"	# -DDEBUG
CPPFLAGS+= -D_RTLD_SOURCE
.PATH: ${LDELFSO}

LDSTATIC?= -static


.include <bsd.prog.mk>
