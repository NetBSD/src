#	$NetBSD: mod.mk,v 1.12 2013/12/28 18:04:18 christos Exp $

NOLINT=		# don't build a lint library
NOPROFILE=	# don't build a profile library
NOPICINSTALL=	# don't install _pic.a library

.include <bsd.own.mk>

.include "${.CURDIR}/../../Makefile.inc"

.if defined(MLIBDIR)
LIBDIR=/usr/lib/${MLIBDIR}/security
.else
LIBDIR=/usr/lib/security
.endif
WARNS=6

.if ${MKPIC} != "no"
LIBDPLIBS+=   pam     ${.CURDIR}/../../libpam
.PRECIOUS: ${DESTDIR}${LIBDIR}/${LIB}.so.${SHLIB_MAJOR}
libinstall:: ${DESTDIR}${LIBDIR}/${LIB}.so.${SHLIB_MAJOR}
.else
libinstall::
.endif

.include <bsd.lib.mk>

${DESTDIR}${LIBDIR}/${LIB}.so.${SHLIB_MAJOR}: lib${LIB}.so.${SHLIB_FULLVERSION}
	${_MKTARGET_INSTALL}
	${INSTALL_FILE} -o ${LIBOWN} -g ${LIBGRP} -m ${LIBMODE} \
	    ${.ALLSRC} ${.TARGET}
