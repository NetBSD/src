#	$NetBSD: srcs.inc,v 1.2.2.4 2002/08/29 16:42:06 he Exp $

CRYPTOINCS= \
	asn1.inc bf.inc bio.inc bn.inc buffer.inc cast.inc comp.inc conf.inc \
	crypto.inc des.inc dh.inc dsa.inc dso.inc err.inc evp.inc hmac.inc \
	lhash.inc md2.inc md4.inc md5.inc mdc2.inc objects.inc pem.inc \
	pkcs12.inc pkcs7.inc rand.inc rc2.inc rc4.inc ripemd.inc rsa.inc \
	sha.inc stack.inc txt_db.inc x509.inc x509v3.inc

# patented algorithms - see ../libcrypto_*
CRYPTOINCS+=	idea.inc
CRYPTOINCS+=	rc5.inc

CRYPTOINCS+=	man.inc

.for cryptoinc in ${CRYPTOINCS}
.if exists(${.CURDIR}/arch/${MACHINE_CPU}/${cryptoinc})
.include "${.CURDIR}/arch/${MACHINE_CPU}/${cryptoinc}"
.else
.include "${cryptoinc}"
.endif
.endfor
