#	$NetBSD: Makefile,v 1.1 2014/03/01 10:00:42 mrg Exp $

.include <bsd.own.mk>
.include <bsd.shlib.mk>

NOLINT=		# defined

LIBISMODULE=	1
USE_SHLIBDIR=	yes
REQUIRETOOLS=	yes
DIST=		${NETBSDSRCDIR}/external/gpl3/gcc/dist
GNUHOSTDIST=	${DIST}
LIB=		liblto_plugin
SRCS=		lto-plugin.c
.PATH:	${DIST}/lto-plugin
CPPFLAGS+=	-DHAVE_CONFIG_H -I${DIST}/include -I.

# make this /usr/lib/gcc?
LIBDIR=		/usr/libexec

.if ${MKGCC} != "no"

SHLIB_MAJOR=	0
SHLIB_MINOR=	0

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
		${HOST_SH} ${DIST}/lto-plugin/configure \
			--build=`${HOST_SH} ${GNUHOSTDIST}/config.guess` \
			--host=`${HOST_SH} ${GNUHOSTDIST}/config.guess` \
			--target=${MACHINE_GNU_PLATFORM} && \
		mv ${.TARGET} ../${.TARGET}) && \
	rm -rf .ab

.include <bsd.lib.mk>

.else
.include <bsd.prog.mk> # do nothing
.endif
