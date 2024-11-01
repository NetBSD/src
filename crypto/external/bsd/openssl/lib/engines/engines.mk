#	$NetBSD: engines.mk,v 1.7 2024/11/01 23:44:04 riastradh Exp $

NOLINT=		# don't build a lint library
NOPROFILE=	# don't build a profile library
NOPICINSTALL=	# don't install _pic.a library

.include <bsd.own.mk>

SHLIB_MAJOR=0
SHLIB_MINOR=0

CRYPTODIST=     ${NETBSDSRCDIR}/crypto
.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"
.PATH: ${OPENSSLSRC}/engines

CPPFLAGS+= -I${OPENSSLSRC}/include -I${OPENSSLSRC}/../include

LIBDPLIBS+=crypto ${.CURDIR}/../../libcrypto

LIBDIR=${OSSL_ENGINESDIR}

.if ${MKPIC} != "no"
.PRECIOUS: ${DESTDIR}${LIBDIR}/${LIB}.so
libinstall:: ${DESTDIR}${LIBDIR}/${LIB}.so
.else
libinstall::
.endif

VERSION_MAP=	${LIB}.map

.include <bsd.lib.mk>

${DESTDIR}${LIBDIR}/${LIB}.so: lib${LIB}.so.${SHLIB_FULLVERSION}
	${_MKTARGET_INSTALL}
	${INSTALL_FILE} -o ${LIBOWN} -g ${LIBGRP} -m ${LIBMODE} \
	    ${.ALLSRC} ${.TARGET}
