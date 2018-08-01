#	$NetBSD: Makefile,v 1.1.1.4 2018/08/01 22:19:01 mrg Exp $

.include <bsd.init.mk>
.include <bsd.shlib.mk>

LIB=		liblto_plugin

DIST=		${GCCDIST}
GNUHOSTDIST=	${DIST}

SRCS=		lto-plugin.c hashtab.c simple-object.c pex-unix.c \
		pex-common.c argv.c make-temp-file.c concat.c \
		simple-object-elf.c simple-object-mach-o.c \
		simple-object-coff.c simple-object-xcoff.c \

# libiberty
SRCS+=		xstrerror.c xasprintf.c xvasprintf.c safe-ctype.c \
		vprintf-support.c

CPPFLAGS+=	-I${DIST}/include -I. 
CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-Dxstrdup=strdup -Dxexit=exit
CPPFLAGS+=	-Dxcalloc=calloc -Dxmalloc=malloc -Dxrealloc=realloc

LIBISMODULE=	1
USE_SHLIBDIR=	yes
REQUIRETOOLS=	yes
NOLINT=		# defined

# make this /usr/lib/gcc?
LIBDIR=		/usr/libexec

.if ${MKGCC} != "no"

SHLIB_MAJOR=	0
SHLIB_MINOR=	1

# Yuck, we have to run configure to generate this one...
CLEANFILES+=	config.h
HOST_CFLAGS+= -I${.OBJDIR}
DPSRCS+=	config.h
config.h: Makefile
	${_MKTARGET_CREATE}
	rm -rf .ab && \
	mkdir .ab && \
	(cd .ab && \
		AWK=${TOOL_AWK:Q} \
		CC=${HOST_CC:Q} CFLAGS=${HOST_CFLAGS:Q} \
		MAKE=${MAKE:Q} \
		CONFIG_SHELL=${HOST_SH:Q} \
		${HOST_SH} ${DIST}/libiberty/configure \
			--build=`${HOST_SH} ${GNUHOSTDIST}/config.guess` \
			--host=`${HOST_SH} ${GNUHOSTDIST}/config.guess` \
			--target=${MACHINE_GNU_PLATFORM} && \
		mv ${.TARGET} ../${.TARGET}) && \
	rm -rf .ab

.include <bsd.lib.mk>

.PATH:	${DIST}/lto-plugin ${DIST}/libiberty
COPTS.lto-plugin.c+= -Wno-missing-prototypes

.else
.include <bsd.prog.mk> # do nothing
.endif
