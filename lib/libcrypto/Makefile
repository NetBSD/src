#	$NetBSD: Makefile,v 1.2 2000/06/16 06:16:38 thorpej Exp $

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

LIB=	crypto
CPPFLAGS+= -Dlib${LIB} -I. -I${OPENSSLSRC}/crypto

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

# XXX
${SRCS}: buildinf.h
buildinf.h:
	@echo "#ifndef MK1MF_BUILD" >buildinf.h
	@echo "#define CFLAGS \"${CC} ${CFLAGS}\"" >>buildinf.h
	@echo "#define PLATFORM \"NetBSD-${MACHINE_ARCH}\"" >>buildinf.h
	@echo "#define DATE \"`date`\"" >>buildinf.h
	@echo "#endif" >>buildinf.h

CLEANFILES+= buildinf.h

# This list is built from the contents of the include/openssl
# directory in the OpenSSL source distribution.
INCS+=	asn1.h asn1_mac.h bio.h blowfish.h bn.h buffer.h cast.h comp.h
INCS+=	conf.h crypto.h des.h dh.h dsa.h e_os.h e_os2.h err.h evp.h hmac.h
INCS+=	lhash.h md2.h mdc2.h md5.h objects.h opensslconf.h
INCS+=	opensslv.h pem.h pem2.h pkcs12.h pkcs7.h rand.h rc2.h rc4.h
INCS+=	ripemd.h safestack.h sha.h stack.h tmdiff.h
INCS+=	txt_db.h x509.h x509_vfy.h x509v3.h

.if !defined(USELESS_CRYPTO)
INCS+=	rsa.h rc5.h idea.h
INCS+=	rsaref.h

.PATH: ${OPENSSLSRC}/rsaref
.endif

INCSDIR=/usr/include/openssl

# this rebuilds the `srcs.inc' and the .inc files it reads.
# note that we have no idea, rc5 or rsa here so we include
# them explicitely above if we are using these ciphers.

update_inc:
	(cd ${.CURDIR}; find ${OPENSSLSRC}/crypto \
	    -name Makefile.ssl | \
	    perl ${OPENSSLSRC}/extsrcs.pl 2> srcs.inc )

.include <bsd.lib.mk>
