#	$NetBSD: Makefile,v 1.1.2.1 2014/08/10 07:10:52 tls Exp $

.include <bsd.init.mk>
.include <bsd.shlib.mk>

LIB=		liblto_plugin

DIST=		${GCCDIST}
GNUHOSTDIST=	${DIST}

SRCS=		lto-plugin.c

CPPFLAGS+=	-DHAVE_CONFIG_H -I${DIST}/include -I.

LIBISMODULE=	1
USE_SHLIBDIR=	yes
REQUIRETOOLS=	yes
NOLINT=		# defined

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

.PATH:	${DIST}/lto-plugin

.else
.include <bsd.prog.mk> # do nothing
.endif
