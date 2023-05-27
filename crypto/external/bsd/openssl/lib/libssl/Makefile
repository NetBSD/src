#	$NetBSD: Makefile,v 1.18 2023/05/27 20:51:22 hgutch Exp $

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

.include <bsd.own.mk>

USE_FORT?=yes	# cryptographic software and network library

CWARNFLAGS.clang+=	-Wno-unused-value
# XXX: This warning seems to trigger incorrectly
CWARNFLAGS.clang+=	-Wno-atomic-alignment

LIB=	ssl
CPPFLAGS+= -Dlib${LIB} -I${OPENSSLSRC} -I${OPENSSLSRC}/crypto
CPPFLAGS+= -I${OPENSSLSRC}/include

CRYPTODIST=	${NETBSDSRCDIR}/crypto

.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"
.PATH: ${OPENSSLSRC} ${OPENSSLSRC}/crypto/pqueue ${OPENSSLSRC}/include/openssl

.include "srcs.inc"
.PATH: ${OPENSSLSRC}/crypto
SRCS+= packet.c

LIBDPLIBS+=crypto ${.CURDIR}/../libcrypto

AFLAGS+=-DELF

# This is from the include/openssl directory; see ../libcrypto/Makefile
INCS=	dtls1.h ssl.h ssl2.h ssl3.h tls1.h srtp.h async.h
INCSDIR=/usr/include/openssl

LDFLAGS+=-Wl,--version-script=${.CURDIR}/ssl.map

PKGCONFIG=libssl openssl
.include "${.CURDIR}/../../pkgconfig.mk"



GENH = \
ssl.h

.for i in ${GENH}
$i: $i.in
	${_MKTARGET_CREATE}
	${HOST_SH} ${.CURDIR}/../libcrypto/gen ${.ALLSRC} > ${.TARGET}
.endfor

.include <bsd.lib.mk>
