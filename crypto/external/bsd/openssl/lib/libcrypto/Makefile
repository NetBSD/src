#	$NetBSD: Makefile,v 1.21 2022/04/15 22:26:36 rillig Exp $

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

LIB=	crypto
USE_FORT?= yes	# cryptographic software
USE_SHLIBDIR=	yes
USE_FIPS=	no
#DBG=-g

.include <bsd.own.mk>
.include <bsd.shlib.mk>

# XXX: There's a bit of work to do before we can enable warnings.
WARNS=0
CWARNFLAGS.clang+=	-Wno-empty-body -Wno-unused-value -Wno-parentheses -Wno-implicit-int-float-conversion
# XXX: This warning seems to trigger incorrectly
CWARNFLAGS.clang+=	-Wno-atomic-alignment

# XXX Not yet.
LINTFLAGS+=-D__int128='long long'

LINTFLAGS+=	-X 161	# constant in conditional context
LINTFLAGS+=	-X 129	# expression has null effect
LINTFLAGS+=	-X 117	# bitwise '>>' on signed value possibly nonportable
LINTFLAGS+=	-X 231	# argument '%s' unused in function '%s'
LINTFLAGS+=	-X 220	# fallthrough on case statement
LINTFLAGS+=	-X 118	# semantics of '%s' change in ANSI C; use explicit cast

CPPFLAGS+= -Dlib${LIB} -I. -I${OPENSSLSRC}/crypto -I${OPENSSLSRC}
CPPFLAGS+= -I${OPENSSLSRC}/include -I${OPENSSLSRC}/crypto/include
CPPFLAGS+= -I${OPENSSLSRC}/crypto/asn1 -I${OPENSSLSRC}/crypto/evp
CPPFLAGS+= -I${OPENSSLSRC}/crypto/modes
CPPFLAGS+= -I${OPENSSLSRC}/../include

CRYPTODIST=	${NETBSDSRCDIR}/crypto

.include "${NETBSDSRCDIR}/crypto/Makefile.openssl"
.PATH: ${OPENSSLSRC} ${OPENSSLSRC}/include/openssl
.PATH: ${OPENSSLSRC}/../include/openssl ${OPENSSLSRC}/include/internal

.include "srcs.inc"

AFLAGS+=-DELF
LIBDPLIBS+= crypt ${NETBSDSRCDIR}/lib/libcrypt

OS_VERSION!= ${HOST_SH} ${NETBSDSRCDIR}/sys/conf/osrelease.sh

# XXX CFLAGS: While it would be nice to know which compiler flags
# XXX the library was built with, we don't want pathname information
# XXX for the host toolchain embedded in the image.
${SRCS}: buildinf.h
buildinf.h: Makefile
	@echo "#ifndef MK1MF_BUILD" >buildinf.h
	@echo "#define compiler_flags \"`${CC} -v 2>&1 | grep 'gcc version'`\"" >>buildinf.h
	@echo "#define PLATFORM \"NetBSD-${MACHINE_ARCH}\"" >>buildinf.h
	@echo "#define DATE \"NetBSD ${OS_VERSION}\"" >>buildinf.h
	@echo "#endif" >>buildinf.h

CLEANFILES+= buildinf.h

# This list is built from the contents of the include/openssl
# directory in the OpenSSL source distribution.
INCS+= \
aes.h \
asn1.h \
asn1_mac.h \
asn1err.h \
asn1t.h \
async.h \
asyncerr.h \
bio.h \
bioerr.h \
blowfish.h \
bn.h \
bnerr.h \
buffer.h \
buffererr.h \
camellia.h \
cast.h \
cmac.h \
cms.h \
cmserr.h \
comp.h \
comperr.h \
conf.h \
conf_api.h \
conferr.h \
crypto.h \
cryptoerr.h \
ct.h \
cterr.h \
des.h \
dh.h \
dherr.h \
dsa.h \
dsaerr.h \
dtls1.h \
e_os2.h \
ebcdic.h \
ec.h \
ecdh.h \
ecdsa.h \
ecerr.h \
engine.h \
engineerr.h \
err.h \
evp.h \
evperr.h \
hmac.h \
kdf.h \
kdferr.h \
lhash.h \
md2.h \
md4.h \
md5.h \
modes.h \
obj_mac.h \
objects.h \
objectserr.h \
ocsp.h \
ocsperr.h \
opensslconf.h \
opensslv.h \
ossl_typ.h \
pem.h \
pem2.h \
pemerr.h \
pkcs12.h \
pkcs12err.h \
pkcs7.h \
pkcs7err.h \
rand.h \
rand_drbg.h \
randerr.h \
rc2.h \
rc4.h \
ripemd.h \
rsa.h \
rsaerr.h \
safestack.h \
seed.h \
sha.h \
srp.h \
srtp.h \
ssl.h \
ssl2.h \
ssl3.h \
sslerr.h \
stack.h \
store.h \
storeerr.h \
symhacks.h \
tls1.h \
ts.h \
tserr.h \
txt_db.h \
ui.h \
uierr.h \
whrlpool.h \
x509.h \
x509_vfy.h \
x509err.h \
x509v3.h \
x509v3err.h

# IDEA - patented, but we install the header anyways
INCS+=	idea.h

# RC5 - patented, but we install the header anyways
INCS+=	rc5.h

# MDC2 - patented, but we install the header anyways
INCS+=	mdc2.h

.if (${USE_FIPS} != "no")
# FIPS
# This part is always included, because OpenSSL does not protect
# The FIPS include files
.PATH: ${OPENSSLSRC}/fips ${OPENSSLSRC}/fips/rand
INCS+=	fips.h fips_rand.h
.endif

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

COPTS.eng_padlock.c = -Wno-stack-protector

INCSDIR=/usr/include/openssl

LDFLAGS+=-Wl,--version-script=${.CURDIR}/crypto.map

PKGCONFIG=libcrypto
.include "${.CURDIR}/../../pkgconfig.mk"


.include <bsd.lib.mk>

