#	$NetBSD: Makefile,v 1.1 2009/10/05 03:54:17 tsarna Exp $

LIB=		nss_mdns
SHLIB_MAJOR=	0

NOLINT=		# don't build a lint library
NOPROFILE=	# don't build a profile library
NOPICINSTALL=	# don't install _pic.a library

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

SRCS=		nss_mdns.c
LDADD+= 	-ldns_sd
DPADD+= 	${LIBDNS_SD}

.if ${MKPIC} != "no"
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

#.if defined(LD32DIR)
#LIBDIR=/usr/lib/${LD32DIR}
#.else
#LIBDIR=/usr/lib
#.endif
