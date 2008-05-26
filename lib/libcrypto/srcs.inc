#	$NetBSD: srcs.inc,v 1.15 2008/05/26 16:51:07 christos Exp $

CRYPTOINCS= \
	aes.inc asn1.inc bf.inc bio.inc bn.inc buffer.inc cast.inc \
	cms.inc comp.inc conf.inc crypto.inc des.inc dh.inc dsa.inc dso.inc \
	ec.inc ecdh.inc ecdsa.inc engine.inc err.inc evp.inc hmac.inc \
	krb5.inc lhash.inc camellia.inc \
	md2.inc md4.inc md5.inc objects.inc ocsp.inc pem.inc \
	pkcs12.inc pkcs7.inc pqueue.inc rand.inc rc2.inc rc4.inc ripemd.inc \
	rsa.inc sha.inc stack.inc store.inc txt_db.inc ts.inc ui.inc x509.inc \
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
