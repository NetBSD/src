LIB=netcipher
SRCS= rijndael-alg-fst.c rijndael-api-fst.c rijndael.c
SRCS+= modes.c
SRCS+= c_skey.c c_ecb.c c_enc.c c_cfb64.c c_ofb64.c # XXX

#SRCS+=cast5.c

MKMAN=no # XXX
WARNS=4

INCS=rijndael.h cast5.h
INCSDIR=/usr/include/netpgp

.include <bsd.lib.mk>
