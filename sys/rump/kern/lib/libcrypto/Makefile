#	$NetBSD: Makefile,v 1.1 2010/12/05 20:11:22 pooka Exp $
#

.PATH:	${.CURDIR}/../../../../crypto/arc4				\
	${.CURDIR}/../../../../crypto/blowfish				\
	${.CURDIR}/../../../../crypto/cast128				\
	${.CURDIR}/../../../../crypto/des				\
	${.CURDIR}/../../../../crypto/rijndael				\
	${.CURDIR}/../../../../crypto/skipjack

LIB=	rumpkern_crypto

# arc4
SRCS+=	arc4.c

# blowfish
SRCS+=	bf_ecb.c bf_enc.c bf_cbc.c bf_skey.c

# cast128
SRCS+=	cast128.c

# DES
SRCS+=	des_ecb.c des_setkey.c des_enc.c des_cbc.c

# rijndael
SRCS+=	rijndael-alg-fst.c rijndael-api-fst.c rijndael.c

# skipjack
SRCS+=	skipjack.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
