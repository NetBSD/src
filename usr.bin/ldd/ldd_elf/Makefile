#	$NetBSD: Makefile,v 1.17 2007/05/19 15:35:04 christos Exp $

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

PROG=	ldd
SRCS=	ldd.c \
	xmalloc.c debug.c expand.c map_object.c load.c search.c headers.c paths.c
MAN=	ldd.1

.PATH:	${.CURDIR}/..

.if (${MACHINE_ARCH} == "alpha") || (${MACHINE_ARCH} == "sparc64") || \
    (${MACHINE_ARCH} == "x86_64") || (${MACHINE_ARCH} == "powerpc64")
CPPFLAGS+= -DELFSIZE=64
.else
CPPFLAGS+= -DELFSIZE=32
.endif
.if (${MACHINE_ARCH} == "sparc") || (${MACHINE_ARCH} == "sparc64") || \
    (${MACHINE_ARCH} == "arm") || (${MACHINE_ARCH} == "m68k") || \
    (${MACHINE_ARCH} == "powerpc")
CPPFLAGS+= -DVARPSZ
.endif

LDELFSO=${NETBSDSRCDIR}/libexec/ld.elf_so
CPPFLAGS+= -I${LDELFSO} -DLIBDIR=\"${LIBDIR}\"	# -DDEBUG
CPPFLAGS+= -D_RTLD_SOURCE
.PATH: ${LDELFSO}

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?= -static
.endif

.include <bsd.prog.mk>
