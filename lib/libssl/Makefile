#	$NetBSD: Makefile,v 1.3 2001/04/12 07:48:09 itojun Exp $

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

# XXX There's a bit of work to do before we can enable warnings.
WARNS=0

LIB=	ssl
CFLAGS+= -Dlib${LIB}

CRYPTODIST=	${.CURDIR}/../../crypto/dist

.include "../../crypto/Makefile.openssl"
.PATH: ${OPENSSLSRC}

.include "srcs.inc"

# XXX
.if ${OBJECT_FMT} == "ELF"
AFLAGS+=-DELF
.else
AFLAGS+=-DOUT
.endif

# This is from the include/openssl directory; see ../libcrypto/Makefile
INCS=	ssl.h ssl2.h ssl23.h ssl3.h tls1.h
INCSDIR=/usr/include/openssl

# this rebuilds the `srcs.inc' and the .inc files it reads.
# note that we have no idea, rc5 or rsa here so we include
# them explicitely above if we are using these ciphers.
# XXX
# we patch `ssl.inc' as we need to be able to turn off SSLv2.
update_inc:
	(cd ${.CURDIR}; \
	find ${OPENSSLSRC}/ssl -name Makefile.ssl | \
		perl ${OPENSSLSRC}/extsrcs.pl 2> srcs.inc; \
	patch -s ssl.inc < ssl.diff )

.include <bsd.lib.mk>
