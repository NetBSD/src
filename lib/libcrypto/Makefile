#	$NetBSD: Makefile,v 1.3.2.4 2000/07/05 18:30:07 thorpej Exp $

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

# XXX There's a bit of work to do before we can enable warnings.
WARNS=0

LIB=	crypto
CPPFLAGS+= -Dlib${LIB} -I. -I${OPENSSLSRC}/crypto

CRYPTODIST=	${.CURDIR}/../../crypto/dist

.include "../../crypto/Makefile.openssl"
.PATH: ${OPENSSLSRC}

.include "srcs.inc"

# NetBSD local addition to des library.
SRCS+=  rnd_keys.c

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

.if (${MKCRYPTO_RSA} != "no")
INCS+=	rsa.h rsaref.h
.PATH: ${OPENSSLSRC}/rsaref
.else
CPPFLAGS+= -DNO_RSA
.endif

.if (${MKCRYPTO_IDEA} != "no")
INCS+=	idea.h
.else
CPPFLAGS+= -DNO_IDEA
.endif

.if (${MKCRYPTO_RC5} != "no")
INCS+=	rc5.h
.else
CPPFLAGS+= -DNO_RC5
.endif

INCSDIR=/usr/include/openssl

# this rebuilds the `srcs.inc' and the .inc files it reads.
# note that the resulting `srcs.inc' file needs to be modified
# to deal with our MKCRYPTO_* configuration variables.

update_inc:
	(cd ${.CURDIR}; find ${OPENSSLSRC}/crypto \
	    -name Makefile.ssl | \
	    perl ${OPENSSLSRC}/extsrcs.pl 2> srcs.inc )

DES_SHLIB_MAJOR != cd ${.CURDIR}/../libdes && make print-shlib-major
DES_SHLIB_MINOR != cd ${.CURDIR}/../libdes && make print-shlib-minor

LINKS+= ${LIBDIR}/libcrypto.a ${LIBDIR}/libdes.a

.if ${MKPROFILE} != "no"
LINKS+= ${LIBDIR}/libcrypto_p.a ${LIBDIR}/libdes_p.a
.endif

.if ${MKPIC} != "no"

.if ${MKPICINSTALL} != "no"
LINKS+= ${LIBDIR}/libcrypto_pic.a ${LIBDIR}/libdes_pic.a
.endif

.if exists(${.CURDIR}/shlib_version)
LINKS+=	${LIBDIR}/libcrypto.so.${SHLIB_MAJOR}.${SHLIB_MINOR} \
	${LIBDIR}/libdes.so.${DES_SHLIB_MAJOR}.${DES_SHLIB_MINOR}

.if (${OBJECT_FMT} == "ELF")
LINKS+=	${LIBDIR}/libcrypto.so.${SHLIB_MAJOR} \
	${LIBDIR}/libdes.so.${DES_SHLIB_MAJOR}
LINKS+=	${LIBDIR}/libcrypto.so ${LIBDIR}/libdes.so
.endif
.endif

.endif  # ${MKPIC} != "no"

.include <bsd.lib.mk>
