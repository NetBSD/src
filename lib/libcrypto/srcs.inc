#	$NetBSD: srcs.inc,v 1.5 2000/07/31 17:04:05 thorpej Exp $

CRYPTOINCS=	asn1.inc crypto.inc bf.inc bio.inc bn.inc buffer.inc \
		cast.inc comp.inc conf.inc des.inc dh.inc dsa.inc \
		err.inc evp.inc hmac.inc lhash.inc md2.inc mdc2.inc \
		objects.inc pem.inc pkcs12.inc pkcs7.inc rand.inc rc2.inc \
		rc4.inc md5.inc ripemd.inc sha.inc stack.inc txt_db.inc \
		x509.inc x509v3.inc

.if (${MKCRYPTO_RSA} != "no")
CRYPTOINCS+=	rsa.inc
.endif

.if (${MKCRYPTO_IDEA} != "no")
CRYPTOINCS+=	idea.inc
.endif

.if (${MKCRYPTO_RC5} != "no")
CRYPTOINCS+=	rc5.inc
.endif

.for cryptoinc in ${CRYPTOINCS}
.if exists(${.CURDIR}/arch/${MACHINE_ARCH:C/mipse[bl]/mips/}/${cryptoinc})
.include "${.CURDIR}/arch/${MACHINE_ARCH:C/mipse[bl]/mips/}/${cryptoinc}"
.else
.include "${cryptoinc}"
.endif
.endfor
