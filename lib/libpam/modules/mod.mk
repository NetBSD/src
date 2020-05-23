#	$NetBSD: mod.mk,v 1.17 2020/05/23 00:43:33 rin Exp $

NOLINT=		# don't build a lint library
NOPROFILE=	# don't build a profile library
NOPICINSTALL=	# don't install _pic.a library

.include <bsd.own.mk>

.include "${.PARSEDIR}/../Makefile.inc"

.if defined(MLIBDIR)
LIBDIR=/usr/lib/${MLIBDIR}/security
.else
LIBDIR=/usr/lib/security
.endif
WARNS=6

.if ${MKPIC} != "no"
LIBDPLIBS+=   pam     ${NETBSDSRCDIR}/lib/libpam/libpam
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
