#	$NetBSD: Makefile,v 1.22 2002/06/26 16:58:09 thorpej Exp $

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

OS_VERSION!= sh ${NETBSDSRCDIR}/sys/conf/osrelease.sh

# XXX CFLAGS: While it would be nice to know which compiler flags
# XXX the library was built with, we don't want pathname information
# XXX for the host toolchain embedded in the image.
${SRCS}: buildinf.h
buildinf.h: Makefile
	@echo "#ifndef MK1MF_BUILD" >buildinf.h
	@echo "#define CFLAGS \"`${CC} -v 2>&1 | grep version`\"" >>buildinf.h
	@echo "#define PLATFORM \"NetBSD-${MACHINE_ARCH}\"" >>buildinf.h
	@echo "#define DATE \"NetBSD ${OS_VERSION}\"" >>buildinf.h
	@echo "#endif" >>buildinf.h

CLEANFILES+= buildinf.h

# This list is built from the contents of the include/openssl
# directory in the OpenSSL source distribution.
INCS+=	asn1.h asn1_mac.h bio.h blowfish.h bn.h buffer.h cast.h comp.h
INCS+=	conf.h conf_api.h crypto.h des.h dh.h dsa.h dso.h e_os.h e_os2.h
INCS+=	ebcdic.h err.h evp.h hmac.h lhash.h md2.h md4.h
INCS+=	md5.h mdc2.h obj_mac.h objects.h opensslconf.h opensslv.h pem.h
INCS+=	pem2.h pkcs12.h pkcs7.h rand.h rc2.h rc4.h ripemd.h
INCS+=	rsaref.h safestack.h sha.h stack.h
INCS+=	symhacks.h tmdiff.h txt_db.h x509.h x509_vfy.h x509v3.h

INCS+=	rsa.h rsaref.h
.PATH: ${OPENSSLSRC}/rsaref

# IDEA - patented, but we install the header anyways
INCS+=	idea.h

# RC5 - patented, but we install the header anyways
INCS+=	rc5.h

INCSDIR=/usr/include/openssl

# this rebuilds the `srcs.inc' and the .inc files it reads.
# note that the resulting `srcs.inc' file needs to be modified
# to deal with our MKCRYPTO_* configuration variables.

update_inc:
	(cd ${.CURDIR}; find ${OPENSSLSRC}/crypto \
	    -name Makefile.ssl | \
	    perl ${OPENSSLSRC}/extsrcs.pl 2> srcs.inc )

SYMLINKS+=	libcrypto.a ${LIBDIR}/libdes.a

.if ${MKPROFILE} != "no"
SYMLINKS+=	libcrypto_p.a ${LIBDIR}/libdes_p.a
.endif

.if ${MKPIC} != "no"

DES_SHLIB_MAJOR != cd ${.CURDIR}/../libdes && ${MAKE} -B print-shlib-major
DES_SHLIB_MINOR != cd ${.CURDIR}/../libdes && ${MAKE} -B print-shlib-minor

.if ${MKPICINSTALL} != "no"
SYMLINKS+=	libcrypto_pic.a ${LIBDIR}/libdes_pic.a
.endif

.if exists(${.CURDIR}/shlib_version)
SYMLINKS+=	libcrypto.so.${SHLIB_MAJOR}.${SHLIB_MINOR} \
		${_LIBSODIR}/libdes.so.${DES_SHLIB_MAJOR}.${DES_SHLIB_MINOR}
.if ${_LIBSODIR} != ${LIBDIR}
SYMLINKS+=	libcrypto.so.${SHLIB_MAJOR}.${SHLIB_MINOR} \
		${LIBDIR}/libdes.so.${DES_SHLIB_MAJOR}.${DES_SHLIB_MINOR}
.endif

.if (${OBJECT_FMT} == "ELF")
SYMLINKS+=	libcrypto.so.${SHLIB_MAJOR} \
		${_LIBSODIR}/libdes.so.${DES_SHLIB_MAJOR}
SYMLINKS+=	libcrypto.so ${_LIBSODIR}/libdes.so
.if ${_LIBSODIR} != ${LIBDIR}
SYMLINKS+=	libcrypto.so.${SHLIB_MAJOR} \
		${LIBDIR}/libdes.so.${DES_SHLIB_MAJOR}
SYMLINKS+=	libcrypto.so ${LIBDIR}/libdes.so
.endif
.endif
.endif	# exists shlib_version

.endif  # ${MKPIC} != "no"

.include <bsd.lib.mk>
