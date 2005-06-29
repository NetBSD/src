#	$NetBSD: Makefile,v 1.41 2005/06/29 14:41:16 simonb Exp $

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

USE_SHLIBDIR=	yes
USE_FIPS=	yes

.include <bsd.own.mk>
.include <bsd.shlib.mk>
.include <bsd.sys.mk>		# for HOST_SH

# XXX There's a bit of work to do before we can enable warnings.
WARNS=0

LIB=	crypto
CPPFLAGS+= -Dlib${LIB} -I. -I${OPENSSLSRC}/crypto -I${OPENSSLSRC}

CRYPTODIST=	${NETBSDSRCDIR}/crypto/dist

.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"
.PATH: ${OPENSSLSRC}

.include "srcs.inc"


# XXX
.if ${OBJECT_FMT} == "ELF"
AFLAGS+=-DELF
LIBDPLIBS= crypt ${.CURDIR}/../libcrypt
.else
AFLAGS+=-DOUT
# XXX: This is broken because we compile with the wrong des headers.
# NetBSD's crypt(3) library, in lieu of the one included with OpenSSL,
# since NetBSD's also supports MD5, SHA1 and Blowfish passwords.
.PATH: ${NETBSDSRCDIR}/lib/libcrypt
SRCS+=	crypt.c md5crypt.c bcrypt.c crypt-sha1.c util.c
.endif

OS_VERSION!= ${HOST_SH} ${NETBSDSRCDIR}/sys/conf/osrelease.sh

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
INCS+=	aes.h asn1.h asn1_mac.h asn1t.h bio.h blowfish.h bn.h \
	buffer.h cast.h comp.h conf.h conf_api.h crypto.h des.h \
	dh.h dsa.h dso.h e_os2.h ebcdic.h ec.h engine.h \
	err.h evp.h hmac.h krb5_asn.h lhash.h md2.h md4.h md5.h \
	obj_mac.h objects.h ocsp.h opensslconf.h opensslv.h ossl_typ.h \
	pem.h pem2.h pkcs12.h pkcs7.h rand.h rc2.h rc4.h ripemd.h \
	rsa.h safestack.h sha.h stack.h symhacks.h tmdiff.h txt_db.h \
	ui.h ui_compat.h x509.h x509_vfy.h x509v3.h
#	des_old.h 

# IDEA - patented, but we install the header anyways
INCS+=	idea.h

# RC5 - patented, but we install the header anyways
INCS+=	rc5.h

# MDC2 - patented, but we install the header anyways
INCS+=	mdc2.h

# FIPS
# This part is always included, because OpenSSL does not protect
# The FIPS include files
.PATH: ${OPENSSLSRC}/fips ${OPENSSLSRC}/fips/rand
INCS+=	fips.h fips_rand.h

.if (${USE_FIPS} != "no")
CPPFLAGS+=-DOPENSSL_FIPS
.PATH: ${OPENSSLSRC}/fips/aes
SRCS+=fips_aes_core.c fips_aes_selftest.c
.PATH: ${OPENSSLSRC}/fips/des
SRCS+=fips_des_enc.c fips_des_selftest.c fips_set_key.c
# asm/fips-dx86-elf.s 
.PATH: ${OPENSSLSRC}/fips/dh
SRCS+=fips_dh_check.c fips_dh_gen.c fips_dh_key.c
.PATH: ${OPENSSLSRC}/fips/dsa
SRCS+=fips_dsa_ossl.c fips_dsa_gen.c fips_dsa_selftest.c
#.PATH: ${OPENSSLSRC}/fips/rand
SRCS+=fips_rand.c
.PATH: ${OPENSSLSRC}/fips/rsa
SRCS+=fips_rsa_eay.c fips_rsa_gen.c fips_rsa_selftest.c
SRCS+=fips_sha1dgst.c fips_sha1_selftest.c
# asm/sx86-elf.s
.PATH: ${OPENSSLSRC}/fips/sha1
#.PATH: ${OPENSSLSRC}/fips
SRCS+=fips.c fips_err_wrapper.c

SRCS+=rc5_skey.c i_skey.c mdc2dgst.c
.endif

INCSDIR=/usr/include/openssl

# this rebuilds the `srcs.inc' and the .inc files it reads.
# note that the resulting `srcs.inc' file needs to be modified
# to deal with our MKCRYPTO_* configuration variables.

update_inc:
	(cd ${.CURDIR}; find ${OPENSSLSRC}/crypto \
	    -name Makefile.ssl | \
	    perl ${OPENSSLSRC}/extsrcs.pl 2> srcs.inc )

.include <bsd.lib.mk>
