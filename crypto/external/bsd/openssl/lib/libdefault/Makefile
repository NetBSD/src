#	$NetBSD: Makefile,v 1.6 2023/05/25 15:52:29 riastradh Exp $

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
LIBISPRIVATE=	yes
LIB=	default
USE_FORT?= yes	# cryptographic software
USE_SHLIBDIR=	no
USE_FIPS=	no
#DBG=-g

.include <bsd.own.mk>
.include <bsd.shlib.mk>

CPPFLAGS+= -Dlib${LIB} -I. -I${OPENSSLSRC}/crypto -I${OPENSSLSRC} -I${.CURDIR}
CPPFLAGS+= -I${OPENSSLSRC}/include -I${OPENSSLSRC}/crypto/include
CPPFLAGS+= -I${OPENSSLSRC}/crypto/asn1 -I${OPENSSLSRC}/crypto/evp
CPPFLAGS+= -I${OPENSSLSRC}/crypto/modes
CPPFLAGS+= -I${OPENSSLSRC}/../include
CPPFLAGS+= -I${OPENSSLSRC}/providers/common/include
CPPFLAGS+= -I${OPENSSLSRC}/providers/implementations/include

CRYPTODIST=	${NETBSDSRCDIR}/crypto
OPENSSLINC=	${OPENSSLSRC}/include/openssl

.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"
.PATH: ${OPENSSLSRC} ${OPENSSLINC}
.PATH: ${OPENSSLSRC}/../include/openssl
.PATH: ${OPENSSLSRC}/providers/implementations/digests
.PATH: ${OPENSSLSRC}/providers/implementations/macs

.include "srcs.inc"

AFLAGS+=-DELF

OS_VERSION!= ${HOST_SH} ${NETBSDSRCDIR}/sys/conf/osrelease.sh

.include <bsd.lib.mk>
