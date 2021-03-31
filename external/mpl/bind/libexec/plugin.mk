#	$NetBSD: plugin.mk,v 1.1 2021/03/31 04:37:50 christos Exp $
#
# Based on src/lib/libpam/modules/mod.mk
#	NetBSD: mod.mk,v 1.17 2020/05/23 00:43:33 rin Exp

.include "${.CURDIR}/../../Makefile.inc"

DIST=		${IDIST}/bin/plugins
.PATH.c:	${DIST}

LIBDIR=		/usr/libexec/named

NOLINT=		# don't build a lint library
NOPROFILE=	# don't build a profile library
NOPICINSTALL=	# don't install _pic.a library

# Define the shared library version here, we need these variables early for
# plugin install rules.
SHLIB_MAJOR=	0
SHLIB_MINOR=	0

.include <bsd.own.mk>

.if ${MKPIC} != "no"
.PRECIOUS: ${DESTDIR}${LIBDIR}/${LIB}.so
libinstall:: ${DESTDIR}${LIBDIR}/${LIB}.so
.else
libinstall::
.endif

.include <bsd.lib.mk>

${DESTDIR}${LIBDIR}/${LIB}.so: lib${LIB}.so.${SHLIB_FULLVERSION}
	${_MKTARGET_INSTALL}
	${INSTALL_FILE} -o ${LIBOWN} -g ${LIBGRP} -m ${LIBMODE} \
	    ${.ALLSRC} ${.TARGET}
