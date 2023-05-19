#	$NetBSD: Makefile,v 1.5 2023/05/19 18:45:52 christos Exp $

# RCSid:
#	Id: Makefile,v 1.33 1998/11/11 11:53:53 sjg Exp
#
#	@(#) Copyright (c) 1994 Simon J. Gerraty
#
#	This file is provided in the hope that it will
#	be of use.  There is absolutely NO WARRANTY.
#	Permission to copy, redistribute or otherwise
#	use this file is hereby granted provided that 
#	the above copyright notice and this notice are
#	left intact. 
#      
#	Please send copies of changes and bug-fixes to:
#	sjg@quick.com.au
#
NOLINT=		# don't build a lint library
NOPROFILE=	# don't build a profile library
NOPICINSTALL=	# don't install _pic.a library
LIB=	legacy
USE_FORT?= yes	# cryptographic software
#DBG=-g

.include <bsd.own.mk>
.include <bsd.shlib.mk>

CWARNFLAGS.clang+=	-Wno-empty-body -Wno-unused-value -Wno-parentheses -Wno-implicit-int-float-conversion
# XXX: This warning seems to trigger incorrectly
CWARNFLAGS.clang+=	-Wno-atomic-alignment

LINTFLAGS+=	-X 161	# constant in conditional context
LINTFLAGS+=	-X 129	# expression has null effect
LINTFLAGS+=	-X 117	# bitwise '>>' on signed value possibly nonportable
LINTFLAGS+=	-X 231	# argument '%s' unused in function '%s'
LINTFLAGS+=	-X 220	# fallthrough on case statement
LINTFLAGS+=	-X 118	# semantics of '%s' change in ANSI C; use explicit cast

CPPFLAGS+= -Dlib${LIB} -I. -I${OPENSSLSRC}/crypto -I${OPENSSLSRC} -I${.CURDIR}
CPPFLAGS+= -I${OPENSSLSRC}/include -I${OPENSSLSRC}/crypto/include
CPPFLAGS+= -I${OPENSSLSRC}/crypto/asn1 -I${OPENSSLSRC}/crypto/evp
CPPFLAGS+= -I${OPENSSLSRC}/crypto/modes
CPPFLAGS+= -I${OPENSSLSRC}/../include
CPPFLAGS+= -I${OPENSSLSRC}/providers/common/include
CPPFLAGS+= -I${OPENSSLSRC}/providers/implementations/include
CPPFLAGS+= -I${.CURDIR}/../libdefault


CRYPTODIST=	${NETBSDSRCDIR}/crypto
OPENSSLINC=	${OPENSSLSRC}/include/openssl

.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"
.PATH: ${OPENSSLSRC} ${OPENSSLINC}
.PATH: ${OPENSSLSRC}/../include/openssl

.include "srcs.inc"

SRCS+=legacyprov.c

AFLAGS+=-DELF

OS_VERSION!= ${HOST_SH} ${NETBSDSRCDIR}/sys/conf/osrelease.sh


SHLIB_MAJOR=0
SHLIB_MINOR=0

LIBDIR=${OSSL_MODULESDIR}

.if ${MKPIC} != "no"
.PRECIOUS: ${DESTDIR}${LIBDIR}/${LIB}.so
libinstall:: ${DESTDIR}${LIBDIR}/${LIB}.so
.else
libinstall::
.endif

LDFLAGS+=-Wl,--version-script=${.CURDIR}/${LIB}.map

.if ${MKPICLIB} == "yes"
LIBDPLIBS+= common_pic ${.CURDIR}/../libcommon
.else
LIBDPLIBS+= common ${.CURDIR}/../libcommon
.endif
LIBDPLIBS+= crypto ${.CURDIR}/../libcrypto

.include <bsd.lib.mk>

${DESTDIR}${LIBDIR}/${LIB}.so: lib${LIB}.so.${SHLIB_FULLVERSION}
	${_MKTARGET_INSTALL}
	${INSTALL_FILE} -o ${LIBOWN} -g ${LIBGRP} -m ${LIBMODE} \
	    ${.ALLSRC} ${.TARGET}
