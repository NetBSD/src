#	$NetBSD: srcs.inc,v 1.11 2003/07/24 14:16:33 itojun Exp $

CRYPTOINCS= \
	aes.inc asn1.inc bf.inc bio.inc bn.inc buffer.inc cast.inc \
	comp.inc conf.inc crypto.inc des.inc dh.inc dsa.inc dso.inc \
	ec.inc engine.inc err.inc evp.inc hmac.inc krb5.inc lhash.inc \
	md2.inc md4.inc md5.inc objects.inc ocsp.inc pem.inc \
	pkcs12.inc pkcs7.inc rand.inc rc2.inc rc4.inc ripemd.inc \
	rsa.inc sha.inc stack.inc txt_db.inc ui.inc x509.inc \
	x509v3.inc

# patented algorithms - see ../libcrypto_*
CRYPTOINCS+=	idea.inc
CRYPTOINCS+=	rc5.inc
CRYPTOINCS+=	mdc2.inc

CRYPTOINCS+=	man.inc

.for cryptoinc in ${CRYPTOINCS}
.if exists(${.CURDIR}/arch/${MACHINE_CPU}/${cryptoinc})
.include "${.CURDIR}/arch/${MACHINE_CPU}/${cryptoinc}"
.else
.include "${cryptoinc}"
.endif
.endfor
