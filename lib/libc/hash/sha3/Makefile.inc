#	$NetBSD: Makefile.inc,v 1.3 2026/10/04 03:28:44 mrg Exp $

.PATH: ${.PARSEDIR}

SRCS+=	keccak.c sha3.c

# XXX not (yet) public
#MAN+=	SHA3_Selftest.3 SHAKE.3 keccak.3 sha3.3

#MLINKS+=sha3.3 SHA3_224_Init.3 sha3.3 SHA3_224_Update.3 sha3.3 SHA3_224_Final.3
#MLINKS+=sha3.3 SHA3_256_Init.3 sha3.3 SHA3_256_Update.3 sha3.3 SHA3_256_Final.3
#MLINKS+=sha3.3 SHA3_384_Init.3 sha3.3 SHA3_384_Update.3 sha3.3 SHA3_384_Final.3
#MLINKS+=sha3.3 SHA3_512_Init.3 sha3.3 SHA3_512_Update.3 sha3.3 SHA3_512_Final.3
#MLINKS+=sha3.3 SHAKE128_Init.3 sha3.3 SHAKE128_Update.3 sha3.3 SHAKE128_Final.3
#MLINKS+=sha3.3 SHAKE256_Init.3 sha3.3 SHAKE256_Update.3 sha3.3 SHAKE256_Final.3
