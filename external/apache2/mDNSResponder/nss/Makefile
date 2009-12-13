#	$NetBSD: Makefile,v 1.3 2009/12/13 08:02:36 mrg Exp $

LIB=		nss_mdnsd
SHLIB_MAJOR=	0
LINKS=  	${LIBDIR}/${LIB}.so.${SHLIB_MAJOR} \
		${LIBDIR}/nss_multicast_dns.so.${SHLIB_MAJOR}

NOLINT=		# don't build a lint library
NOPROFILE=	# don't build a profile library
NOPICINSTALL=	# don't install _pic.a library

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

SRCS=		nss_mdnsd.c
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
