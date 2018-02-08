#	$NetBSD: srcs.inc,v 1.5 2018/02/08 21:57:24 christos Exp $

CRYPTOINCS= \
aes.inc \
asn1.inc \
async.inc \
bf.inc \
bio.inc \
blake2.inc \
bn.inc \
buffer.inc \
camellia.inc \
cast.inc \
chacha.inc \
cmac.inc \
cms.inc \
comp.inc \
conf.inc \
crypto.inc \
ct.inc \
des.inc \
dh.inc \
dsa.inc \
dso.inc \
ec.inc \
engine.inc \
err.inc \
evp.inc \
hmac.inc \
idea.inc \
kdf.inc \
lhash.inc \
md2.inc \
md4.inc \
md5.inc \
mdc2.inc \
modes.inc \
objects.inc \
ocsp.inc \
pem.inc \
pkcs12.inc \
pkcs7.inc \
poly1305.inc \
rand.inc \
rc2.inc \
rc4.inc \
ripemd.inc \
rsa.inc \
seed.inc \
sha.inc \
srp.inc \
stack.inc \
ts.inc \
txt_db.inc \
ui.inc \
whrlpool.inc \
x509.inc \
x509v3.inc \

# patented algorithms - see ../libcrypto_*
CRYPTOINCS+=	rc5.inc

CRYPTOINCS+=	man.inc

.if exists(${.CURDIR}/arch/${MACHINE_ARCH})
CRYPTO_MACHINE_CPU?=    ${MACHINE_ARCH}
.else
CRYPTO_MACHINE_CPU?=    ${MACHINE_CPU}
.endif

.for cryptoinc in ${CRYPTOINCS}
.if exists(${.CURDIR}/arch/${CRYPTO_MACHINE_CPU}/${cryptoinc})
.include "${.CURDIR}/arch/${CRYPTO_MACHINE_CPU}/${cryptoinc}"
.else
.include "${cryptoinc}"
.endif
.endfor
